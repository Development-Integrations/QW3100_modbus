# Compilación

## Makefile (forma recomendada)

```bash
make          # compila binario ARM → sensor_trident_modbus_ARM
make native   # compila tests nativos → test/main_test
make deploy   # arm + scp al dispositivo
make run      # deploy + ejecuta en el dispositivo vía SSH
make clean    # elimina binarios generados
```

---

## Comandos manuales

### ARM (cross-compile)

```bash
arm-linux-gnueabihf-gcc \
  src/main.c src/sensor.c src/modbus_comm.c src/config.c \
  src/persist.c src/http_sender.c src/circuit_breaker.c lib/cJSON.c \
  -o sensor_trident_modbus_ARM \
  -static -Wall -Wextra \
  -I$HOME/opt/libmodbus-arm/include/modbus \
  -I$HOME/opt/libcurl-arm/include \
  $HOME/opt/libmodbus-arm/lib/libmodbus.a \
  $HOME/opt/libcurl-arm/lib/libcurl.a
```

### Nativa (tests)

```bash
gcc \
  test/main_test.c src/sensor.c src/modbus_comm.c src/config.c \
  src/persist.c src/http_sender.c lib/cJSON.c \
  -o test/main_test \
  -g -O0 -Wall -Wextra \
  -I$HOME/opt/libmodbus-native/include/modbus \
  $HOME/opt/libmodbus-native/lib/libmodbus.a -lcurl
```

---

## Verificar el binario ARM

```bash
# Arquitectura correcta
file sensor_trident_modbus_ARM
# → ELF 32-bit LSB executable, ARM

# Sin dependencias dinámicas
ldd sensor_trident_modbus_ARM
# → not a dynamic executable
```

---

## Warnings esperados

Al compilar estáticamente con libcurl aparecen estos warnings — son inofensivos:

```
warning: Using 'getaddrinfo' in statically linked applications requires
         at runtime the shared libraries from the glibc version used for linking
```

El binario funciona correctamente en el dispositivo.
