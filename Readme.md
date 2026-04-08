# QW3100 Modbus — Sensor Gateway

Daemon C embebido para Linux ARM que simula el comportamiento de un gateway Poseidon AP2200. Lee datos del sensor QW3100 vía Modbus RTU, los persiste localmente y los envía a la API Scante (HTTP) o AWS IoT Core (MQTT) con reintentos, cola FIFO y circuit breaker.

---

## Inicio rápido

```bash
# 1. Compilar dependencias ARM (una sola vez)
./scripts/build_third_party.sh

# 2. Compilar para ARM
make

# 3. Enviar al dispositivo
make deploy

# 4. Ejecutar en el dispositivo
make run
```

---

## Requisitos de compilación

```bash
# Toolchain ARM + build tools
sudo apt install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf \
    binutils-arm-linux-gnueabihf meson ninja-build cmake git perl

# Paquetes devlinux (para tests en el PC de desarrollo)
sudo apt install libmodbus-dev libcurl4-openssl-dev libmosquitto-dev libssl-dev
```

Ver [docs/WIKI_SETUP.md](docs/WIKI_SETUP.md) para instrucciones completas.

---

## Makefile

```bash
make          # compila binario ARM → sensor_trident_modbus_ARM
make test     # compila y ejecuta tests en el PC de desarrollo
make deploy   # arm + scp al dispositivo (requiere alias SSH)
make run      # deploy + ejecuta vía SSH
make clean    # elimina build-arm/ build-devlinux/ y binario
```

**Alias SSH requerido** (`~/.ssh/config`):

```
Host qw3100-device
  HostName <IP_DEL_DISPOSITIVO>
  Port 2122
  User root
```

---

## Configuración

El binario carga `/SD/qw3100-config.json` por defecto. Se puede cambiar con `--config <ruta>`.

Todos los campos son opcionales — si no están presentes se usan los valores por defecto.

```json
{
    "interval_sec": 120,
    "serial_port": "/dev/ttymxc2",
    "slave_id": 1,
    "persist_path": "/SD/pending",
    "persist_sent_path": "/SD/sent",

    "web_interface": {
        "primary": "api"
    },

    "api": {
        "enabled": true,
        "base_url": "https://apiv4-integration.productious.com",
        "item_guid": "<item_guid>",
        "pull_type_guid": "<pull_type_guid>",
        "scante_token": "<token>",
        "scante_appid": "<appid>",
        "scante_sgid": "<sgid>",
        "ca_bundle_path": ""
    },

    "mqtt": {
        "enabled": false,
        "broker_url": "<endpoint>.iot.<region>.amazonaws.com",
        "port": 8883,
        "thing_name": "<thing_name>",
        "cert_path": "/SD/certs/device.crt",
        "key_path": "/SD/certs/device.key",
        "ca_path": "/SD/certs/root-CA.crt",
        "connect_timeout_sec": 10,
        "keepalive_sec": 60
    },

    "send": {
        "fifo_max_per_cycle": 10,
        "sent_retention_count": 1000,
        "cb_fail_threshold": 5,
        "cb_open_timeout_sec": 60,
        "cb_backoff_max_sec": 300
    }
}
```

### Referencia de parámetros

| Campo | Default | Descripción |
|-------|---------|-------------|
| `interval_sec` | `120` | Intervalo entre lecturas (segundos) |
| `serial_port` | `/dev/ttymxc2` | Puerto RS485 para Modbus RTU |
| `slave_id` | `1` | ID del slave Modbus |
| `persist_path` | `/SD/pending` | Directorio para JSONs pendientes de envío |
| `persist_sent_path` | `/SD/sent` | Directorio para JSONs ya enviados |
| `web_interface.primary` | `api` | Interfaz principal (`api` o `mqtt`) |
| `api.enabled` | `true` | Habilita/deshabilita el envío HTTP |
| `api.base_url` | — | URL base de la API Scante |
| `api.ca_bundle_path` | `""` | CA bundle para TLS; vacío = CA del sistema |
| `mqtt.enabled` | `false` | Habilita/deshabilita MQTT |
| `mqtt.broker_url` | — | Endpoint del broker MQTT |
| `mqtt.port` | `8883` | Puerto del broker |
| `mqtt.thing_name` | — | Nombre del Thing en AWS IoT |
| `mqtt.ca_path` | — | CA del broker; vacío = sin TLS |
| `mqtt.cert_path` | — | Certificado del cliente (mTLS, opcional) |
| `mqtt.key_path` | — | Clave privada del cliente (mTLS, opcional) |
| `send.fifo_max_per_cycle` | `10` | Archivos pendientes a enviar por ciclo |
| `send.sent_retention_count` | `1000` | Máximo de archivos en `sent/` (rotación FIFO) |
| `send.cb_fail_threshold` | `5` | Fallos para abrir el circuit breaker |
| `send.cb_open_timeout_sec` | `60` | Timeout inicial antes del primer reintento |
| `send.cb_backoff_max_sec` | `300` | Tope del backoff exponencial |

### Override por CLI

```bash
./sensor_trident_modbus_ARM --config /ruta/config.json --interval 60
```

---

## Flujo de operación

```
Arranque
  └─ modbus_connect()
  └─ modbus_wait_first_sweep()   ← espera hasta sweepCount > 0 (máx 150s)

while(1):
  ├─ read_block_modbus(addr=21)  ← info block (uptime, fw, sn...)
  ├─ read_block_modbus(addr=79)  ← data block (mag, phase, temp, rh...)
  ├─ build_gateway_payload_json()
  ├─ persist_write()             ← guarda <ts>.json en /SD/pending/
  ├─ try_send_pending()          ← envía FIFO con circuit breaker y failover
  ├─ [status] primary=X  cb_api=Y  cb_mqtt=Z  pendientes=N
  └─ sleep(interval_sec)
```

---

## Circuit breaker

Hay dos instancias independientes (`cb_api` y `cb_mqtt`). Si la interfaz principal abre su CB y la alternativa está habilitada, el daemon hace failover automático.

| Estado | Comportamiento |
|--------|---------------|
| `CLOSED` | Normal — envía hasta `fifo_max_per_cycle` archivos/ciclo |
| `OPEN` | Pausado — espera `cb_open_timeout_sec` antes de reintentar |
| `HALF_OPEN` | Prueba — envía 1 archivo para verificar si el servidor volvió |

**Backoff exponencial** en cada reapertura: `60s → 120s → 240s → 300s (tope)`

| Código/Error | Tipo | Acción |
|--------------|------|--------|
| `200` / MQTT ACK | Éxito | `cb_on_success()` → CLOSED |
| `5xx` / timeout / MQTT connect error | Transitorio | Cuenta hacia apertura del CB |
| `4xx` / MQTT build error | Persistente | Apertura inmediata del CB |

---

## MQTT — formato del payload

Los mensajes se publican en `$aws/things/<thing_name>/shadow/update` con formato AWS Device Shadow:

```json
{
  "state": { "reported": <payload_canónico_gateway> },
  "clientToken": "<thing_name>-<timestamp>",
  "version": 1
}
```

---

## Reconexión Modbus

`read_block_modbus()` distingue dos tipos de fallo:

- **Excepción Modbus** (`Illegal function`, `Slave busy`): espera sin cerrar la conexión.
- **Timeout / fallo de bus**: cierra el puerto, reconecta y verifica con una lectura real.

Backoff: `2s → 4s → 8s → 16s → 32s` (5 intentos).

---

## Warmup del sensor

El QW3100 tarda ~120s en completar su primer barrido EIS al energizarse:

```
[INFO]  [warmup] esperando primer barrido EIS (timeout 150s, poll 10s)
[INFO]  [warmup] sensor responde, aún procesando sweepCount=0 (20s)
[INFO]  [warmup] sensor listo, sweepCount=1, elapsed=110s
```

Si el daemon se reinicia con el sensor ya encendido, el warmup retorna inmediatamente.

---

## Tests en el PC de desarrollo

```bash
make test
# Resultados: 74 OK / 0 FAIL
```

---

## Mock server (pruebas sin API real)

```bash
python3 scripts/mock_server.py              # 200 OK
python3 scripts/mock_server.py --fail 500   # 5xx (transitorio)
python3 scripts/mock_server.py --fail 401   # 4xx (persistente)
python3 scripts/mock_server.py --tls        # con TLS autofirmado
```

Configurar `api.base_url` a `http://<IP_HOST>:9090` en el dispositivo.

---

## Broker MQTT local (pruebas)

```bash
sudo apt install mosquitto mosquitto-clients
sudo tee /etc/mosquitto/conf.d/local-test.conf <<EOF
listener 1883 0.0.0.0
allow_anonymous true
EOF
sudo systemctl restart mosquitto

# Suscribirse (usar comillas simples para el $)
mosquitto_sub -h localhost -t '$aws/things/#' -v
```

---

## Instalación como servicio systemd

```bash
scp deploy/qw3100-modbus.service qw3100-device:/etc/systemd/system/
ssh qw3100-device "systemctl daemon-reload && systemctl enable qw3100-modbus && systemctl start qw3100-modbus"
ssh qw3100-device "journalctl -u qw3100-modbus -f"
```

---

## Estructura del proyecto

```
QW3100_modbus/
├── src/                  — código fuente del daemon
├── lib/cJSON.c/h         — serialización JSON (vendored)
├── test/main_test.c      — 74 tests unitarios
├── third_party/arm/      — dependencias ARM precompiladas (.a)
├── cross/armv7.ini       — cross-file Meson para arm-linux-gnueabihf
├── deploy/               — unit file systemd
├── scripts/
│   ├── build_third_party.sh  — compila deps ARM (ejecutar una vez)
│   └── mock_server.py        — servidor HTTP de prueba
├── docs/                 — documentación extendida
├── meson.build           — build system
├── Makefile              — wrapper de Meson
└── qw3100-config.json    — ejemplo de configuración completo
```
