# Deployment

## Envío del binario

```bash
make deploy   # compila ARM + scp al dispositivo
```

Manual:
```bash
scp sensor_trident_modbus_ARM qw3100-device:/SD/
```

---

## Configuración en el dispositivo

Copiar y editar el archivo de configuración:

```bash
scp qw3100-config.json qw3100-device:/SD/
```

Editar en el dispositivo con las credenciales reales:

```bash
ssh qw3100-device
vi /SD/qw3100-config.json
```

```json
{
    "interval_sec": 120,
    "serial_port": "/dev/ttymxc2",
    "slave_id": 1,
    "persist_path": "/SD/pending",

    "api": {
        "enabled": true,
        "base_url": "https://apiv4-integration.productious.com",
        "item_guid": "<item_guid_real>",
        "pull_type_guid": "<pull_type_guid_real>",
        "scante_token": "<token_real>",
        "scante_appid": "<appid_real>",
        "scante_sgid": "<sgid_real>"
    },

    "send": {
        "fifo_max_per_cycle": 10,
        "cb_fail_threshold": 5,
        "cb_open_timeout_sec": 60,
        "cb_backoff_max_sec": 300
    }
}
```

---

## Instalación como servicio systemd

```bash
# Copiar unit file
scp deploy/qw3100-modbus.service qw3100-device:/etc/systemd/system/

# Activar e iniciar
ssh qw3100-device "systemctl daemon-reload && \
                   systemctl enable qw3100-modbus && \
                   systemctl start qw3100-modbus"
```

El unit file del proyecto está en `deploy/qw3100-modbus.service`. El dispositivo puede tener uno propio en `/etc/systemd/system/qw3100.service` — verificar cuál está activo:

```bash
ssh qw3100-device "systemctl list-units | grep qw3100"
```

---

## Operación del servicio

```bash
# Estado
ssh qw3100-device "systemctl status qw3100"

# Logs en tiempo real
ssh qw3100-device "journalctl -u qw3100 -f"

# Logs de las últimas 2 horas
ssh qw3100-device "journalctl -u qw3100 --since '2 hours ago'"

# Reiniciar
ssh qw3100-device "systemctl restart qw3100"

# Detener
ssh qw3100-device "systemctl stop qw3100"
```

---

## Ejecutar manualmente (sin systemd)

```bash
ssh qw3100-device
./sensor_trident_modbus_ARM --config /SD/qw3100-config.json
```

---

## Directorio de pendientes

```bash
# Ver cantidad de archivos acumulados
ssh qw3100-device "ls /SD/pending/ | wc -l"

# Vaciar (si se quiere reiniciar el backlog)
ssh qw3100-device "rm -f /SD/pending/*.json"
```

---

## Notas de producción

- El binario imprime el JSON completo de cada ciclo en stdout — en producción esto va al journal y consume espacio. Considerar redirigir o filtrar con `journalctl --output=short`.
- `interval_sec: 120` es el valor de producción (el sensor actualiza registros cada 120s).
- El directorio `/SD/pending/` no tiene límite de archivos — con 7GB de almacenamiento y 1KB por JSON alcanza para ~26 años de datos sin envío.
