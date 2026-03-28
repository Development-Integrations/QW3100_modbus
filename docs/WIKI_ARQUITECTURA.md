# Arquitectura

## Módulos

```
src/
├── main.c              — loop principal de captura
├── config.c/h          — carga de configuración (JSON + CLI)
├── modbus_comm.c/h     — comunicación Modbus RTU
├── sensor.c/h          — definición de registros y snapshot
├── persist.c/h         — persistencia FIFO en /SD/pending/
├── http_sender.c/h     — envío HTTP POST a API Scante
├── circuit_breaker.c/h — control de reintentos (CB pattern)
└── logger.h            — macros de logging con timestamp
```

---

## Flujo de datos

```
Arranque
  ├── modbus_new_rtu() + modbus_connect()
  └── modbus_wait_first_sweep()   ← bloquea hasta sweepCount > 0

while(1):
  ├── [ciclo] LOG_INFO estado CB
  ├── read_block_modbus(addr=21)  → register_sensor_info[]
  ├── read_block_modbus(addr=79)  → register_sensor_Data[]
  ├── build_sensor_snapshot()     → SensorSnapshot
  ├── build_gateway_payload_json()→ char* JSON
  ├── persist_write()             → /SD/pending/<ts>.json
  ├── try_send_pending()          → HTTP POST + circuit breaker
  ├── [status] LOG_INFO cb=STATE pendientes=N
  └── sleep(interval_sec)
```

---

## Registros Modbus

| Bloque | Dirección | Tamaño | Contenido |
|--------|-----------|--------|-----------|
| Info | 21–32 | 12 words | uptime, sn, fwMajor, fwMinor |
| Data | 79–141 | 63 words | s0–s4 mag/phase/temp, oilTemp, boardTemp, rh |
| sweepCount | 201 | 1 word | Indicador de primer barrido EIS completado |

---

## Circuit breaker — máquina de estados

```
                ┌──────────────────────────────────┐
                │           CLOSED                 │
                │  envía hasta fifo_max_per_cycle  │
                └────────┬─────────────────────────┘
                         │ fail_count >= cb_fail_threshold
                         ▼
                ┌──────────────────────────────────┐
                │            OPEN                  │
                │  pausa envío, captura continúa   │
                │  espera current_timeout segundos │
                └────────┬─────────────────────────┘
                         │ next_retry expiró
                         ▼
                ┌──────────────────────────────────┐
                │          HALF_OPEN               │
                │  envía exactamente 1 archivo     │
                └────┬──────────────┬──────────────┘
                     │ HTTP 200     │ fallo
                     ▼             ▼
                  CLOSED        OPEN (backoff dobla)
```

**Backoff exponencial:** `cb_open_timeout_sec → ×2 → ... → cb_backoff_max_sec`

---

## Clasificación de errores HTTP

| Respuesta | Tipo | Efecto en CB |
|-----------|------|-------------|
| 200 | Éxito | `cb_on_success()` → CLOSED, fail_count=0 |
| 5xx / timeout curl | Transitorio | `cb_on_transient_fail()` → cuenta hacia OPEN |
| 4xx (400/401/403) | Persistente | `cb_on_persistent_fail()` → OPEN inmediato |

---

## Reconexión Modbus

`read_block_modbus()` distingue el tipo de error antes de actuar:

```
errno >= MODBUS_ENOBASE   →  excepción Modbus (slave responde)
                              no cerrar puerto, solo reintentar lectura

errno < MODBUS_ENOBASE    →  timeout / fallo de bus
                              modbus_close() + modbus_connect() + lectura real
```

Backoff de reintentos: `2s → 4s → 8s → 16s → 32s` (5 intentos).
Si agota → retorna -1 → `continue` en el ciclo. El daemon nunca cierra por desconexión del sensor.

---

## Decisiones de diseño

| Decisión | Alternativa descartada | Razón |
|----------|----------------------|-------|
| Loop `while(1)` single-thread | POSIX threads | Sin concurrencia necesaria en ciclo de 120s |
| Archivos JSON como persistencia | SQLite / LevelDB | Simplicidad en embebido minimal |
| libcurl estática sin SSL | Con SSL | El endpoint final usa HTTPS en proxy externo |
| `exit(1)` solo en warmup timeout | Loop infinito | systemd reinicia limpiamente |
| Circuit breaker en cliente | Lógica en servidor | Canal celular — proteger la red |
