# Deployment al Dispositivo

## 📤 Flujo General

```
Binario compilado (sensor_trident_modbus_ARM)
         ↓
    Validar/Testar localmente
         ↓
    Enviar por SCP
         ↓
    Ejecutar en dispositivo
         ↓
    Monitorear output
```

## 🚀 Deployment Rápido (3 pasos)

### Opción 1: VS Code Tasks (Integrado - Recomendado)

```
Cmd+Shift+B → "2. Enviar por SCP"
   (esperar a que complete)
```

```
Cmd+Shift+B → "3. Ejecutar en Dispositivo"
```

Las tareas están encadenadas con `dependsOn`, se ejecutan en orden.

### Opción 2: Línea de Comando Manual

```bash
# Step 1: Verificar binario
ls -la sensor_trident_modbus_ARM

# Step 2: Enviar por SCP
scp -P 2122 sensor_trident_modbus_ARM root@192.168.188.39:/SD/

# Step 3: SSH y ejecutar
ssh -p 2122 root@192.168.188.39 /SD/sensor_trident_modbus_ARM
```

## 📡 Protocolo SCP (Envío)

### Configuración

| Parámetro | Valor | Ubicación |
|-----------|-------|-----------|
| Puerto SSH | 2122 | `.vscode/tasks.json` |
| Host | 192.168.188.39 | `.vscode/tasks.json` |
| Usuario | root | `.vscode/tasks.json` |
| Destino | /SD/ | `.vscode/tasks.json` |

### Comando Detallado

```bash
scp -P 2122 \
    sensor_trident_modbus_ARM \
    root@192.168.188.39:/SD/
```

### Verificar Envío

```bash
# Conectar a dispositivo y verificar
ssh -p 2122 root@192.168.188.39

# En el dispositivo remoto:
ls -la /SD/sensor_trident_modbus_ARM
file /SD/sensor_trident_modbus_ARM
```

## 🎯 Ejecución Remota

### Opción 1: SSH Interactivo

```bash
ssh -p 2122 root@192.168.188.39

# En el dispositivo:
cd /SD
./sensor_trident_modbus_ARM --interval 5
```

### Opción 2: SSH No-interactivo (directamente)

```bash
ssh -p 2122 root@192.168.188.39 "/SD/sensor_trident_modbus_ARM --interval 5"
```

### Opción 3: Terminal Integrada (VS Code)

```
Cmd+Shift+B → "3. Ejecutar en Dispositivo"
```

Abre una terminal (terminator) con conexión remota.

## 📊 Parámetros de Ejecución

El binario acepta argumentos:

```bash
./sensor_trident_modbus_ARM [OPCIÓN]
```

### Opciones

| Opción | Alias | Descripción | Ejemplo |
|--------|-------|-------------|---------|
| `--interval` | `-i` | Intervalo polling (seg) | `-i 10` (cada 10 seg) |

### Ejemplos

```bash
# Default (5 segundos)
./sensor_trident_modbus_ARM

# Cada 2 segundos
./sensor_trident_modbus_ARM --interval 2

# Cada 30 segundos
./sensor_trident_modbus_ARM -i 30
```

## 📥 Capturar Output

### Opción 1: En Terminal SSH

```bash
ssh -p 2122 root@192.168.188.39 "/SD/sensor_trident_modbus_ARM -i 5" 

# Salida típica (JSON o estructurada)
```

### Opción 2: Redirigir a Archivo Remoto

```bash
ssh -p 2122 root@192.168.188.39 \
    "/SD/sensor_trident_modbus_ARM -i 5 > /SD/output.log 2>&1 &"

# Descargar log después
scp -P 2122 root@192.168.188.39:/SD/output.log ./output.log
```

### Opción 3: Redirigir a Local

```bash
ssh -p 2122 root@192.168.188.39 "/SD/sensor_trident_modbus_ARM -i 5" | \
    tee local_output.log
```

## 🔗 Ejecución en Background (Daemon)

Si deseas que el binario corra continuamente:

```bash
# Iniciar en background
ssh -p 2122 root@192.168.188.39 \
    "nohup /SD/sensor_trident_modbus_ARM -i 60 > /SD/sensor.log 2>&1 &"

# Verificar que está corriendo
ssh -p 2122 root@192.168.188.39 "ps aux | grep sensor_trident"

# Detener
ssh -p 2122 root@192.168.188.39 "pkill sensor_trident"
```

## 🛠️ Script de Deployment Automatizado

Crear archivo `deploy.sh`:

```bash
#!/bin/bash

set -e

REMOTE_HOST="192.168.188.39"
REMOTE_PORT="2122"
REMOTE_USER="root"
REMOTE_PATH="/SD"
BINARY="sensor_trident_modbus_ARM"
INTERVAL="${1:-5}"

echo "🔨 Compilando ARM..."
task "1. Compilar ARM" || arm-linux-gnueabihf-gcc \
    ./src/*.c ./lib/cJSON.c \
    -o "$BINARY" -static \
    -I$PREFIX_ARM/include/modbus \
    $PREFIX_ARM/lib/libmodbus.a

echo "📤 Enviando por SCP..."
scp -P "$REMOTE_PORT" "$BINARY" "$REMOTE_USER@$REMOTE_HOST:$REMOTE_PATH/"

echo "🚀 Ejecutando en dispositivo..."
ssh -p "$REMOTE_PORT" "$REMOTE_USER@$REMOTE_HOST" \
    "$REMOTE_PATH/$BINARY --interval $INTERVAL"

echo "✅ Done!"
```

Uso:
```bash
chmod +x deploy.sh
./deploy.sh 10  # interval de 10 seg
```

## 📋 Checklist de Deployment

- [ ] Binario compilado: `ls -la sensor_trident_modbus_ARM`
- [ ] SSH accesible: `ssh -p 2122 root@192.168.188.39 'echo OK'`
- [ ] Directorio remoto existe: `ssh ... 'ls /SD'`
- [ ] SCP funciona: `scp -P 2122 sensor_trident_modbus_ARM ...`
- [ ] Binario ejecutable en remoto: `ssh ... '/SD/sensor_trident_modbus_ARM --help'`

## 🐛 Troubleshooting

### "Connection refused"
```bash
# Verificar dispositivo está encendido
ping 192.168.188.39

# Verificar SSH accesible
ssh -p 2122 root@192.168.188.39 'echo OK'
```

### "Permission denied (publickey)"
```bash
# Verificar credenciales o SSH keys
# Si no necesitas key, conectar:
ssh -p 2122 root@192.168.188.39  # Pide contraseña
```

### "Permission denied" en archivo binario
```bash
# Hacer binario ejecutable
ssh -p 2122 root@192.168.188.39 "chmod +x /SD/sensor_trident_modbus_ARM"
```

### "Segmentation fault" al ejecutar
- Ver [Debugging](Debugging) para análisis con GDB remoto

### Binario no termina (Ctrl+C no funciona)
```bash
# Terminar de otra terminal
ssh -p 2122 root@192.168.188.39 "pkill -f sensor_trident"
```

