# Flujo de desarrollo

## Ciclo típico

```bash
# 1. Cambio en código
vi src/modbus_comm.c

# 2. Tests en el PC de desarrollo
make test

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
chore: mantenimiento (deps, build, config)
refactor: refactoring sin cambio de comportamiento
```

---

## Estructura de ramas

- `master` — rama principal, siempre compilable y funcional

---

## Antes de hacer deploy en producción

- [ ] `make test` pasa todos los tests sin errores
- [ ] `make arm` compila sin errores
- [ ] `ldd sensor_trident_modbus_ARM` → `not a dynamic executable`
- [ ] Configuración en `/SD/qw3100-config.json` con credenciales reales
- [ ] `interval_sec: 120` (no 10 o 12 como en pruebas)
- [ ] Interfaz activa (`api` o `mqtt`) y credenciales correspondientes configuradas
- [ ] Servicio systemd activo: `systemctl status qw3100`

---

## Troubleshooting frecuente

### El binario cierra inmediatamente al arrancar

El sensor no respondió dentro de los 150s de warmup. Causas posibles:
- Cable RS485 desconectado
- `slave_id` incorrecto en config
- `serial_port` incorrecto (`/dev/ttymxc2` en el dispositivo)

### CB nunca pasa a CLOSED (API)

La API no devuelve 200 en el HALF_OPEN. Verificar:
- `api.base_url` alcanzable desde el dispositivo
- Credenciales (`scante_token`, `scante_appid`, etc.) correctas

### CB nunca pasa a CLOSED (MQTT)

- Verificar que los archivos de certificados existen en las rutas configuradas
- Verificar conectividad al broker: `mosquitto_pub -h <broker> -p 8883 --cafile /SD/certs/root-CA.crt ...`
- Si `ca_path` está vacío: conecta sin TLS (solo para pruebas locales)

### Archivos se acumulan en `/SD/pending/` sin enviarse

```bash
# Ver estado del CB
journalctl -u qw3100 | grep status | tail -5

# Ver último error
journalctl -u qw3100 | grep -E "curl error|mqtt|POST OK" | tail -10
```

### "Text file busy" al hacer scp

El binario está corriendo. Detener el servicio antes de reemplazarlo:
```bash
ssh qw3100-device "systemctl stop qw3100"
make deploy
ssh qw3100-device "systemctl start qw3100"
```
