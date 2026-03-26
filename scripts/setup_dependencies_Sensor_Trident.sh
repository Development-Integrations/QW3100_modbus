#!/bin/bash

# =============================================================================
# Instala libmodbus estatica para el target seleccionado.
# Descomenta UNA de las dos secciones antes de ejecutar:
#
#   NATIVA   -> equipo Actual  (PC de desarrollo, Raspberry Pi, etc)
#   CRUZADA  -> armv7l  (dispositivo embebido objetivo, arm-linux-gnueabihf)
#               Requiere: sudo apt install gcc-arm-linux-gnueabihf
# =============================================================================

set -e


sudo apt update
sudo apt upgrade -y

sudo apt install -y build-essential gcc-arm-linux-gnueabihf autoconf automake libtool git \
    libcurl4-openssl-dev

export WORKDIR=$(pwd)

cd $WORKDIR/lib

# Clonar libmodbus si no existe
if [ ! -d "libmodbus" ]; then
    git clone https://github.com/stephane/libmodbus.git
fi
cd $WORKDIR/lib/libmodbus
./autogen.sh

# =============================================================================
# OPCION A: Nativa -> equipo Actual  (PC de desarrollo, Raspberry Pi, etc)
# =============================================================================
# export PREFIX="$HOME/opt/libmodbus-native"
# mkdir -p "$PREFIX"
# ./configure \
#   --prefix="$PREFIX" \
#   --enable-static \
#   --disable-shared

# =============================================================================
# OPCION B: Compilacion cruzada -> armv7l  (dispositivo embebido)
# =============================================================================
export PREFIX="$HOME/opt/libmodbus-arm"
mkdir -p "$PREFIX"
./configure \
  --host=arm-linux-gnueabihf \
  --prefix="$PREFIX" \
  --enable-static \
  --disable-shared \
  CC=arm-linux-gnueabihf-gcc

# =============================================================================
make -j$(nproc)
make install

if [ -f "$PREFIX/lib/libmodbus.a" ]; then
    echo "OK: $PREFIX/lib/libmodbus.a"
else
    echo "ERROR: no se encontro libmodbus.a en $PREFIX/lib/"
    exit 1
fi


# =============================================================================
# Instala libcurl estatica para el target seleccionado.
# Descomenta UNA de las dos secciones antes de ejecutar:
#
#   NATIVA   -> equipo Actual  (PC de desarrollo, Raspberry Pi, etc)
#   CRUZADA  -> armv7l  (dispositivo embebido objetivo, arm-linux-gnueabihf)
#               Requiere: sudo apt install gcc-arm-linux-gnueabihf
# =============================================================================



cd $WORKDIR/lib

# Clonar curl si no existe
if [ ! -d "curl" ]; then
    git clone  https://github.com/curl/curl.git
fi
cd $WORKDIR/lib/curl
./autogen.sh

# =============================================================================
# OPCION A: Nativa -> equipo Actual  (PC de desarrollo, Raspberry Pi, etc)
# =============================================================================
# export PREFIX_CURL_NATIVE="$HOME/opt/curl-native"
# mkdir -p "$PREFIX_CURL_NATIVE"
# ./configure \
#   --prefix="$PREFIX_CURL_NATIVE" \
#   --enable-static \
#   --disable-shared \
#   --disable-ssl \
#   --with-openssl=no \
#   --disable-ldap 
# =============================================================================
# OPCION B: Compilacion cruzada -> armv7l  (dispositivo embebido)
# =============================================================================
export PREFIX_CURL_ARM="$HOME/opt/libcurl-arm"
mkdir -p "$PREFIX_CURL_ARM"

./configure \
  --host=arm-linux-gnueabihf \
  --prefix="$PREFIX_CURL_ARM" \
  --enable-static \
  --disable-shared \
  --without-ssl \
  --with-openssl=no \
  --disable-ldap \
  --without-libpsl \
  CC=arm-linux-gnueabihf-gcc


make -j$(nproc)
make install

if [ -f "$PREFIX_CURL_ARM/lib/libcurl.a" ]; then
    echo "OK: $PREFIX_CURL_ARM/lib/libcurl.a"
else
    echo "ERROR: no se encontro libcurl.a en $PREFIX_CURL_ARM/lib/"
    exit 1
fi