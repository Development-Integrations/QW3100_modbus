# QW3100 Modbus — Sensor Gateway

Daemon C embebido para Linux ARM que simula el comportamiento de un gateway Poseidon AP2200. Lee datos del sensor QW3100 vía Modbus RTU, los persiste localmente y los envía a la API Scante con reintentos, cola FIFO y circuit breaker.

---

## Inicio rápido

```bash
# 1. Compilar para ARM
make

# 2. Enviar al dispositivo
make deploy

# 3. Ejecutar en el dispositivo
make run
```

---

## Requisitos de compilación

| Dependencia | Ruta | Propósito |
|-------------|------|-----------|
| `arm-linux-gnueabihf-gcc` | `apt install gcc-arm-linux-gnueabihf` | Compilador cruzado |
| `libmodbus` ARM estática | `$HOME/opt/libmodbus-arm/` | Comunicación Modbus RTU |
| `libcurl` ARM estática (sin SSL) | `$HOME/opt/libcurl-arm/` | HTTP POST a API |
| `libmodbus` devlinux | `$HOME/opt/libmodbus-devlinux/` | Pruebas en el PC de desarrollo |

Ver [docs/WIKI_SETUP.md](docs/WIKI_SETUP.md) para instrucciones de compilación de dependencias.

---

## Makefile

```bash
make          # compila binario ARM → sensor_trident_modbus_ARM
make devlinux # compila tests en el PC de desarrollo → test/main_test
make deploy   # arm + scp al dispositivo (requiere alias SSH)
make run      # deploy + ejecuta vía SSH
make clean    # elimina binarios
```

**Alias SSH requerido** (`~/.ssh/config`):

```
Host qw3100-device
  HostName <IP_DEL_DISPOSITIVO>
  Port 2122
  User root
```

**Variables de entorno (opcionales, tienen defaults):**

```bash
export PREFIX_ARM=$HOME/opt/libmodbus-arm
export PREFIX_CURL=$HOME/opt/libcurl-arm
export PREFIX_DEVLINUX=$HOME/opt/libmodbus-devlinux
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

    "api": {
        "enabled": true,
        "base_url": "https://apiv4-integration.productious.com",
        "item_guid": "<item_guid>",
        "pull_type_guid": "<pull_type_guid>",
        "scante_token": "<token>",
        "scante_appid": "<appid>",
        "scante_sgid": "<sgid>"
    },

    "send": {
        "fifo_max_per_cycle": 10,
        "cb_fail_threshold": 5,
        "cb_open_timeout_sec": 60,
        "cb_backoff_max_sec": 300
    }
}
```

### Referencia de parámetros

| Campo | Default | Descripción |
|-------|---------|-------------|
| `interval_sec` | `120` | Intervalo entre lecturas del sensor (segundos) |
| `serial_port` | `/dev/ttymxc2` | Puerto RS485 para Modbus RTU |
| `slave_id` | `1` | ID del slave Modbus |
| `persist_path` | `/SD/pending` | Directorio para JSONs pendientes de envío |
| `api.enabled` | `true` | Habilita/deshabilita el envío HTTP |
| `api.base_url` | — | URL base de la API Scante |
| `api.item_guid` | — | GUID del item en la plataforma |
| `api.pull_type_guid` | — | GUID del tipo de pull |
| `api.scante_token` | — | Token de autenticación |
| `api.scante_appid` | — | App ID de la plataforma |
| `api.scante_sgid` | — | Subgroup ID |
| `send.fifo_max_per_cycle` | `10` | Máximo de archivos pendientes a enviar por ciclo |
| `send.cb_fail_threshold` | `5` | Fallos consecutivos para abrir el circuit breaker |
| `send.cb_open_timeout_sec` | `60` | Tiempo de espera inicial antes del primer reintento (s) |
| `send.cb_backoff_max_sec` | `300` | Tope máximo del backoff exponencial (s) |

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
  ├─ [ciclo] LOG_INFO cb=STATE fallos=N
  ├─ read_block_modbus(addr=21)  ← info block (uptime, fw, sn...)
  ├─ read_block_modbus(addr=79)  ← data block (mag, phase, temp, rh...)
  ├─ build_gateway_payload_json()
  ├─ persist_write()             ← guarda <ts>.json en /SD/pending/
  ├─ try_send_pending()          ← envía FIFO con circuit breaker
  ├─ [status] LOG_INFO cb=STATE pendientes=N
  └─ sleep(interval_sec)
```

---

## Circuit breaker

Protege el canal HTTP ante fallos continuos. Evita reintentos que saturen la red celular.

| Estado | Comportamiento |
|--------|---------------|
| `CLOSED` | Normal — envía hasta `fifo_max_per_cycle` archivos/ciclo |
| `OPEN` | Pausado — espera `cb_open_timeout_sec` antes de reintentar |
| `HALF_OPEN` | Prueba — envía 1 archivo para verificar si el servidor volvió |

**Backoff exponencial** en cada reapertura: `60s → 120s → 240s → 300s (tope)`

**Clasificación de errores HTTP:**

| Código | Tipo | Acción |
|--------|------|--------|
| `200` | Éxito | `cb_on_success()` → CLOSED |
| `5xx` / timeout | Transitorio | Cuenta hacia apertura del CB |
| `400` / `401` / `403` | Persistente | Apertura inmediata del CB |

---

## Reconexión Modbus

`read_block_modbus()` distingue dos tipos de fallo:

- **Excepción Modbus** (`Illegal function`, `Slave busy`): el slave responde pero no está listo — espera sin cerrar la conexión.
- **Timeout / fallo de bus**: cierra el puerto, reconecta y verifica con una lectura real.

Backoff: `2s → 4s → 8s → 16s → 32s` (5 intentos). Si agota → el ciclo continúa en el próximo `interval_sec`.

---

## Warmup del sensor

El QW3100 tarda ~120s en completar su primer barrido EIS al energizarse. El daemon espera activamente antes de iniciar la captura:

```
[INFO]  [warmup] esperando primer barrido EIS (timeout 150s, poll 10s)
[INFO]  [warmup] registro no disponible aún (Illegal function, 0s)
[INFO]  [warmup] sensor responde, aún procesando sweepCount=0 (20s)
...
[INFO]  [warmup] sensor listo, sweepCount=1, elapsed=110s
```

Si el daemon se reinicia con el sensor ya encendido, el warmup retorna inmediatamente.

---

## Logs

Todos los mensajes incluyen timestamp ISO 8601 y nivel de severidad:

```
[2026-03-28 09:37:53] [INFO]  [ciclo] cb=OPEN fallos=5
[2026-03-28 09:37:53] [WARN]  [cb] OPEN — envío pausado (47 s restantes)
[2026-03-28 09:37:54] [INFO]  [cb] CLOSED — circuito restablecido tras éxito
[2026-03-28 09:37:54] [INFO]  [status] cb=CLOSED fallos=0 pendientes=1916
```

| Nivel | Stream | Cuándo |
|-------|--------|--------|
| `INFO` | stdout | Estado normal: ciclo, CB, FIFO drain |
| `WARN` | stderr | Fallos recuperables: transitorio, OPEN, retry |
| `ERROR` | stderr | Fallos irrecuperables: persistent fail, warmup timeout |

---

## Instalación como servicio systemd

```bash
# Copiar unit file al dispositivo
scp deploy/qw3100-modbus.service qw3100-device:/etc/systemd/system/

# Activar
ssh qw3100-device "systemctl daemon-reload && systemctl enable qw3100-modbus && systemctl start qw3100-modbus"

# Ver logs
ssh qw3100-device "journalctl -u qw3100-modbus -f"
```

Ver [deploy/qw3100-modbus.service](deploy/qw3100-modbus.service) para el unit file completo.

---

## Tests en el PC de desarrollo

```bash
make devlinux
./test/main_test
# All 40 tests passed.
```

---

## Mock server (pruebas sin API real)

```bash
# Responde 200 OK
python3 scripts/mock_server.py

# Simula error de servidor (500)
python3 scripts/mock_server.py --fail 500

# Simula error de autenticación (401)
python3 scripts/mock_server.py --fail 401
```

Configurar `api.base_url` a `http://<IP_HOST>:9090` para apuntar al mock.

---

## Estructura del proyecto

```
QW3100_modbus/
├── src/
│   ├── main.c              — loop principal
│   ├── sensor.c/h          — definición de sensores y snapshot
│   ├── modbus_comm.c/h     — read_block_modbus(), modbus_wait_first_sweep()
│   ├── config.c/h          — AppConfig, carga de JSON y CLI
│   ├── persist.c/h         — FIFO de archivos en /SD/pending/
│   ├── http_sender.c/h     — http_post() via libcurl
│   ├── circuit_breaker.c/h — estados CLOSED/OPEN/HALF_OPEN
│   └── logger.h            — LOG_INFO / LOG_WARN / LOG_ERROR
├── lib/
│   └── cJSON.c/h           — serialización JSON
├── test/
│   └── main_test.c         — 40 tests unitarios
├── deploy/
│   └── qw3100-modbus.service — unit systemd
├── scripts/
│   ├── mock_server.py      — servidor HTTP de prueba
│   └── setup_dependencies_Sensor_Trident.sh
├── docs/                   — documentación extendida
├── Makefile
└── qw3100-config.json      — ejemplo de configuración
```
