#!/bin/bash

set -e  # detener si hay error



# Clonar libmodbus e ignorar si ya existe
if [ ! -d "libmodbus" ]; then
    git clone https://github.com/stephane/libmodbus.git
fi
cd libmodbus

# generar el configure
./autogen.sh

# ------------ solo dejar un tipo de compilacion -----------------
# para la compilacion cruzada
export PREFIX_ARM="$HOME/opt/libmodbus-arm" # para persistencia pegar en .bashrc o .zshrc

echo "Instalando libmodbus en $PREFIX_ARM"
mkdir -p "$PREFIX_ARM"

./configure \
  --host=arm-linux-gnueabihf \
  --prefix="$PREFIX_ARM" \
  --enable-static \
  --disable-shared \
  CC=arm-linux-gnueabihf-gcc

# para la compilacion nativa
# export PREFIX_NATIVE="$HOME/opt/libmodbus-native" # para persistencia pegar en .bashrc o .zshrc
# mkdir -p "$PREFIX_NATIVE"
# ./configure \
#   --prefix="$PREFIX_NATIVE" \
#   --enable-static \
#   --disable-shared

# ----------------------------------------------------------------

# Compilar e instalar
make -j$(nproc)
make install




#verificar la instalacion para ARM
if [ -f "$PREFIX_ARM/lib/libmodbus.a" ]; then
    echo "La biblioteca libmodbus se ha instalado correctamente en $PREFIX_ARM/lib/libmodbus.a"
else
    echo "Error: No se encontró la biblioteca libmodbus en $PREFIX_ARM/lib/libmodbus.a"
fi  

# verificar la instalacion para nativo
# if [ -f "$PREFIX_NATIVE/lib/libmodbus.a" ]; then
#     echo "La biblioteca libmodbus se ha instalado correctamente en $PREFIX_NATIVE/lib/libmodbus.a"
# else
#     echo "Error: No se encontró la biblioteca libmodbus en $PREFIX_NATIVE/lib/libmodbus.a"
# fi    