# Setup del Entorno

## ✅ Requisitos Previos

### Hardware
- PC Linux con acceso a internet
- Compilador cruzado ARM: `arm-linux-gnueabihf-gcc`
- Dispositivo remoto (sensor Trident) con SSH activo

### Software
- Linux (Ubuntu 20.04+ recomendado)
- `git`, `gcc`, `make`, `autoconf`, `libtool`
- ARM toolchain
- SSH/SCP

### Conectividad
- Acceso SSH al dispositivo remoto: `192.168.188.39:2122`
- Credenciales: `root` (configurable)

## 🚀 Instalación Automática

El proyecto incluye script de setup automatizado:

```bash
cd QW3100_modbus
./scripts/setup_dependencies_Sensor_Trident.sh
```

### ¿Qué hace el script?

1. ✅ Clona libmodbus (si no existe)
2. ✅ Genera archivos de configuración (`./autogen.sh`)
3. ✅ Configura para compilación cruzada ARM
4. ✅ Compila e instala en `$HOME/opt/libmodbus-arm`
5. ✅ Verifica la instalación

## 📋 Instalación Manual (Paso a Paso)

Si prefieres hacer todo manualmente:

### 1. Instalar dependencias del sistema

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    autoconf \
    libtool \
    git \
    arm-linux-gnueabihf-gcc \
    arm-linux-gnueabihf-binutils \
    pkg-config
```

### 2. Ir al directorio lib/

```bash
cd lib/
```

### 3. Clonar libmodbus

```bash
git clone https://github.com/stephane/libmodbus.git
cd libmodbus
```

### 4. Generar configure

```bash
./autogen.sh
```

### 5. Configurar para ARM

```bash
export PREFIX_ARM="$HOME/opt/libmodbus-arm"
mkdir -p "$PREFIX_ARM"

./configure \
  --host=arm-linux-gnueabihf \
  --prefix="$PREFIX_ARM" \
  --enable-static \
  --disable-shared \
  CC=arm-linux-gnueabihf-gcc
```

### 6. Compilar e instalar

```bash
make -j$(nproc)
make install
```

### 7. Verificar instalación

```bash
ls -la $PREFIX_ARM/lib/libmodbus.a
# Debe existir y tener tamaño > 0
```

### 8. Persistir la variable (opcional pero recomendado)

```bash
# Agregar a ~/.bashrc o ~/.zshrc
echo 'export PREFIX_ARM="$HOME/opt/libmodbus-arm"' >> ~/.bashrc
source ~/.bashrc
```

## 🛠️ Configuración de VS Code

### 1. Instalar extensión C/C++

```bash
# En VS Code
# Cmd+Shift+X → Buscar "C/C++" → Instalar Microsoft extension
```

### 2. Configurar tareas (tasks.json)

Ya incluidas en `.vscode/tasks.json`. Cargar con:
```
Cmd+Shift+B → Seleccionar tarea
```

### 3. Configurar IntelliSense

`.vscode/c_cpp_properties.json` (crear si no existe):

```json
{
  "configurations": [
    {
      "name": "ARM",
      "includePath": [
        "${workspaceFolder}/src",
        "${workspaceFolder}/lib",
        "${workspaceFolder}/lib/libmodbus/src",
        "$HOME/opt/libmodbus-arm/include"
      ],
      "defines": ["__ARM__"],
      "compilerPath": "/usr/bin/arm-linux-gnueabihf-gcc"
    },
    {
      "name": "Native",
      "includePath": [
        "${workspaceFolder}/src",
        "${workspaceFolder}/lib",
        "$HOME/opt/libmodbus-native/include"
      ],
      "compilerPath": "/usr/bin/gcc"
    }
  ],
  "version": 4
}
```

## 📝 Variáveis de Entorno

```bash
# ARM (cross-compile)
export PREFIX_ARM="$HOME/opt/libmodbus-arm"

# Native (testing local)
export PREFIX_NATIVE="$HOME/opt/libmodbus-native"

# Dispositivo remoto
export REMOTE_HOST="192.168.188.39"
export REMOTE_PORT="2122"
export REMOTE_USER="root"
export REMOTE_PATH="/SD"
```

## ✔️ Checklist de Verificación

- [ ] `arm-linux-gnueabihf-gcc --version` funciona
- [ ] `gcc --version` funciona
- [ ] `$PREFIX_ARM/lib/libmodbus.a` existe
- [ ] `.vscode/tasks.json` está presente
- [ ] SSH al dispositivo remoto accesible: `ssh -p 2122 root@192.168.188.39`
- [ ] Directorio `/SD/` accesible en dispositivo remoto

## 🐛 Troubleshooting

### "arm-linux-gnueabihf-gcc: not found"
```bash
# Ubuntu/Debian
sudo apt-get install arm-linux-gnueabihf-gcc
```

### "libmodbus.a not found"
```bash
# Verificar ruta
ls -la $PREFIX_ARM/lib/

# Re-ejecutar setup
./scripts/setup_dependencies_Sensor_Trident.sh
```

### "SSH connection refused"
```bash
# Verificar dispositivo remoto está disponible
ping 192.168.188.39

# Verificar puerto
ssh -p 2122 root@192.168.188.39 'echo OK'
```

