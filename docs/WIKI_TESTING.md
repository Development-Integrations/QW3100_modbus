# Testing

## Tests en el PC de desarrollo (devlinux)

```bash
make devlinux
./test/main_test
# All 40 tests passed.
```

Cubren: parsing de sensores, carga de config, persistencia FIFO, circuit breaker.

---

## Mock server HTTP

Para probar el envío sin API real:

```bash
# Responde 200 OK a todo
python3 scripts/mock_server.py

# Simula error de servidor (5xx → error transitorio → CB cuenta fallo)
python3 scripts/mock_server.py --fail 500

# Simula error de autenticación (4xx → error persistente → CB abre inmediato)
python3 scripts/mock_server.py --fail 401
```

Configurar `api.base_url` en el dispositivo apuntando a la IP del host:
```json
"base_url": "http://192.168.188.64:9090"
```

---

## Escenarios de validación en hardware

### Envío normal (CB CLOSED)
1. Levantar mock server sin flags
2. Arrancar el binario
3. Esperar warmup (~110s)
4. Verificar `[http] POST OK (200)` y archivos eliminados de `/SD/pending/`

### Circuit breaker — apertura por fallos 5xx
1. `python3 scripts/mock_server.py --fail 500`
2. Observar `[WARN] fallo transitorio N/5`
3. Tras 5 fallos: `[WARN] OPEN — reintentará en 60s`
4. Verificar que captura continúa aunque CB esté OPEN

### Circuit breaker — recuperación (HALF_OPEN → CLOSED)
1. Con CB en OPEN, reiniciar mock sin `--fail`
2. Esperar que expire `next_retry`
3. Observar `[INFO] HALF_OPEN — probando conexión`
4. Tras POST OK: `[INFO] CLOSED — circuito restablecido`
5. Verificar drenado del FIFO (5 archivos/ciclo hasta vaciar)

### Backoff exponencial
Con mock en 500, observar la secuencia de timeouts:
```
OPEN — reintentará en 60s
OPEN — reintentará en 120s
OPEN — reintentará en 240s
OPEN — reintentará en 300s   ← tope
```

### Desconexión del sensor durante operación
1. Desconectar físicamente el sensor durante el loop
2. Verificar reintentos con backoff: `Retry 1/5 (wait 2s)...`
3. Reconectar el sensor
4. Verificar `Read OK after N retry(s)` y continuación del ciclo
5. El binario no debe cerrarse en ningún momento

### Warmup al arrancar
1. Apagar y encender el sensor
2. Arrancar el binario inmediatamente
3. Observar la secuencia:
   ```
   [warmup] registro no disponible aún (Illegal function, 0s)
   [warmup] sensor responde, aún procesando sweepCount=0 (Xs)
   [warmup] sensor listo, sweepCount=N, elapsed=XXs
   ```

---

## Interpretar los logs

```
[INFO]  [status] cb=CLOSED fallos=0 pendientes=5   ← fin de ciclo
[WARN]  [cb] fallo transitorio 2/5    ← error recuperable
[WARN]  [cb] OPEN — reintentará en 60 s
[INFO]  [cb] HALF_OPEN — probando conexión
[INFO]  [cb] CLOSED — circuito restablecido tras éxito
[ERROR] [warmup] timeout: sensor no alcanzó sweepCount>0 en 150s
```
