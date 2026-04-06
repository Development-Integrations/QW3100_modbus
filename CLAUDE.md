# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Descripción del proyecto

**QW3100 Modbus** es un daemon para Linux embebido ARM escrito en C que:
- Lee datos de sensores QW3100 vía Modbus RTU
- Persiste lecturas localmente como archivos JSON en una cola FIFO
- Envía datos a una API de Scante simulando un gateway Poseidon AP2200
- Implementa circuit breaker + backoff exponencial para resiliencia de red

El dispositivo destino es una placa ARM con Linux embebido. El desarrollo y las pruebas se hacen en Linux ya sea x86 o Raspberry PI 4 B.

## Comandos

### Compilar binario ARM (cross-compile)
```bash
make          # o: make arm
```
Produce `sensor_trident_modbus_ARM` (ELF estático para ARM).

### Compilar y correr pruebas en el PC de desarrollo
```bash
make devlinux       # compila test/main_test en el Linux local
./test/main_test    # ejecuta ~40 pruebas unitarias
```

### Desplegar al dispositivo
```bash
make deploy   # SCP a qw3100-device:/SD/ (requiere alias SSH en ~/.ssh/config)
make run      # deploy + ejecuta vía SSH
```

### Servidor mock para pruebas de integración
```bash
python3 scripts/mock_server.py              # 200 OK
python3 scripts/mock_server.py --fail 500   # 5xx (transitorio)
python3 scripts/mock_server.py --fail 401   # 4xx (persistente)
python3 scripts/mock_server.py --fail timeout
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
| Config | `src/config.c/h` | Carga JSON de configuración + overrides por CLI |
| Persistencia | `src/persist.c/h` | Cola FIFO de archivos en `/SD/pending/` |
| HTTP | `src/http_sender.c/h` | POST único vía libcurl, retorna `HttpResult` |
| Circuit Breaker | `src/circuit_breaker.c/h` | Máquina de estados CLOSED/OPEN/HALF_OPEN |
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

### Circuit breaker

Tres estados con transiciones:
- **CLOSED**: operación normal, envía hasta `fifo_max_per_cycle` (default 10) archivos por ciclo
- **OPEN**: envíos bloqueados; pasa a HALF_OPEN tras expirar `current_timeout`
- **HALF_OPEN**: envía un archivo como sonda; éxito → CLOSED, fallo → OPEN (backoff se duplica)

Backoff: 60s → 120s → 240s → 300s (tope en `cb_backoff_max_sec`).

`HTTP_ERR_TRANSIENT` (timeout, 5xx) incrementa contador de fallos; `HTTP_ERR_PERSISTENT` (4xx) abre el circuito inmediatamente.

### Lógica de reconexión Modbus

`read_block_modbus()` distingue dos modos de fallo:
- **Excepción Modbus** (slave ocupado): reintenta la lectura sin cerrar el puerto
- **Fallo de bus** (timeout/desconexión): cierra el puerto → reconecta → reintenta

Backoff: 2s → 4s → 8s → 16s → 32s en 5 intentos.

### Configuración

Precedencia: defaults compilados → `/SD/qw3100-config.json` → flags CLI `--interval` / `--config`.

Las claves de configuración viven en el struct `AppConfig` (`src/config.h`). El archivo JSON es opcional; todos los campos tienen defaults. Ver `qw3100-config.json` en la raíz del repo como referencia de estructura.

## Entorno de compilación

El Makefile usa estas rutas (se pueden sobreescribir con variables de entorno):
```
PREFIX_MODBUS_ARM      → $HOME/opt/libmodbus-arm      (ARM libmodbus, cross-compile)
PREFIX_CURL_ARM        → $HOME/opt/libcurl-arm        (ARM libcurl, sin SSL, cross-compile)
PREFIX_MODBUS_DEVLINUX → $HOME/opt/libmodbus-devlinux (libmodbus para tests en el PC de desarrollo)
PREFIX_CURL_DEVLINUX   → $HOME/opt/libcurl-devlinux   (libcurl para tests en el PC de desarrollo)
```

Ambas compilaciones (ARM y devlinux) son completamente estáticas (sin dependencias `.so` en runtime).

### Setup de dependencias

Usar el script incluido (maneja arm y devlinux, incluyendo libcurl estática sin dependencias externas):
```bash
./scripts/setup_dependencies_Sensor_Trident.sh arm       # libmodbus + libcurl para ARM
./scripts/setup_dependencies_Sensor_Trident.sh devlinux  # libmodbus + libcurl para el PC de desarrollo
```

## Pruebas

Las pruebas en `test/main_test.c` usan un harness propio simple (sin framework externo). Para agregar pruebas, seguir el patrón existente `=== TEST: Nombre ===` / `[OK]` / `[FAIL]`. Cubren: parseo de sensores, carga de config, FIFO de persistencia, transiciones del circuit breaker, clasificación de errores HTTP y cálculo de backoff.

`test/main_test_JSON.c` es un archivo de exploración/scratch — **no forma parte de `make devlinux`** y no tiene aserciones. No confiar en él.

Nota: `src/circuit_breaker.c` está excluido intencionalmente de `SRCS_TEST`; el circuit breaker se ejercita indirectamente a través de stubs de `http_sender`.

## Documentación de referencia

`docs/` contiene wikis detalladas en español:
- `WIKI_HOME.md` — índice general de la documentación
- `WIKI_ARQUITECTURA.md` — decisiones de diseño y arquitectura en profundidad
- `WIKI_COMPILACION.md` — setup de cross-compilación
- `WIKI_DEPLOYMENT.md` — operación y despliegue en el dispositivo
- `WIKI_TESTING.md` — estrategia de testing y cómo agregar pruebas
- `WIKI_FLUJO_DESARROLLO.md` — flujo de desarrollo y checklist de deploy
- `WIKI_SETUP.md` — configuración del entorno desde cero
- `Requisitos.md` — requisitos funcionales del sistema
