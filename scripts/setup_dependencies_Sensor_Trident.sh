#!/bin/bash

# =============================================================================
# Instala libmodbus estatica para el target seleccionado.
# Descomenta UNA de las dos secciones antes de ejecutar:
#
#   NATIVA   -> x86_64  (PC de desarrollo, para pruebas locales)
#   CRUZADA  -> armv7l  (dispositivo embebido, arm-linux-gnueabihf)
#               Requiere: sudo apt install gcc-arm-linux-gnueabihf
# =============================================================================

set -e

sudo apt update
sudo apt upgrade -y

sudo apt install -y build-essential gcc-arm-linux-gnueabihf autoconf automake libtool git

export WORKDIR=$(pwd)

cd $WORKDIR/lib

# Clonar libmodbus si no existe
if [ ! -d "libmodbus" ]; then
    git clone https://github.com/stephane/libmodbus.git
fi
cd $WORKDIR/lib/libmodbus
./autogen.sh

# =============================================================================
# OPCION A: Nativa x86_64  (PC de desarrollo)
# =============================================================================
export PREFIX="$HOME/opt/libmodbus-native"
mkdir -p "$PREFIX"
./configure \
  --prefix="$PREFIX" \
  --enable-static \
  --disable-shared

# =============================================================================
# OPCION B: Compilacion cruzada -> armv7l  (dispositivo embebido)
# =============================================================================
# export PREFIX="$HOME/opt/libmodbus-arm"
# mkdir -p "$PREFIX"
# ./configure \
#   --host=arm-linux-gnueabihf \
#   --prefix="$PREFIX" \
#   --enable-static \
#   --disable-shared \
#   CC=arm-linux-gnueabihf-gcc

# =============================================================================
make -j$(nproc)
make install

if [ -f "$PREFIX/lib/libmodbus.a" ]; then
    echo "OK: $PREFIX/lib/libmodbus.a"
else
    echo "ERROR: no se encontro libmodbus.a en $PREFIX/lib/"
    exit 1
fi
