# Arquitectura del Proyecto

## 🏗️ Diagrama de Componentes

```
┌─────────────────────────────────────────────┐
│     Binario QW3100 (main.c)                │
│  - Loop de polling                         │
│  - Manejo de argumentos CLI                │
└────────────┬────────────────────────────────┘
             │
        ┌────┴──────────────────┬──────────────┐
        │                       │              │
     ┌──▼──────┐         ┌─────▼─────┐  ┌────▼────┐
     │ Sensor  │         │  Modbus   │  │  JSON   │
     │ Module  │         │  Comm     │  │ Serial  │
     │         │         │           │  │         │
     └──┬──────┘         └─────┬─────┘  └────┬────┘
        │                      │             │
        │      libmodbus       │     cJSON   │
        └──────────┬───────────┴─────────────┘
                   │
        ┌──────────▼──────────┐
        │  Dispositivo Remoto  │
        │  (Sensor Trident)    │
        └─────────────────────┘
```

## 📦 Módulos

### `main.c`
**Propósito**: Punto de entrada, coordinación general

**Responsabilidades**:
- Parseo de argumentos (intervalo de polling)
- Inicialización de módulos
- Loop principal de polling con intervalo configurable
- Manejo de señales (Ctrl+C)
- Salida de datos

**Interfaz**:
```c
int parse_interval_arg(int argc, char *argv[], uint16_t *interval_sec)
```

### `sensor.c/h`
**Propósito**: Abstracción del sensor y sus registros

**Responsabilidades**:
- Definición de estructura de registros del sensor
- Mapeo de registros Modbus ↔ campos del sensor
- Tipos de datos (UINT32, UINT16, FLOAT)
- Información de unidades y metadatos

**Estructuras**:
```c
#define REGISTER_SENSOR_INFO_SIZE  12
#define REGISTER_SENSOR_DATA_SIZE  63
#define REGISTER_SENSOR_ALL_SIZE   75
```

**Tipos de datos soportados**:
- `TYPE_UINT32`: Registros de 32 bits (2 × 16-bit)
- `TYPE_UINT16`: Registros de 16 bits
- `TYPE_FLOAT`: Números flotantes

### `modbus_comm.c/h`
**Propósito**: Abstracción de comunicación Modbus

**Responsabilidades**:
- Inicialización de contexto Modbus
- Lectura de bloques de registros
- Traducciones de direcciones
- Manejo de errores de comunicación

**Interfaz principal**:
```c
int read_block_modbus(modbus_t *ctx, int addr, int size, uint16_t *buffer)
```

### `lib/cJSON.c/h`
**Propósito**: Serialización JSON de datos

**Uso**: Formateo de salida de datos del sensor a JSON

## 🔄 Flujo de Datos

```
main()
  ↓
parse_interval_arg()
  ↓
Setup Modbus Context
  ↓
LOOP (cada N segundos):
  ├─ read_block_modbus(ctx, ADDR, SIZE, buffer)
  ├─ Parse buffer → sensor_data
  ├─ Serialize → JSON
  └─ Output (stdout/archivo)
```

## 📡 Protocolo Modbus

- **Tipo**: RTU (Serial) / TCP
- **ID Esclavo**: 1 (configurable como `REMOTE_ID`)
- **Rango de Registros**:
  - Info: Registros 0-11 (12 × 16-bit)
  - Datos: Registros 12-74 (63 × 16-bit)
  - Total: 75 registros

## 🔐 Configuración

| Parámetro | Valor | Ubicación |
|-----------|-------|-----------|
| REMOTE_ID | 1 | main.c:L19 |
| TIME_POLLING_INTERVAL_DEFAULT | 5 seg | main.c:L21 |
| REGISTER_SENSOR_INFO_SIZE | 12 | sensor.h |
| REGISTER_SENSOR_DATA_SIZE | 63 | sensor.h |

## 🔗 Dependencias

```
QW3100 Binario
├── libmodbus (externo, compilado)
└── cJSON (incluido en lib/)
```

## 📝 Notas de Diseño

- **Modular**: Cada componente puede ser testeado independientemente
- **Cross-platform**: Compilable para ARM y x86/x64
- **Estático**: Todas las dependencias se enlazan estáticamente
- **JSON-ready**: Fácil serialización de resultados
