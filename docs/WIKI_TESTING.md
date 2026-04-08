# Testing

## Tests unitarios (devlinux)

```bash
make test
# o directamente:
meson setup build-devlinux
meson test -C build-devlinux --print-errorlogs
```

Cubren: parsing de sensores, carga de config, persistencia FIFO, circuit breaker,
clasificación de errores HTTP, transiciones CB, MQTT disabled/invalid.

---

## Mock server HTTP

Para probar el envío a la API sin servidor real:

```bash
# Responde 200 OK a todo
python3 scripts/mock_server.py

# Simula error de servidor (5xx → error transitorio → CB cuenta fallo)
python3 scripts/mock_server.py --fail 500

# Simula error de autenticación (4xx → error persistente → CB abre inmediato)
python3 scripts/mock_server.py --fail 401

# Simula timeout
python3 scripts/mock_server.py --fail timeout

# Con TLS (genera cert autofirmado)
python3 scripts/mock_server.py --tls
# → imprime ruta del cert para poner en api.ca_bundle_path
```

Configurar en el dispositivo:
```json
"api": { "base_url": "http://192.168.188.64:9090" }
```

---

## Broker MQTT local (Mosquitto)

Para probar MQTT sin AWS IoT:

```bash
# Instalar
sudo apt install mosquitto mosquitto-clients

# Configurar para aceptar conexiones externas sin TLS
sudo tee /etc/mosquitto/conf.d/local-test.conf <<EOF
listener 1883 0.0.0.0
allow_anonymous true
EOF
sudo systemctl restart mosquitto

# Suscribirse (# no captura topics $aws/..., usar la forma explícita)
mosquitto_sub -h localhost -t '$aws/things/#' -v
```

Configurar en el dispositivo:
```json
"web_interface": { "primary": "mqtt" },
"mqtt": {
    "enabled":    true,
    "broker_url": "192.168.188.64",
    "port":       1883,
    "thing_name": "qw3100-test",
    "cert_path":  "",
    "key_path":   "",
    "ca_path":    ""
}
```

Con `ca_path` vacío el daemon conecta sin TLS (modo prueba local).

---

## Escenarios de validación en hardware

### Envío normal (CB CLOSED)
1. Levantar mock server o broker local
2. Arrancar el binario
3. Verificar `[mqtt] publicado OK` o `[http] POST OK` y archivos en `sent/`

### Circuit breaker — apertura por fallos
1. Detener el servidor/broker
2. Observar `[WARN] fallo transitorio N/5`
3. Tras 5 fallos: `[WARN] OPEN — reintentará en Xs`
4. Verificar que la captura continúa aunque CB esté OPEN

### Circuit breaker — recuperación (HALF_OPEN → CLOSED)
1. Con CB en OPEN, reiniciar el servidor
2. Esperar que expire `next_retry`
3. Observar `[INFO] HALF_OPEN — probando conexión`
4. Tras éxito: `[INFO] CLOSED — circuito restablecido`
5. Verificar drenado del FIFO

### Failover API → MQTT
1. Configurar `primary: api` con `mqtt.enabled: true`
2. Detener el servidor HTTP
3. Tras 5 fallos API: CB API abre → failover automático a MQTT
4. Observar `[status] cb_api=OPEN cb_mqtt=CLOSED`

### Backoff exponencial
Con servidor caído, observar la secuencia de timeouts:
```
OPEN — reintentará en 15s
OPEN — reintentará en 30s
OPEN — reintentará en 60s  ← tope (cb_backoff_max_sec)
```

### Desconexión del sensor durante operación
1. Desconectar físicamente el sensor
2. Verificar reintentos con backoff: `Retry 1/5 (wait 2s)...`
3. Reconectar el sensor
4. Verificar `Read OK after N retry(s)` y continuación del ciclo

---

## Interpretar los logs

```
[INFO]  [status] primary=api  cb_api=CLOSED  cb_mqtt=CLOSED  fallos_api=0  pendientes=0
[WARN]  [cb] fallo transitorio 2/5
[WARN]  [cb] OPEN — reintentará en 15 s
[INFO]  [cb] HALF_OPEN — probando conexión
[INFO]  [cb] CLOSED — circuito restablecido tras éxito
[WARN]  [mqtt] ca_path vacío — conectando sin TLS
[INFO]  [mqtt] publicado OK → $aws/things/qw3100-test/shadow/update
[INFO]  [fifo] enviado(mqtt) → sent/: 1234567890.json
```
