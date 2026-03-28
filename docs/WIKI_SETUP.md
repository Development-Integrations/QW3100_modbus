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

## Compilar libmodbus nativa (para tests)

```bash
# En el mismo directorio de libmodbus clonado
make distclean

./configure --prefix=$HOME/opt/libmodbus-native --enable-static --disable-shared
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

Agregar a `~/.bashrc` o `~/.zshrc`:

```bash
export PREFIX_ARM=$HOME/opt/libmodbus-arm
export PREFIX_CURL=$HOME/opt/libcurl-arm
export PREFIX_NATIVE=$HOME/opt/libmodbus-native
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
ls $HOME/opt/libmodbus-native/lib/libmodbus.a

# Compilar y testear
make native && ./test/main_test
make arm
```
