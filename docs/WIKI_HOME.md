# QW3100 Sensor Trident - Modbus

Binario embedded para lectura de datos de sensor Trident con comunicación Modbus RTU/TCP.

## 📋 Descripción General

Este proyecto implementa un cliente Modbus para lectura periódica de registros de sensor Trident, con soporte para:
- **Compilación cruzada**: ARM 32-bit (arm-linux-gnueabihf)
- **Compilación nativa**: x86/x64 para pruebas locales
- **Comunicación**: Modbus RTU/TCP
- **Periodicity**: Configurable (default 5 segundos)
- **Serialización**: JSON via cJSON

## 🚀 Quick Start

```bash
# 1. Setup del entorno
./scripts/setup_dependencies_Sensor_Trident.sh

# 2. Compilar para ARM
task "1. Compilar ARM"

# 3. Enviar al dispositivo
task "2. Enviar por SCP"

# 4. Ejecutar
task "3. Ejecutar en Dispositivo"
```

## 📚 Documentación

- **[Arquitectura](Arquitectura)** - Componentes y estructura del proyecto
- **[Setup del Entorno](Setup)** - Configuración inicial y dependencias
- **[Compilación](Compilación)** - Guía completa de build (ARM & Native)
- **[Deployment](Deployment)** - Envío y ejecución en dispositivo
- **[Testing](Testing)** - Pruebas locales y en dispositivo
- **[Flujo de Desarrollo](Flujo-de-Desarrollo)** - Workflow típico y troubleshooting

## 📁 Estructura del Proyecto

```
QW3100_modbus/
├── src/
│   ├── main.c                # Punto de entrada, loop principal
│   ├── sensor.c/h            # Abstracción del sensor, registros
│   └── modbus_comm.c/h       # Comunicación Modbus
├── lib/
│   ├── cJSON.c/h             # Serialización JSON
│   └── libmodbus/            # Librería Modbus (submódulo)
├── test/
│   ├── main_test.c           # Suite de pruebas
│   └── main_test_JSON.c      # Pruebas de serialización JSON
├── scripts/
│   └── setup_dependencies... # Setup automático
└── .vscode/tasks.json        # Tareas de compilación y deploy
```

## 🛠️ Stack Técnico

| Componente | Versión | Propósito |
|-----------|---------|----------|
| libmodbus | master  | Comunicación Modbus |
| cJSON     | -       | Serialización JSON |
| arm-linux-gnueabihf | - | Compilador cruzado ARM |
| gcc       | native  | Compilador nativo |

## ⚙️ Configuración

- **Intervalo de polling**: `-i / --interval` (segundos, default: 5)
- **ID esclavo Modbus**: 1 (configurable en `REMOTE_ID`)
- **Puerto remoto**: 2122 (SSH)
- **Host remoto**: 192.168.188.39

## 📊 Estado del Proyecto

- ✅ Compilación ARM funcional
- ✅ Comunicación Modbus establecida
- ✅ Deploy mediante SCP
- ✅ Lectura periódica de registros
- 🔄 Testing en progreso
- 📝 Documentación en desarrollo

## 🤝 Contribuciones

Sigue el [flujo de desarrollo](Flujo-de-Desarrollo) para cambios.
