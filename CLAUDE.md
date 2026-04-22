# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Descripción del proyecto

**QW3100 Modbus** es un daemon para Linux embebido ARM escrito en C que:
- Lee datos de sensores QW3100 vía Modbus RTU
- Persiste lecturas localmente como archivos JSON en una cola FIFO
- Envía datos a la API REST de fleet.nebulae.com.co (HTTP Bearer token) o AWS IoT Core (MQTT) con failover automático
- Implementa circuit breaker + backoff exponencial para resiliencia de red

El dispositivo destino es una placa ARM con Linux embebido. El desarrollo y las pruebas se hacen en Linux x86 o Raspberry Pi 4B.

## Comandos

### Compilar binario ARM (cross-compile)
```bash
make          # o: make arm
```
Produce `sensor_trident_modbus_ARM` (ELF estático para ARM). Requiere `third_party/arm/` con las dependencias precompiladas.

### Compilar y correr pruebas en el PC de desarrollo
```bash
make test       # compila y ejecuta tests (usa paquetes del sistema)
make devlinux   # solo compila sin ejecutar
```

No hay forma de correr un test individual: todos los tests viven en un único binario (`test/main_test.c`) y se ejecutan juntos. Para aislar un fallo, buscar el prefijo `=== TEST: Nombre ===` en la salida.

### Desplegar al dispositivo
```bash
make deploy   # SCP a FLO-W9-YYYY:/SD/ (requiere alias SSH en ~/.ssh/config)
make run      # deploy + ejecuta vía SSH
```

### Herramienta de diagnóstico TLS/curl (`probe_env`)
```bash
make probe          # compila para devlinux y ejecuta POST real al endpoint
make probe-arm      # cross-compila para ARM
make probe-deploy   # probe-arm + scp al dispositivo
```
Valida las librerías estáticas ARM: DNS (c-ares), TLS (OpenSSL), CA bundle y conectividad HTTP. Si sale `HTTP 200 — OK`, las `.a` compiladas son correctas. Credenciales y URL hardcodeadas en `tools/probe_env_config.h`.

### Servidor mock HTTP para pruebas
```bash
python3 scripts/mock_server.py              # 200 OK
python3 scripts/mock_server.py --fail 500   # 5xx (transitorio)
python3 scripts/mock_server.py --fail 401   # 4xx (persistente)
python3 scripts/mock_server.py --tls        # con TLS autofirmado
```

### Broker MQTT local para pruebas
```bash
# Suscribirse (# no captura topics que empiezan con $)
mosquitto_sub -h localhost -t '$aws/things/#' -v
```

### Limpiar
```bash
make clean
```

## Arquitectura

### Responsabilidad de cada módulo

| Módulo | Archivo | Rol |
|--------|---------|-----|
| Bucle principal | `src/main.c` | Orquesta todos los módulos en un ciclo `while(1)` |
| Sensor | `src/sensor.c/h` | Definiciones de registros, construcción de snapshots, serialización JSON |
| Modbus | `src/modbus_comm.c/h` | Lectura RTU con lógica de reconexión y backoff |
| Config | `src/config.c/h` | Carga JSON de configuración + overrides por CLI. Define `GatewayConfig` (name/sn) y `GATEWAY_FW_VERSION` |
| Persistencia | `src/persist.c/h` | Cola FIFO en `/SD/pending/`, directorio `sent/` con rotación |
| HTTP | `src/http_sender.c/h` | POST a `base_url` con `Authorization: Bearer <bearer_token>`, retorna `HttpResult`. `http_global_init()` **debe llamarse una vez antes del loop** (inicializa OpenSSL estático) |
| MQTT | `src/mqtt_sender.c/h` | Publica AWS Device Shadow vía libmosquitto con TLS/mTLS |
| Circuit Breaker | `src/circuit_breaker.c/h` | Máquina de estados CLOSED/OPEN/HALF_OPEN (una instancia por interfaz) |
| Logger | `src/logger.h` | Macros con timestamp — INFO→stdout, WARN/ERROR→stderr |
| cJSON | `lib/cJSON.c/h` | Biblioteca JSON incluida en el repo |

### Flujo del bucle principal

```
config_init() → config_load_file() → config_apply_cli()
modbus_new_rtu() → modbus_connect()
modbus_wait_first_sweep()  ← espera sweepCount (addr 201) > 0, máx 150s

while(1):
  read_block_modbus(addr=21, 12 regs)   ← info: uptime, FW, serial
  read_block_modbus(addr=79, 63 regs)   ← datos: mag, fase, temp, HR para S0–S4
  build_sensor_snapshot()
  build_gateway_payload_json()          ← cJSON, simula AP2200
  persist_write()                       ← /SD/pending/<timestamp>.json
  try_send_pending()                    ← drena FIFO, controlado por CB
  sleep(interval_sec)
```

### Selección de interfaz y failover

`select_interface()` en `main.c` determina qué interfaz usar:
- Si `primary=api` y `cb_api` no está OPEN → usa API
- Si `primary=api` y `cb_api` está OPEN y MQTT está habilitado → failover a MQTT
- Si `primary=mqtt` y `cb_mqtt` está OPEN y API está habilitada → failover a API
- Hay dos instancias de `CircuitBreaker` independientes: `cb_api` y `cb_mqtt`

### Circuit breaker

Tres estados con transiciones:
- **CLOSED**: operación normal, envía hasta `fifo_max_per_cycle` (default 10) archivos por ciclo
- **OPEN**: envíos bloqueados; pasa a HALF_OPEN al expirar `current_timeout`
- **HALF_OPEN**: envía un archivo como sonda; éxito → CLOSED, fallo → OPEN (backoff se duplica)

`HTTP_ERR_TRANSIENT`/`MQTT_ERR_CONNECT` (timeout, 5xx) incrementan contador; errores persistentes (4xx, `MQTT_ERR_BUILD`) abren el circuito inmediatamente.

`cb_maybe_recover(cb)` — tíquea silenciosamente el CB primario cada ciclo mientras se envía por el failover. Permite la transición OPEN→HALF_OPEN sin llamar a `cb_allow_send()` (que generaría logs ruidosos de "envío pausado").

### MQTT — formato del payload

`mqtt_sender.c` envía al topic `$aws/things/<thing_name>/shadow/update` con formato AWS Device Shadow:
```json
{
  "state": { "reported": <payload_canónico_gateway> },
  "clientToken": "<thing_name>-<timestamp>",
  "version": 1
}
```
Si `ca_path` está vacío conecta sin TLS (modo prueba). Con `ca_path` configurado activa TLS; `cert_path`/`key_path` son opcionales (mTLS).

### Lógica de reconexión Modbus

`read_block_modbus()` distingue dos modos de fallo:
- **Excepción Modbus** (slave ocupado): reintenta la lectura sin cerrar el puerto
- **Fallo de bus** (timeout/desconexión): cierra el puerto → reconecta → reintenta

Backoff: 2s → 4s → 8s → 16s → 32s en 5 intentos.

### Configuración

Precedencia: defaults compilados → `/SD/qw3100-config.json` → flags CLI `--interval` / `--config`.

Las claves de configuración viven en el struct `AppConfig` (`src/config.h`). El archivo JSON es opcional; todos los campos tienen defaults. Ver `qw3100-config.json` en la raíz del repo como referencia de estructura.

Bloques JSON del archivo de configuración:
- `gateway`: `name` (string) y `sn` (serial 4 dígitos como string). Defaults: `"FLO-W9"` / `"0000"`. La versión de firmware no va en config — se define con `GATEWAY_FW_VERSION` en `src/config.h`.
- `api`: `enabled`, `base_url` (URL completa del endpoint), `bearer_token` (JWT), `ca_bundle_path`.
- `mqtt`: configuración AWS IoT Core (broker, certs, thing_name).
- `send`: parámetros de circuit breaker y FIFO.

## Entorno de compilación

### Build system

El proyecto usa **Meson** con un Makefile como wrapper. El cross-file está en `cross/armv7.ini`.

```bash
apt install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf meson ninja-build
```

### Dependencias ARM

Las dependencias ARM se compilan desde fuente una sola vez (no se versionan — `third_party/` está en `.gitignore`):

```bash
./scripts/build_third_party.sh   # OpenSSL → c-ares → libmodbus → libcurl → libmosquitto
```

Resultado en `third_party/arm/{openssl,cares,modbus,curl,mosquitto}/`.

Flags críticos de OpenSSL: `no-shared no-engine no-dso` — sin `no-dso` el binario estático crashea en ARM al intentar hacer `dlopen` de providers en runtime. libcurl se compila con `--enable-ares` (c-ares) para DNS sin glibc NSS, que también crashea en binarios estáticos ARM.

### Dependencias devlinux

Para tests en el PC de desarrollo se usan paquetes del sistema:
```bash
apt install libmodbus-dev libcurl4-openssl-dev libmosquitto-dev libssl-dev
```

Ambas compilaciones (ARM y devlinux) son completamente estáticas (sin dependencias `.so` en runtime).

### CA bundle en el dispositivo

El `/etc/ssl/certs/ca-certificates.crt` del dispositivo está desactualizado (no incluye Let's Encrypt R13). El bundle correcto está en:
```
/usr/local/share/ca-certificates/roots.pem       # bundle completo
/usr/local/share/ca-certificates/isrgrootx1.pem  # ISRG Root X1 — cadena LE R13
```
El campo `ca_bundle_path` en `qw3100-config.json` debe apuntar a una de estas rutas. Para el daemon en producción:
```json
"ca_bundle_path": "/usr/local/share/ca-certificates/isrgrootx1.pem"
```

## Pruebas

Las pruebas en `test/main_test.c` usan un harness propio simple (sin framework externo). Para agregar pruebas, seguir el patrón existente `=== TEST: Nombre ===` / `[OK]` / `[FAIL]`. Cubren: parseo de sensores, carga de config, FIFO de persistencia, directorio sent/, transiciones del circuit breaker, clasificación de errores HTTP, configuración y early-returns de MQTT.

`test/main_test_JSON.c` es un archivo de exploración/scratch — **no forma parte de `make test`** y no tiene aserciones.

Nota: `src/circuit_breaker.c` está excluido intencionalmente de los sources de test; el circuit breaker se ejercita indirectamente a través de stubs de `http_sender`.

## Documentación de referencia

`docs/` contiene wikis detalladas en español:
- `WIKI_HOME.md` — índice general y estado del proyecto
- `WIKI_ARQUITECTURA.md` — decisiones de diseño y arquitectura en profundidad
- `WIKI_COMPILACION.md` — comandos de compilación con Meson/Makefile
- `WIKI_DEPLOYMENT.md` — operación y despliegue en el dispositivo
- `WIKI_TESTING.md` — tests unitarios, mock server HTTP y MQTT, escenarios de validación
- `WIKI_FLUJO_DESARROLLO.md` — flujo de desarrollo y checklist de deploy
- `WIKI_SETUP.md` — configuración del entorno desde cero (deps ARM y devlinux)
- `Requisitos.md` — requisitos de Fase 2 (todos implementados)
