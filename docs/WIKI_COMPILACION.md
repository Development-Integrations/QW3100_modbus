# Compilación

## 📊 Matriz de Compilación

| Target | Propósito | Comando | Output |
|--------|-----------|---------|--------|
| **ARM** | Dispositivo embebido | Task "1. Compilar ARM" | `sensor_trident_modbus_ARM` |
| **Native** | Pruebas locales | Task "Compilar Native" | `test/main_test` |

## 🔧 Compilación ARM (Cross-compile)

### Opción 1: Usar VS Code Task (Recomendado)

```
Cmd+Shift+B → Seleccionar "1. Compilar ARM"
```

Or:
```
Cmd+Shift+P → Tasks: Run Task → "1. Compilar ARM"
```

### Opción 2: Línea de Comando Manual

```bash
export PREFIX_ARM="$HOME/opt/libmodbus-arm"

arm-linux-gnueabihf-gcc \
  ./src/main.c \
  ./src/sensor.c \
  ./src/modbus_comm.c \
  ./lib/cJSON.c \
  -o sensor_trident_modbus_ARM \
  -static \
  -Wall \
  -Wextra \
  -I$PREFIX_ARM/include/modbus \
  $PREFIX_ARM/lib/libmodbus.a
```

### Flags Explicados

| Flag | Propósito |
|------|-----------|
| `-static` | Enlazar estáticamente todas las dependencias |
| `-Wall` | Mostrar todos los warnings |
| `-Wextra` | Warnings extra (recomendado) |
| `-I` | Path de include (headers) |

### Verificar Output

```bash
# Verificar que se compiló
ls -la sensor_trident_modbus_ARM

# Verificar arquitectura
file sensor_trident_modbus_ARM
# Output: ELF 32-bit LSB executable, ARM, ...

# Verificar tamaño (debe estar entre 100KB-500KB)
du -h sensor_trident_modbus_ARM
```

## 🧪 Compilación Native (Pruebas Locales)

### Opción 1: VS Code Task

```
Cmd+Shift+B → "Compilar binario Native para Pruebas"
```

### Opción 2: Línea de Comando

```bash
export PREFIX_NATIVE="$HOME/opt/libmodbus-native"

gcc \
  ./test/main_test.c \
  ./lib/cJSON.c \
  -o ./test/main_test \
  -g \
  -O0 \
  -Wall \
  -Wextra \
  -I$PREFIX_NATIVE/include/modbus \
  $PREFIX_NATIVE/lib/libmodbus.a
```

### Verificar y Ejecutar

```bash
# Verificar compilación
ls -la ./test/main_test

# Ejecutar con GDB si deseas debugging
gdb ./test/main_test

# O ejecutar directamente
./test/main_test --interval 2
```

## 🛠️ Opciones de Compilación Avanzada

### Habilitar Debug Info

```bash
# Agregar -g para símbolos de debug
arm-linux-gnueabihf-gcc ... -g ...
```

### Optimización

| Flag | Nivel | Uso |
|------|-------|-----|
| `-O0` | Sin optimización | Debug (más lento pero predecible) |
| `-O1` | Optimización básica | Balance |
| `-O2` | Optimización moderada | Producción |
| `-O3` | Optimización máxima | Máxima velocidad (raro) |

**Recomendación**: 
- ARM (producción): `-O2`
- Native (testing): `-O0` o `-g -O0`

### Sanitizers (Detección de Bugs)

```bash
# Agregar para detección de memory leaks, etc.
gcc ... -fsanitize=address -fsanitize=undefined ...
```

## 📋 Checklist de Compilación

- [ ] `$PREFIX_ARM/lib/libmodbus.a` existe
- [ ] Todos los `*.c` y `*.h` están presentes
- [ ] No hay errores de compilación
- [ ] Binario ARM es ejecutable: `file sensor_trident_modbus_ARM`
- [ ] Tamaño binario razonable (100KB-500KB)
- [ ] Testear compilación native: `./test/main_test`

## 🐛 Errores Comunes

### Error: "undefined reference to `modbus_*'"

**Causa**: Librería libmodbus no enlazada correctamente

**Solución**:
```bash
# Verificar ruta
ls -la $PREFIX_ARM/lib/libmodbus.a

# Re-compilar con ruta absoluta
... /home/user/opt/libmodbus-arm/lib/libmodbus.a
```

### Error: "arm-linux-gnueabihf-gcc: command not found"

**Causa**: Compilador ARM no instalado

**Solución**:
```bash
sudo apt-get install arm-linux-gnueabihf-gcc
```

### Error: "multiple definition of `SYMBOL`"

**Causa**: Duplicado en includes o compilación

**Solución**:
```bash
# Verificar que cada archivo .c se compila una sola vez
# En el comando de gcc, NO incluir archivos .c dos veces
```

### Warning: "unused variable"

**Solución**: 
- Si es falso positivo: `__attribute__((unused))`
- Si es real: Remover variable no usada

## 🔍 Debugging de Compilación

### Ver comandos exactos ejecutados

```bash
# Usar -v para verbose
arm-linux-gnueabihf-gcc -v ... (toda la salida debug)
```

### Pre-procesar sin compilar

```bash
# Ver resultado de #define, #include, etc.
arm-linux-gnueabihf-gcc -E ./src/main.c -o main.i
```

### Generar assembly

```bash
# Ver assembly generado
arm-linux-gnueabihf-gcc -S ./src/main.c -o main.s
```

## 📊 Tiempos Típicos

| Operación | Tiempo |
|-----------|--------|
| Compilación ARM limpia | 5-10 seg |
| Compilación Native limpia | 2-3 seg |
| Recompilación (cambio pequeño) | 1-2 seg |

