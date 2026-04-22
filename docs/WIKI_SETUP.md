# Setup del entorno

## Requisitos del sistema

```bash
# Compilador cruzado ARM
sudo apt install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf binutils-arm-linux-gnueabihf

# Herramientas de build
sudo apt install meson ninja-build cmake git curl perl make
```

---

## Compilar dependencias ARM (una sola vez)

Las dependencias (OpenSSL, libmodbus, libcurl, libmosquitto) se compilan desde fuente
y se instalan en `third_party/arm/`. Los archivos `.a` resultantes se commitean al repo,
por lo que en proyectos ya clonados este paso solo es necesario si `third_party/arm/`
no existe.

```bash
./scripts/build_third_party.sh
```

El script compila en orden:
1. **OpenSSL** — base TLS (`no-shared no-engine no-dso` — crítico para binario estático ARM)
2. **c-ares** — resolver DNS sin glibc NSS (crítico para binario estático ARM)
3. **libmodbus** — Modbus RTU, sin dependencias externas
4. **libcurl** — con OpenSSL propio y `--enable-ares` para usar c-ares
5. **libmosquitto** — con cJSON bundled, con OpenSSL propio

Las fuentes se descargan a `_deps_build/` (gitignoreado). Los `.a` van a:

```
third_party/arm/
├── openssl/    include/ + lib/libssl.a + lib/libcrypto.a
├── cares/      include/ + lib/libcares.a
├── modbus/     include/modbus/ + lib/libmodbus.a
├── curl/       include/curl/  + lib/libcurl.a
└── mosquitto/  include/mosquitto.h + lib/libmosquitto.a
```

Verificar:
```bash
find third_party/arm -name "*.a" | sort
# → third_party/arm/cares/lib/libcares.a
# → third_party/arm/curl/lib/libcurl.a
# → third_party/arm/modbus/lib/libmodbus.a
# → third_party/arm/mosquitto/lib/libmosquitto.a
# → third_party/arm/openssl/lib/libcrypto.a
# → third_party/arm/openssl/lib/libssl.a
```

---

## Dependencias devlinux (tests en el PC de desarrollo)

Para compilar y ejecutar tests en el PC de desarrollo se usan los paquetes del sistema:

```bash
sudo apt install libmodbus-dev libcurl4-openssl-dev libmosquitto-dev libssl-dev
```

---

## Alias SSH para el dispositivo

Agregar a `~/.ssh/config`:

```
Host qw3100-device
  HostName <IP_DEL_DISPOSITIVO>
  Port 2122
  User root
```

Para habilitar `make deploy` sin contraseña:

```bash
ssh-copy-id -p 2122 root@<IP_DEL_DISPOSITIVO>
```

---

## Verificar el entorno

```bash
# Compilador ARM disponible
arm-linux-gnueabihf-gcc --version

# Dependencias ARM presentes
find third_party/arm -name "*.a" | sort

# Compilar tests y verificar
make test

# Compilar ARM
make arm
file sensor_trident_modbus_ARM   # → ELF 32-bit LSB executable, ARM
ldd sensor_trident_modbus_ARM    # → not a dynamic executable
```
