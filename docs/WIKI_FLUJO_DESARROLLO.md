# Flujo de desarrollo

## Ciclo típico

```bash
# 1. Cambio en código
vi src/modbus_comm.c

# 2. Tests nativos
make native && ./test/main_test

# 3. Compilar ARM
make arm

# 4. Deploy y prueba en hardware
make deploy
ssh qw3100-device "journalctl -u qw3100 -f"

# 5. Commit
git add src/modbus_comm.c
git commit -m "fix: descripción del cambio"
git push
```

---

## Convenciones de commit

```
feat:  nueva funcionalidad
fix:   corrección de bug
docs:  cambios en documentación
test:  cambios en tests
refactor: refactoring sin cambio de comportamiento
```

Ejemplos del proyecto:
```
feat: reconexión verificada, warmup EIS y observabilidad de ciclo
fix: agregar http_sender.c a SRCS_TEST en Makefile
feat: Fase 5 — Makefile, systemd unit y observabilidad
```

---

## Estructura de ramas

- `master` — rama principal, siempre compilable y funcional

---

## Antes de hacer deploy en producción

- [ ] `make native && ./test/main_test` pasa los 40 tests
- [ ] `make arm` compila sin errores
- [ ] Configuración en `/SD/qw3100-config.json` con credenciales reales
- [ ] `interval_sec: 120` (no 10 como en pruebas)
- [ ] `api.enabled: true`
- [ ] Servicio systemd activo: `systemctl status qw3100`

---

## Troubleshooting frecuente

### El binario cierra inmediatamente al arrancar

El sensor no respondió dentro de los 150s de warmup. Causas posibles:
- Cable RS485 desconectado
- `slave_id` incorrecto en config
- `serial_port` incorrecto (`/dev/ttymxc2` en el dispositivo)

### CB nunca pasa a CLOSED

La API no está devolviendo 200 en el HALF_OPEN. Verificar:
- `api.base_url` alcanzable desde el dispositivo
- Credenciales (`scante_token`, `scante_appid`, etc.) correctas
- `curl -X POST <base_url>/IOTData?...` manual desde el dispositivo

### Archivos se acumulan en `/SD/pending/` sin enviarse

```bash
# Ver estado del CB
journalctl -u qw3100 | grep status | tail -5

# Ver último error HTTP
journalctl -u qw3100 | grep "curl error\|POST OK" | tail -10
```

### "Text file busy" al hacer scp

El binario está corriendo. Detener el servicio antes de reemplazarlo:
```bash
ssh qw3100-device "systemctl stop qw3100"
make deploy
ssh qw3100-device "systemctl start qw3100"
```
