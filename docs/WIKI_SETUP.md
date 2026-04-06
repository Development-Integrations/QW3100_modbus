# Setup del entorno

## Requisitos del sistema

```bash
# Compilador cruzado ARM
sudo apt install gcc-arm-linux-gnueabihf

# Herramientas de build
sudo apt install make git curl
```

---

## Compilar libmodbus para ARM

```bash
git clone https://github.com/stephane/libmodbus.git
cd libmodbus

./autogen.sh
./configure --host=arm-linux-gnueabihf --prefix=$HOME/opt/libmodbus-arm --enable-static --disable-shared
make -j$(nproc)
make install
```

Verificar:
```bash
ls $HOME/opt/libmodbus-arm/lib/libmodbus.a   # debe existir
```

---

## Compilar libmodbus para el PC de desarrollo (devlinux)

```bash
# En el mismo directorio de libmodbus clonado
make distclean

./configure --prefix=$HOME/opt/libmodbus-devlinux --enable-static --disable-shared
make -j$(nproc)
make install
```

---

## Compilar libcurl para ARM (sin SSL)

```bash
curl -LO https://curl.se/download/curl-8.7.1.tar.gz
tar xzf curl-8.7.1.tar.gz
cd curl-8.7.1

./configure \
  --host=arm-linux-gnueabihf \
  --prefix=$HOME/opt/libcurl-arm \
  --enable-static \
  --disable-shared \
  --without-ssl \
  --disable-ldap \
  --disable-ldaps \
  --without-libpsl \
  --without-brotli \
  --without-zstd

make -j$(nproc)
make install
```

Verificar:
```bash
ls $HOME/opt/libcurl-arm/lib/libcurl.a   # debe existir
```

> **Nota:** Se compila sin SSL porque el endpoint Scante usa HTTPS en un proxy externo. El binario ARM se conecta al proxy interno por HTTP.

---

## Variables de entorno

Estas son las variables que usa el Makefile (sobreescribibles vía entorno o `make VAR=valor`):

```bash
# ARM (cross-compile)
PREFIX_MODBUS_ARM=$HOME/opt/libmodbus-arm
PREFIX_CURL_ARM=$HOME/opt/libcurl-arm

# devlinux (tests en el PC de desarrollo)
PREFIX_MODBUS_DEVLINUX=$HOME/opt/libmodbus-devlinux
PREFIX_CURL_DEVLINUX=$HOME/opt/libcurl-devlinux
```

---

## Forma recomendada: script de setup

El script `scripts/setup_dependencies_Sensor_Trident.sh` automatiza todo lo anterior (libmodbus + libcurl) para ambos targets:

```bash
./scripts/setup_dependencies_Sensor_Trident.sh arm       # para cross-compile ARM
./scripts/setup_dependencies_Sensor_Trident.sh devlinux  # para tests en el PC de desarrollo
```

El script detecta si las librerías ya están compiladas y las omite, y valida que libcurl no tenga dependencias externas (nghttp2, zlib, etc.) que causarían errores al linkear estáticamente.

---

## Compilar libcurl para el PC de desarrollo (devlinux)

La compilación devlinux también requiere libcurl estática propia (sin nghttp2/zlib/brotli), ya que `-lcurl` del sistema puede causar errores "undefined reference" al linkear en modo estático.

```bash
# Usar el script es lo más simple:
./scripts/setup_dependencies_Sensor_Trident.sh devlinux

# O manualmente, desde el directorio de fuentes de curl:
./configure \
  --prefix=$HOME/opt/libcurl-devlinux \
  --enable-static \
  --disable-shared \
  --without-ssl \
  --without-libpsl \
  --without-zstd \
  --without-brotli \
  --without-zlib \
  --without-libidn2 \
  --without-nghttp2

make -j$(nproc) && make install
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

# Librerías presentes
ls $HOME/opt/libmodbus-arm/lib/libmodbus.a
ls $HOME/opt/libcurl-arm/lib/libcurl.a
ls $HOME/opt/libmodbus-devlinux/lib/libmodbus.a

# Compilar y testear
make devlinux && ./test/main_test
make arm
```
