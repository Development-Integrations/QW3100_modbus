## Estado actual del módulo src/

La migración de autenticación Scante → Bearer token está **completada y validada en producción** (HTTP 200 en RC9-930D81-2740).

---

## Estructura de autenticación HTTP actual

`HttpConfig` en `src/http_sender.h`:
- `base_url[256]` — URL completa del endpoint (sin query params)
- `bearer_token[2048]` — JWT para `Authorization: Bearer`
- `ca_bundle_path[256]` — ruta al CA bundle; vacío = bundle del sistema

`http_post()` construye exactamente dos headers:
```
Content-Type: application/json
Authorization: Bearer <bearer_token>
```

---

## Identidad del gateway en el payload JSON

`GatewayConfig` en `src/config.h`:
- `name[32]` — nombre del gateway (ej: `"FLO-W9"`)
- `sn[8]` — serial como string de 4 dígitos (ej: `"0003"`)

`GATEWAY_FW_VERSION` — `#define` en `src/config.h`, no va en config JSON.

En `main()` se construye un `GatewayInfo` (definido en `sensor.h`) copiando desde `cfg.gateway` y el define:
```c
GatewayInfo gw;
strncpy(gw.name, cfg.gateway.name, ...);
strncpy(gw.sn,   cfg.gateway.sn,   ...);
strncpy(gw.fw,   GATEWAY_FW_VERSION, ...);
```

`build_gateway_payload_json()` en `sensor.c` serializa `sn` como **string** (no número).

---

## Lo que NO cambió

- Lógica FIFO, circuit breaker, persistencia, MQTT — sin tocar
- Formato del bloque `data[]` dentro del JSON — sin tocar
- Flags de compilación ARM ni Makefile/meson.build — sin tocar
- Timeouts de libcurl (30s total / 10s conexión) — sin tocar
