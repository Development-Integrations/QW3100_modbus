# Testing

## 🧪 Estrategia de Testing

```
Unitario (módulos aislados)
        ↓
Integración (módulos juntos)
        ↓
Native (binario en host)
        ↓
Remoto (dispositivo embebido)
```

## 🔬 Tests Unitarios

### Test: Lectura de Registros Modbus

**Archivo**: `test/main_test.c`

**Propósito**: Verificar que las funciones de lectura Modbus funcionan

**Compilar**:
```bash
# Via VS Code Task
Cmd+Shift+B → "Compilar binario Native para Pruebas"

# O manualmente
gcc ./test/main_test.c ./lib/cJSON.c \
    -o ./test/main_test -g -O0 -Wall -Wextra \
    -I$PREFIX_NATIVE/include/modbus \
    $PREFIX_NATIVE/lib/libmodbus.a
```

**Ejecutar**:
```bash
./test/main_test
```

**Verificar**: 
- ✅ No hay segmentation fault
- ✅ Output JSON válido
- ✅ Valores dentro de rango esperado

### Test: Serialización JSON

**Archivo**: `test/main_test_JSON.c`

**Propósito**: Verificar serialización correcta de datos a JSON

**Compilar**:
```bash
gcc ./test/main_test_JSON.c ./lib/cJSON.c \
    -o ./test/main_test_json -g -O0
```

**Ejecutar**:
```bash
./test/main_test_json | jq .
```

**Verificar**:
- ✅ JSON válido (parseable con `jq`)
- ✅ Todos los campos presentes
- ✅ Tipos de datos correctos

## 🖥️ Testing en Host (Native)

### Setup para Testing Native

Para compilar y testar **localmente** sin ARM:

```bash
# Compilar libmodbus nativo
export PREFIX_NATIVE="$HOME/opt/libmodbus-native"
mkdir -p "$PREFIX_NATIVE"

cd lib/libmodbus
./configure --prefix="$PREFIX_NATIVE" --enable-static --disable-shared
make -j$(nproc)
make install
```

### Test Suite Completo

```bash
# Ejecutar todos los tests
./test/main_test --interval 2

# Con valgrind (memory check)
valgrind --leak-check=full ./test/main_test --interval 1
```

### Expected Output

```
Conectando a Modbus...
Leyendo registros 0-11 (INFO)...
Leyendo registros 12-74 (DATA)...
Sensor Data:
{
  "temp": 25.3,
  "humidity": 65.2,
  "status": "OK",
  ...
}
```

## 🔌 Testing en Dispositivo Remoto

### Test Remoto Básico

```bash
# Enviar y ejecutar
scp -P 2122 sensor_trident_modbus_ARM root@192.168.188.39:/SD/
ssh -p 2122 root@192.168.188.39 "/SD/sensor_trident_modbus_ARM --interval 5"

# Capturar output por 30 segundos
timeout 30 ssh -p 2122 root@192.168.188.39 \
    "/SD/sensor_trident_modbus_ARM -i 2" | head -100
```

### Verificar Conectividad Modbus

Desde el dispositivo remoto:

```bash
# Verificar que el sensor es accesible
modbus-cli -b 9600 -d /dev/ttyRS485 read-registers 0 12

# Si modbus-cli no disponible, usar strace para inspeccionar
strace -e trace=open,read,write /SD/sensor_trident_modbus_ARM -i 2
```

## 📊 Criterios de Aceptación

| Criterio | Prueba | Paso/Fallo |
|----------|--------|-----------|
| Compila sin errores | `gcc ... -Wall` | ✓ = 0 errores |
| Enlaza correctamente | `nm sensor_*` | ✓ = Símbolos resueltos |
| Ejecuta sin crash | `./main_test` | ✓ = Exit 0 |
| Lee registros | Output contiene valores | ✓ = Datos válidos |
| JSON válido | `jq . < output` | ✓ = Parses OK |
| Conecta a remoto | SSH + ejecuta | ✓ = Output recibido |
| Intervalo correcto | Medir tiempo entre líneas | ✓ = Intervalo ≈ configurado |

## 🔍 Debugging

### Debugging Local con GDB

```bash
# Compilar con símbolos
gcc ./test/main_test.c ... -g ...

# Iniciar GDB
gdb ./test/main_test

# En GDB:
(gdb) break main
(gdb) run --interval 2
(gdb) next
(gdb) print variable_name
(gdb) continue
```

### Debugging Remoto con GDB

```bash
# En dispositivo: iniciar gdbserver
ssh -p 2122 root@192.168.188.39 \
    "gdbserver localhost:9999 /SD/sensor_trident_modbus_ARM"

# En host: conectar
arm-linux-gnueabihf-gdb sensor_trident_modbus_ARM

# En gdb:
(gdb) target remote 192.168.188.39:9999
(gdb) break main
(gdb) continue
```

### Análisis con Valgrind (Memory Leaks)

```bash
# Compilar nativo con -g
gcc ./test/main_test.c ... -g -O0 ...

# Ejecutar con valgrind
valgrind --leak-check=full --show-leak-kinds=all \
    ./test/main_test --interval 1

# Salida esperada:
# ==12345== HEAP SUMMARY:
# ==12345== in use at exit: 0 bytes (expected)
```

## 🎯 Testing End-to-End

### Script de E2E Test

```bash
#!/bin/bash

set -e

echo "🔨 [1] Compilar..."
gcc ./test/main_test.c ./lib/cJSON.c -o /tmp/test -g

echo "🧪 [2] Ejecutar test local..."
/tmp/test --interval 1

echo "📦 [3] Compilar ARM..."
arm-linux-gnueabihf-gcc ./src/*.c ./lib/cJSON.c \
    -o sensor_trident_modbus_ARM -static \
    -I$PREFIX_ARM/include/modbus $PREFIX_ARM/lib/libmodbus.a

echo "📤 [4] Enviar..."
scp -P 2122 sensor_trident_modbus_ARM root@192.168.188.39:/SD/

echo "🚀 [5] Ejecutar remoto..."
timeout 15 ssh -p 2122 root@192.168.188.39 \
    "/SD/sensor_trident_modbus_ARM -i 3"

echo "✅ E2E Test PASSED"
```

## 📋 Checklist de Testing

- [ ] Tests locales compilan sin errores
- [ ] Tests ejecutan sin crashes
- [ ] Valgrind no detecta memory leaks
- [ ] Conectividad Modbus verific

ada
- [ ] JSON output válido
- [ ] ARM binary tested en dispositivo
- [ ] Intervalo de polling correcto
- [ ] Output capturado y verificado

## 🐛 Casos de Error a Testar

- ✓ Error de conexión Modbus
- ✓ Valores fuera de rango
- ✓ Timeout en lectura
- ✓ Intervalo inválido (0, -1, MAX)
- ✓ Dispositivo desconectado
- ✓ Memoria insuficiente

