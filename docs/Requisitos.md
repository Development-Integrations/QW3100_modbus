# Requisitos de Implementación — Fase 2
**Proyecto:** QW3100 Modbus Gateway  
**Fecha:** 2026-04-07  
**Para:** Claude Code

---

## Estado de implementación

| Bloque | Descripción | Estado |
|--------|-------------|--------|
| BLOQUE 1 | TLS/SSL para API REST | **Completo** (libcurl requiere recompilación con SSL) |
| BLOQUE 2 | Persistencia con directorio `sent/` | **Completo** |
| BLOQUE 3 | Interfaz dual MQTT + API REST con failover | **Completo** |
| BLOQUE 4 | Tests unitarios | **Completo** — 74 OK / 0 FAIL |
| BLOQUE 5 | Makefile — libmosquitto + libssl/libcrypto | **Completo** |

**Orden de implementación sugerido:**
1. **BLOQUE 2** — independiente, no necesita MQTT ni TLS. Buen punto de entrada.
2. **BLOQUE 1** — habilita HTTPS; requiere recompilar libcurl.
3. **BLOQUE 5** + **BLOQUE 3** — agregar libmosquitto al Makefile y luego implementar `mqtt_sender`.
4. **BLOQUE 4** — tests al final, cuando los módulos nuevos estén estables.

---

## Contexto del proyecto

Daemon C embebido (Linux ARM, binario estático) que lee el sensor QW3100 vía Modbus RTU y envía datos a la plataforma Scante. Módulos existentes relevantes:

| Módulo | Archivo | Rol actual |
|--------|---------|-----------|
| Config | `src/config.c/h` | Carga JSON + CLI, struct `AppConfig` |
| Persistencia | `src/persist.c/h` | FIFO de archivos en `/SD/pending/` |
| HTTP sender | `src/http_sender.c/h` | POST vía libcurl, retorna `HttpResult` |
| Circuit breaker | `src/circuit_breaker.c/h` | Estados CLOSED/OPEN/HALF_OPEN |
| Loop principal | `src/main.c` | Orquesta todo en `while(1)` |
| Dependencias | `scripts/setup_dependencies_Sensor_Trident.sh` | Compila libmodbus + libcurl estáticas |

El payload canónico que produce `build_gateway_payload_json()` y que se guarda en `/SD/pending/<ts>.json` es:

```json
{
  "name": "AP2200-Gateway",
  "sn": 100234,
  "fw": "1.20.3",
  "data": [{
    "ts": 1775503676,
    "type": "qw",
    "uptime": 52665,
    "sn": 113901568,
    "fwMajor": 1, "fwMinor": 20, "sweepCount": 541,
    "s0mag": 2186896896, "s0phase": -18.46, "s0TempPre": 29.70, "s0TempPost": 29.66,
    "s1mag": 342745664,  "s1phase": -75.35, "s1TempPre": 29.63, "s1TempPost": 29.63,
    "s2mag": 35639284,   "s2phase": -87.50, "s2TempPre": 29.70, "s2TempPost": 29.73,
    "s3mag": 3708683.75, "s3phase": -88.81, "s3TempPre": 29.73, "s3TempPost": 29.70,
    "s4mag": 38764.26,   "s4phase": -81.57, "s4TempPre": 29.70, "s4TempPost": 29.70,
    "oilTemp": 29.70, "boardTemp": 4645, "rh": 3121
  }]
}
```

---

## BLOQUE 1 — TLS/SSL para API REST

### Objetivo
Recompilar libcurl con soporte OpenSSL para que `http_sender.c` pueda conectarse a endpoints HTTPS de forma nativa, sin depender de proxy externo.

### REQ-1.1 — Script `setup_dependencies_Sensor_Trident.sh`

Actualizar la función de compilación de libcurl para **ambos targets** (`arm` y `devlinux`):

- Agregar instalación de `libssl-dev` y `openssl` en el bloque de dependencias del sistema.
- Reemplazar el flag `--without-ssl` por `--with-openssl` en la invocación de `./configure` de libcurl.
- Para el target `arm`, el script debe detectar los headers de OpenSSL del sistema (`/usr/include/openssl`) y pasarlos como `--with-openssl=/usr` al configure, y usar `CROSS_COMPILE` correctamente.
- Mantener todos los demás flags actuales (`--enable-static`, `--disable-shared`, `--without-libpsl`, etc.).
- La validación final del script (`ldd` / `curl-config --feature`) debe verificar que `SSL` aparezca en los features compilados.
- No romper la compilación existente de libmodbus.

### REQ-1.2 — `src/http_sender.c`

Agregar soporte TLS en la función `http_post()`:

- Activar `CURLOPT_SSL_VERIFYPEER` con valor `1L` (verificación estricta).
- Activar `CURLOPT_SSL_VERIFYHOST` con valor `2L`.
- Agregar `CURLOPT_CAINFO` apuntando a `cfg->ca_bundle_path` (nuevo campo, ver REQ-1.3).
- Si `ca_bundle_path` está vacío (`""`), usar el default del sistema sin setear `CURLOPT_CAINFO` (libcurl lo resuelve solo).
- El comportamiento para HTTP plano (URLs que no empiezan con `https://`) no debe cambiar.

### REQ-1.3 — `src/http_sender.h` y `src/config.h`

Agregar campo al struct `HttpConfig`:

```c
char ca_bundle_path[256];  /* Ruta al CA bundle. Vacío = usar default del sistema */
```

Default: `"/usr/local/share/ca-certificates/"` (configurable vía JSON).

Clave JSON: `api.ca_bundle_path`

### REQ-1.4 — `scripts/mock_server.py`

Actualizar el mock server para soportar HTTPS:

- Agregar argumento `--tls` (flag booleano, por defecto desactivado).
- Cuando `--tls` está activo: generar un certificado autofirmado en memoria con `ssl.wrap_socket()` o equivalente en Python 3, y servir sobre HTTPS en el mismo puerto.
- Cuando `--tls` está inactivo: comportamiento actual sin cambios.
- Documentar en el docstring del archivo cómo generar el certificado de prueba y cómo apuntar el config a él.

---

## BLOQUE 2 — Persistencia con directorios separados

### Objetivo
Mantener un backup de los datos ya enviados en un directorio separado, con rotación por cantidad de archivos.

### Diseño de la operación post-envío exitoso

```
Antes (actual):    HTTP OK → persist_delete(pending/, file)
Después (nuevo):   HTTP OK → persist_move_to_sent(pending/, sent/, file)
                           → persist_rotate_sent(sent/, cfg.sent_retention_count)
```

### REQ-2.1 — `src/persist.h` y `src/persist.c`

Agregar dos funciones nuevas. **No modificar las funciones existentes.**

**Función 1:**
```c
/*
 * Mueve pending_dir/filename a sent_dir/filename usando rename().
 * Crea sent_dir si no existe (un solo nivel).
 * rename() es atómico si ambos directorios están en el mismo filesystem.
 * Retorna 0 en éxito, -1 en error.
 */
int persist_move_to_sent(const char *pending_dir,
                         const char *sent_dir,
                         const char *filename);
```

**Función 2:**
```c
/*
 * Elimina los archivos más antiguos de sent_dir si la cantidad total
 * supera max_files. Usa orden FIFO (lexicográfico = cronológico por nombre ts).
 * Solo elimina el exceso: si hay 1005 y max=1000, elimina los 5 más antiguos.
 * Retorna número de archivos eliminados, -1 en error.
 */
int persist_rotate_sent(const char *sent_dir, int max_files);
```

### REQ-2.2 — `src/config.h` y `src/config.c`

Agregar campos nuevos al struct `AppConfig`:

```c
char     persist_sent_path[128];  /* Directorio para JSONs ya enviados */
```

Y al struct `SendConfig`:

```c
uint16_t sent_retention_count;   /* Máximo de archivos en sent/ antes de rotar */
```

Defaults:
- `persist_sent_path`: `"/SD/sent"`
- `sent_retention_count`: `1000`

Claves JSON:
- `"persist_sent_path"`
- `"send.sent_retention_count"`

Agregar al log de inicio (`[config]`) las líneas:
```
[config] persist_sent_path : /SD/sent
[config] sent_retention    : 1000 archivos
```

### REQ-2.3 — `src/main.c`

En la función `try_send_pending()`, reemplazar la llamada a `persist_delete()` tras un `HTTP_OK` por la secuencia:

```c
persist_move_to_sent(cfg->persist_path, cfg->persist_sent_path, files[i]);
persist_rotate_sent(cfg->persist_sent_path, cfg->send.sent_retention_count);
```

Aplica para **ambas interfaces** (API REST y MQTT, ver Bloque 3).

### REQ-2.4 — `qw3100-config.json`

Agregar los nuevos campos al ejemplo de configuración:

```json
{
  "persist_path":      "/SD/pending",
  "persist_sent_path": "/SD/sent",

  "send": {
    "fifo_max_per_cycle":    10,
    "sent_retention_count":  1000,
    "cb_fail_threshold":     5,
    "cb_open_timeout_sec":   60,
    "cb_backoff_max_sec":    300
  }
}
```

---

## BLOQUE 3 — Interfaz dual MQTT + API REST con failover

### Objetivo
Añadir MQTT como segunda interfaz de envío. La interfaz principal y la de respaldo se definen en config. El failover se activa automáticamente cuando el circuit breaker de la interfaz principal entra en estado `OPEN`.

### Diseño de payload por interfaz

El archivo en `/SD/pending/` siempre contiene el **JSON canónico** (sin modificar). Cada sender aplica su wrapper en memoria antes de transmitir:

**API REST** — envía el JSON canónico directamente (sin cambios respecto al comportamiento actual).

**MQTT** — envuelve el JSON canónico en el formato AWS Device Shadow antes de publicar:

```json
{
  "state": {
    "reported": {
      "name": "AP2200-Gateway",
      "sn": 100234,
      "fw": "1.20.3",
      "data": [{ "ts": 1775503676, "type": "qw", "..." : "..." }]
    }
  },
  "clientToken": "<thing_name>-1775503676",
  "version": 1
}
```

El `clientToken` se construye como `<mqtt.thing_name>-<ts>` donde `ts` es el nombre del archivo sin extensión.

Topic de publicación: `$aws/things/<thing_name>/shadow/update`  
QoS: 1  
Puerto: 8883 (TLS obligatorio)

### REQ-3.1 — Librería MQTT

Usar **libmosquitto** (`libmosquitto-dev`). Detectar la versión instalada en el sistema con:

```bash
mosquitto_pub --version   # o: dpkg -l libmosquitto-dev
```

Para ARM: compilar libmosquitto estáticamente desde fuente (misma estrategia que libmodbus/libcurl en el script de setup). Agregar la compilación de libmosquitto al script `setup_dependencies_Sensor_Trident.sh` para ambos targets.

Variables de entorno nuevas en el Makefile:
```makefile
PREFIX_MOSQUITTO_ARM      ?= $(HOME)/opt/libmosquitto-arm
PREFIX_MOSQUITTO_DEVLINUX ?= $(HOME)/opt/libmosquitto-devlinux
```

Linkear con `-lmosquitto` estático en ambos targets.

### REQ-3.2 — `src/mqtt_sender.h` — NUEVO ARCHIVO

```c
#ifndef MQTT_SENDER_H
#define MQTT_SENDER_H

#include <stdint.h>

typedef struct {
    int     enabled;            /* 0 = skip */
    char    broker_url[256];    /* ej: a2g1ow8hkm0df8-ats.iot.us-east-1.amazonaws.com */
    int     port;               /* default: 8883 */
    char    thing_name[128];    /* nombre del device en AWS IoT */
    char    cert_path[256];     /* /SD/certs/device.crt */
    char    key_path[256];      /* /SD/certs/device.key */
    char    ca_path[256];       /* /SD/certs/root-CA.crt */
    uint32_t connect_timeout_sec; /* timeout de conexión, default: 10 */
    uint32_t keepalive_sec;       /* keepalive MQTT, default: 60 */
} MqttConfig;

typedef enum {
    MQTT_OK = 0,
    MQTT_ERR_CONNECT,     /* no pudo conectar al broker */
    MQTT_ERR_PUBLISH,     /* error al publicar */
    MQTT_ERR_TLS,         /* error de certificados */
    MQTT_ERR_BUILD,       /* error construyendo el payload shadow */
    MQTT_DISABLED         /* mqtt.enabled = 0 */
} MqttResult;

/*
 * Envía canonical_json (JSON canónico del sensor) al broker MQTT de Scante.
 * Construye el wrapper AWS Device Shadow en memoria.
 * Abre conexión, publica, cierra conexión (stateless por ciclo).
 * filename se usa para extraer el ts y construir el clientToken.
 * Retorna MqttResult.
 */
MqttResult mqtt_publish(const MqttConfig *cfg,
                        const char *canonical_json,
                        const char *filename);

#endif /* MQTT_SENDER_H */
```

### REQ-3.3 — `src/mqtt_sender.c` — NUEVO ARCHIVO

Implementar `mqtt_publish()`:

1. Si `!cfg->enabled`, retornar `MQTT_DISABLED`.
2. Parsear `canonical_json` con `cJSON_Parse()`.
3. Construir el JSON shadow en memoria (ver formato arriba). El `clientToken` se forma con `<thing_name>-<basename_sin_extension_de_filename>`.
4. Inicializar cliente mosquitto con `mosquitto_new()`.
5. Configurar TLS con `mosquitto_tls_set(cert_path, key_path, ca_path)`.
6. Conectar con `mosquitto_connect()` usando `connect_timeout_sec`.
7. Publicar con `mosquitto_publish()` QoS=1.
8. Esperar ACK con `mosquitto_loop()` o `mosquitto_loop_forever()` con timeout.
9. Desconectar y destruir el cliente.
10. Liberar memoria cJSON.
11. Mapear errores de mosquitto a `MqttResult`:
    - Error de certificados / TLS → `MQTT_ERR_TLS`
    - Error de conexión → `MQTT_ERR_CONNECT`
    - Error de publish → `MQTT_ERR_PUBLISH`
    - `MOSQ_ERR_SUCCESS` → `MQTT_OK`

### REQ-3.4 — `src/config.h` y `src/config.c`

Agregar al struct `AppConfig`:

```c
MqttConfig   mqtt;          /* Configuración MQTT */
char         primary_interface[8];   /* "api" o "mqtt" */
```

Defaults:
- `primary_interface`: `"api"`
- `mqtt.enabled`: `0`
- `mqtt.port`: `8883`
- `mqtt.connect_timeout_sec`: `10`
- `mqtt.keepalive_sec`: `60`
- `mqtt.broker_url`, `mqtt.thing_name`, `mqtt.cert_path`, `mqtt.key_path`, `mqtt.ca_path`: strings vacíos

Claves JSON del bloque `"mqtt"`:

```json
{
  "web_interface": {
    "primary": "api"
  },
  "mqtt": {
    "enabled":              false,
    "broker_url":           "a2g1ow8hkm0df8-ats.iot.us-east-1.amazonaws.com",
    "port":                 8883,
    "thing_name":           "<thing_name>",
    "cert_path":            "/SD/certs/device.crt",
    "key_path":             "/SD/certs/device.key",
    "ca_path":              "/SD/certs/root-CA.crt",
    "connect_timeout_sec":  10,
    "keepalive_sec":        60
  }
}
```

Agregar al log de inicio:
```
[config] primary_interface : api
[config] mqtt.enabled      : false
[config] mqtt.broker_url   : ...
```

### REQ-3.5 — `src/main.c` — Circuit breakers independientes

Declarar **dos** instancias de `CircuitBreaker` en `main()`:

```c
CircuitBreaker cb_api;
CircuitBreaker cb_mqtt;
cb_init(&cb_api);
cb_init(&cb_mqtt);
```

### REQ-3.6 — `src/main.c` — Lógica de envío con failover

Reemplazar la función `try_send_pending()` con la nueva lógica de selección de interfaz:

```
Determinar interfaz activa:
  si primary == "api":
    si cb_api.state != OPEN  → usar API  (cb = &cb_api)
    si cb_api.state == OPEN  → usar MQTT (cb = &cb_mqtt)  [failover]
  si primary == "mqtt":
    si cb_mqtt.state != OPEN → usar MQTT (cb = &cb_mqtt)
    si cb_mqtt.state == OPEN → usar API  (cb = &cb_api)   [failover]

Para cada archivo en pending (hasta fifo_max_per_cycle):
  leer canonical_json desde pending/
  si interfaz activa == "api":
      result = http_post(&cfg->api, canonical_json)
      mapear HttpResult → éxito/transitorio/persistente
  si interfaz activa == "mqtt":
      result = mqtt_publish(&cfg->mqtt, canonical_json, filename)
      mapear MqttResult → éxito/transitorio/persistente

  si éxito:
      persist_move_to_sent(pending/, sent/, filename)
      persist_rotate_sent(sent/, sent_retention_count)
      cb_on_success(cb)
  si transitorio:
      cb_on_transient_fail(cb, ...)
      break
  si persistente:
      cb_on_persistent_fail(cb, ...)
      break
```

Mapeo de `MqttResult` para el circuit breaker:
- `MQTT_OK` → éxito
- `MQTT_ERR_CONNECT`, `MQTT_ERR_PUBLISH`, `MQTT_ERR_TLS` → transitorio
- `MQTT_DISABLED` → no cuenta (skip silencioso)
- `MQTT_ERR_BUILD` → persistente (datos corruptos, no tiene sentido reintentar)

### REQ-3.7 — Log de estado

La línea `[status]` existente debe ampliarse para mostrar ambos CBs:

```
[status] primary=api  cb_api=CLOSED  cb_mqtt=CLOSED  fallos_api=0  fallos_mqtt=0  pendientes=5
```

### REQ-3.8 — `qw3100-config.json`

Agregar los bloques nuevos al archivo de ejemplo de configuración en la raíz del repo:

```json
{
  "interval_sec": 120,
  "serial_port": "/dev/ttymxc2",
  "slave_id": 1,
  "persist_path":      "/SD/pending",
  "persist_sent_path": "/SD/sent",

  "web_interface": {
    "primary": "api"
  },

  "api": {
    "enabled":        true,
    "base_url":       "https://apiv4-integration.productious.com",
    "item_guid":      "<item_guid>",
    "pull_type_guid": "<pull_type_guid>",
    "scante_token":   "<token>",
    "scante_appid":   "<appid>",
    "scante_sgid":    "<sgid>",
    "ca_bundle_path": ""
  },

  "mqtt": {
    "enabled":             false,
    "broker_url":          "a2g1ow8hkm0df8-ats.iot.us-east-1.amazonaws.com",
    "port":                8883,
    "thing_name":          "<thing_name>",
    "cert_path":           "/SD/certs/device.crt",
    "key_path":            "/SD/certs/device.key",
    "ca_path":             "/SD/certs/root-CA.crt",
    "connect_timeout_sec": 10,
    "keepalive_sec":       60
  },

  "send": {
    "fifo_max_per_cycle":   10,
    "sent_retention_count": 1000,
    "cb_fail_threshold":    5,
    "cb_open_timeout_sec":  60,
    "cb_backoff_max_sec":   300
  }
}
```

---

## BLOQUE 4 — Tests unitarios

### REQ-4.1 — `test/main_test.c`

Agregar casos de prueba para los módulos nuevos. Seguir el patrón existente `=== TEST: Nombre ===` / `[OK]` / `[FAIL]`.

**Tests para `persist.c` (nuevas funciones):**
- `persist_move_to_sent`: mueve archivo correctamente entre directorios.
- `persist_rotate_sent`: elimina solo el exceso cuando se supera `max_files`.
- `persist_rotate_sent`: no elimina nada si `count <= max_files`.

**Tests para `config.c` (nuevos campos):**
- Parseo correcto de `persist_sent_path` desde JSON.
- Parseo correcto de `send.sent_retention_count` desde JSON.
- Parseo correcto del bloque `mqtt` completo desde JSON.
- Parseo correcto de `web_interface.primary` desde JSON.
- Defaults correctos cuando los campos no están en el JSON.

**Tests para `mqtt_sender.c` (con stub):**
- `mqtt_publish` retorna `MQTT_DISABLED` cuando `cfg->enabled = 0`.
- `mqtt_publish` retorna `MQTT_ERR_BUILD` cuando el JSON canónico es inválido.
- El wrapper shadow se construye correctamente (verificar campos `state.reported`, `clientToken`, `version`).

### REQ-4.2 — Meta de tests

El total de tests debe seguir pasando al 100%. Los tests nuevos se agregan sin modificar los existentes. Todos los tests nuevos deben poder ejecutarse con `make devlinux && ./test/main_test`.

---

## BLOQUE 5 — Makefile

### REQ-5.1

Agregar `src/mqtt_sender.c` a `SRCS_ARM` y `SRCS_TEST`.

Agregar las variables de prefijo para libmosquitto:

```makefile
PREFIX_MOSQUITTO_ARM      ?= $(HOME)/opt/libmosquitto-arm
PREFIX_MOSQUITTO_DEVLINUX ?= $(HOME)/opt/libmosquitto-devlinux
```

Agregar a los flags de compilación:

```makefile
# ARM
-I$(PREFIX_MOSQUITTO_ARM)/include
$(PREFIX_MOSQUITTO_ARM)/lib/libmosquitto.a

# devlinux
-I$(PREFIX_MOSQUITTO_DEVLINUX)/include
$(PREFIX_MOSQUITTO_DEVLINUX)/lib/libmosquitto.a
```

Agregar `-lssl -lcrypto` al linkeo de ambos targets (requerido por libcurl con OpenSSL y por libmosquitto).

---

## Restricciones generales

- **Sin threads.** El binario es single-thread. La conexión MQTT se abre y cierra en cada ciclo (stateless), igual que libcurl hoy.
- **Compilación completamente estática.** `ldd sensor_trident_modbus_ARM` debe seguir retornando `not a dynamic executable`.
- **No romper los 40 tests existentes.** Cualquier cambio en `persist.c`, `config.c` o `main.c` debe mantener compatibilidad con los tests actuales.
- **Precedencia de configuración sin cambios:** defaults → JSON → CLI.
- **El bloque `api` es retrocompatible.** Un config que no tenga `ca_bundle_path` debe seguir funcionando con el default.
- **Si `mqtt.enabled = false`**, el binario no intenta conectar al broker bajo ninguna circunstancia, aunque sea el failover.
- **Logging:** usar las macros existentes `LOG_INFO` / `LOG_WARN` / `LOG_ERROR` de `logger.h`. No usar `printf` directamente en código nuevo (excepto donde ya existe en el código actual).

---

## Archivos a crear o modificar

| Archivo | Acción |
|---------|--------|
| `scripts/setup_dependencies_Sensor_Trident.sh` | Modificar — agregar OpenSSL + libmosquitto |
| `src/http_sender.h` | Modificar — agregar `ca_bundle_path` a `HttpConfig` |
| `src/http_sender.c` | Modificar — agregar `CURLOPT_SSL_VERIFYPEER/HOST/CAINFO` |
| `src/config.h` | Modificar — agregar `persist_sent_path`, `sent_retention_count`, `MqttConfig`, `primary_interface` |
| `src/config.c` | Modificar — parsear y loggear nuevos campos |
| `src/persist.h` | Modificar — declarar `persist_move_to_sent()` y `persist_rotate_sent()` |
| `src/persist.c` | Modificar — implementar las dos funciones nuevas |
| `src/mqtt_sender.h` | **Crear** |
| `src/mqtt_sender.c` | **Crear** |
| `src/main.c` | Modificar — dos CBs, lógica failover, nueva línea de status |
| `qw3100-config.json` | Modificar — agregar todos los campos nuevos |
| `scripts/mock_server.py` | Modificar — agregar flag `--tls` |
| `Makefile` | Modificar — agregar libmosquitto y libssl/libcrypto |
| `test/main_test.c` | Modificar — agregar nuevos casos de prueba |