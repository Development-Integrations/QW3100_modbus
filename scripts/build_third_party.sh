#!/bin/bash
# =============================================================================
# build_third_party.sh  —  Compila dependencias ARM una sola vez
#
# Uso:
#   ./scripts/build_third_party.sh
#
# Resultado:
#   third_party/arm/{openssl,modbus,curl,mosquitto}/
#     ├── include/
#     └── lib/*.a
#
# Los archivos .a se commitean al repo. Las fuentes se descargan a
# _deps_build/ (gitignoreado, no se versiona).
#
# Orden de compilación:
#   1. OpenSSL   (base TLS para curl y mosquitto)
#   2. libmodbus (sin dependencias externas)
#   3. libcurl   (--with-openssl=third_party/arm/openssl, sin zlib/nghttp2/...)
#   4. libmosquitto (--with-bundled-cjson, openssl=third_party/arm/openssl)
# =============================================================================

set -euo pipefail

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'
log_info()  { echo -e "${GREEN}[INFO]${NC} $*"; }
log_warn()  { echo -e "${YELLOW}[WARN]${NC} $*"; }
log_error() { echo -e "${RED}[ERROR]${NC} $*" >&2; }

# --- Rutas ------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_ROOT="$REPO_ROOT/_deps_build"
TP="$REPO_ROOT/third_party/arm"

# --- Toolchain ARM ----------------------------------------------------------
HOST_TRIPLE="arm-linux-gnueabihf"
CC="${HOST_TRIPLE}-gcc"
CXX="${HOST_TRIPLE}-g++"
AR="${HOST_TRIPLE}-ar"
RANLIB="${HOST_TRIPLE}-ranlib"
STRIP="${HOST_TRIPLE}-strip"

# --- Versiones --------------------------------------------------------------
OPENSSL_VER="openssl-3.3.1"
MODBUS_TAG="v3.1.10"
CURL_VER="curl-8.8.0"
CARES_VER="c-ares-1.33.1"
MOSQUITTO_TAG="v2.0.18"

# --- Prerequisitos ----------------------------------------------------------
need_cmd() { command -v "$1" &>/dev/null || { log_error "Falta: $1  (apt install $2)"; exit 1; }; }
need_cmd "$CC"   "gcc-arm-linux-gnueabihf"
need_cmd "$CXX"  "g++-arm-linux-gnueabihf"
need_cmd "$AR"   "binutils-arm-linux-gnueabihf"
need_cmd cmake   "cmake"
need_cmd autoreconf "autoconf"
need_cmd git     "git"
need_cmd make    "build-essential"
need_cmd perl    "perl"

log_info "REPO    : $REPO_ROOT"
log_info "BUILD   : $BUILD_ROOT"
log_info "DESTINO : $TP"
log_info "CC      : $CC  ($(${CC} --version 2>&1 | head -1))"

mkdir -p "$BUILD_ROOT" "$TP"

# ============================================================================
# 1. OpenSSL
# ============================================================================
build_openssl() {
    local PREFIX="$TP/openssl"
    if [[ -f "$PREFIX/lib/libssl.a" ]]; then
        log_warn "OpenSSL ya compilado → omitiendo  ($PREFIX/lib/libssl.a)"
        return 0
    fi

    log_info "=== OpenSSL ==="
    local SRC="$BUILD_ROOT/$OPENSSL_VER"
    local TARBALL="$BUILD_ROOT/${OPENSSL_VER}.tar.gz"

    if [[ ! -d "$SRC" ]]; then
        if [[ ! -f "$TARBALL" ]]; then
            log_info "Descargando $OPENSSL_VER..."
            curl -fsSL \
                "https://www.openssl.org/source/${OPENSSL_VER}.tar.gz" \
                -o "$TARBALL"
        fi
        tar -xf "$TARBALL" -C "$BUILD_ROOT"
    fi

    mkdir -p "$PREFIX"
    cd "$SRC"

    # linux-armv4 = target OpenSSL para arm-linux-gnueabihf
    ./Configure linux-armv4 \
        --cross-compile-prefix="${HOST_TRIPLE}-" \
        --prefix="$PREFIX" \
        no-shared no-tests no-docs \
        no-zlib no-comp no-engine no-dso

    make -j"$(nproc)" build_libs
    make install_dev     # solo headers + .a, sin binarios ni mans

    log_info "OpenSSL listo → $PREFIX"
    cd "$REPO_ROOT"
}

# ============================================================================
# 2. c-ares  (DNS async sin glibc NSS — requerido para binarios estáticos ARM)
# ============================================================================
build_cares() {
    local PREFIX="$TP/cares"
    if [[ -f "$PREFIX/lib/libcares.a" ]]; then
        log_warn "c-ares ya compilado → omitiendo"
        return 0
    fi

    log_info "=== c-ares ==="
    local SRC="$BUILD_ROOT/$CARES_VER"
    local TARBALL="$BUILD_ROOT/${CARES_VER}.tar.gz"

    if [[ ! -d "$SRC" ]]; then
        if [[ ! -f "$TARBALL" ]]; then
            log_info "Descargando $CARES_VER..."
            curl -fsSL \
                "https://github.com/c-ares/c-ares/releases/download/v1.33.1/${CARES_VER}.tar.gz" \
                -o "$TARBALL"
        fi
        tar -xf "$TARBALL" -C "$BUILD_ROOT"
    fi

    local BUILD="$BUILD_ROOT/cares-build"
    mkdir -p "$BUILD" "$PREFIX"
    cd "$BUILD"

    "$SRC/configure" \
        --host="$HOST_TRIPLE" \
        --prefix="$PREFIX" \
        --enable-static --disable-shared \
        --disable-tests \
        CC="$CC" AR="$AR" RANLIB="$RANLIB"

    make -j"$(nproc)"
    make install

    log_info "c-ares listo → $PREFIX"
    cd "$REPO_ROOT"
}

# ============================================================================
# 3. libmodbus
# ============================================================================
build_modbus() {
    local PREFIX="$TP/modbus"
    if [[ -f "$PREFIX/lib/libmodbus.a" ]]; then
        log_warn "libmodbus ya compilado → omitiendo"
        return 0
    fi

    log_info "=== libmodbus ==="
    local SRC="$BUILD_ROOT/libmodbus"

    if [[ ! -d "$SRC" ]]; then
        git clone --depth=1 --branch "$MODBUS_TAG" \
            https://github.com/stephane/libmodbus.git "$SRC"
    fi

    if [[ ! -f "$SRC/configure" ]]; then
        cd "$SRC" && ./autogen.sh && cd "$REPO_ROOT"
    fi

    local BUILD="$BUILD_ROOT/libmodbus-build"
    mkdir -p "$BUILD" "$PREFIX"
    cd "$BUILD"

    "$SRC/configure" \
        --host="$HOST_TRIPLE" \
        --prefix="$PREFIX" \
        --enable-static --disable-shared \
        CC="$CC" AR="$AR" RANLIB="$RANLIB"

    make -j"$(nproc)"
    make install

    log_info "libmodbus listo → $PREFIX"
    cd "$REPO_ROOT"
}

# ============================================================================
# 4. libcurl  (con c-ares para DNS estático, sin otras dependencias extras)
# ============================================================================
build_curl() {
    local PREFIX="$TP/curl"
    if [[ -f "$PREFIX/lib/libcurl.a" ]]; then
        log_warn "libcurl ya compilado → omitiendo"
        return 0
    fi

    log_info "=== libcurl ==="
    local SRC="$BUILD_ROOT/$CURL_VER"
    local TARBALL="$BUILD_ROOT/${CURL_VER}.tar.gz"

    if [[ ! -d "$SRC" ]]; then
        if [[ ! -f "$TARBALL" ]]; then
            log_info "Descargando $CURL_VER..."
            curl -fsSL \
                "https://curl.se/download/${CURL_VER}.tar.gz" \
                -o "$TARBALL"
        fi
        tar -xf "$TARBALL" -C "$BUILD_ROOT"
    fi

    local BUILD="$BUILD_ROOT/curl-build"
    mkdir -p "$BUILD" "$PREFIX"
    cd "$BUILD"

    "$SRC/configure" \
        --host="$HOST_TRIPLE" \
        --prefix="$PREFIX" \
        --enable-static --disable-shared \
        --with-openssl="$TP/openssl" \
        --enable-ares="$TP/cares" \
        --without-zlib \
        --without-nghttp2 \
        --without-brotli \
        --without-zstd \
        --without-libidn2 \
        --without-libpsl \
        --without-librtmp \
        --without-libssh2 \
        --without-nss \
        --without-gnutls \
        --disable-ldap \
        --disable-ldaps \
        --disable-rtsp \
        --disable-ftp \
        --disable-file \
        --disable-dict \
        --disable-telnet \
        --disable-tftp \
        --disable-pop3 \
        --disable-imap \
        --disable-smtp \
        --disable-gopher \
        --disable-manual \
        CC="$CC" AR="$AR" RANLIB="$RANLIB" \
        CPPFLAGS="-I$TP/openssl/include -I$TP/cares/include" \
        LDFLAGS="-L$TP/openssl/lib -L$TP/cares/lib"

    make -j"$(nproc)"
    make install

    log_info "libcurl listo → $PREFIX"
    log_info "  features: $(${BUILD}/curl-config --features 2>/dev/null || echo 'n/a')"
    cd "$REPO_ROOT"
}

# ============================================================================
# 5. libmosquitto  (cJSON bundled, OpenSSL propio)
# ============================================================================
build_mosquitto() {
    local PREFIX="$TP/mosquitto"
    if [[ -f "$PREFIX/lib/libmosquitto.a" ]]; then
        log_warn "libmosquitto ya compilado → omitiendo"
        return 0
    fi

    log_info "=== libmosquitto ==="
    local SRC="$BUILD_ROOT/mosquitto"

    if [[ ! -d "$SRC" ]]; then
        git clone --depth=1 --branch "$MOSQUITTO_TAG" \
            https://github.com/eclipse/mosquitto.git "$SRC"
    fi

    # Toolchain file para CMake
    local TOOLCHAIN="$BUILD_ROOT/armv7-toolchain.cmake"
    cat > "$TOOLCHAIN" <<EOF
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER   ${CC})
set(CMAKE_CXX_COMPILER ${CXX})
set(CMAKE_AR           ${AR})
set(CMAKE_RANLIB       ${RANLIB})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
EOF

    local BUILD="$BUILD_ROOT/mosquitto-build"
    mkdir -p "$BUILD" "$PREFIX/lib" "$PREFIX/include"
    cd "$BUILD"

    cmake "$SRC" \
        -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
        -DCMAKE_BUILD_TYPE=Release \
        -DWITH_BUNDLED_CJSON=ON \
        -DWITH_STATIC_LIBRARIES=ON \
        -DWITH_SHARED_LIBRARIES=OFF \
        -DWITH_BROKER=OFF \
        -DWITH_APPS=OFF \
        -DWITH_CLIENTS=OFF \
        -DBUILD_TESTING=OFF \
        -DOPENSSL_ROOT_DIR="$TP/openssl" \
        -DOPENSSL_INCLUDE_DIR="$TP/openssl/include" \
        -DOPENSSL_SSL_LIBRARY="$TP/openssl/lib/libssl.a" \
        -DOPENSSL_CRYPTO_LIBRARY="$TP/openssl/lib/libcrypto.a"

    cmake --build . --target libmosquitto_static -j"$(nproc)"

    # Copiar artefactos
    cp lib/libmosquitto_static.a "$PREFIX/lib/libmosquitto.a"
    cp "$SRC/include/mosquitto.h" "$PREFIX/include/"

    log_info "libmosquitto listo → $PREFIX"
    cd "$REPO_ROOT"
}

# ============================================================================
# Main
# ============================================================================
build_openssl
build_cares
build_modbus
build_curl
build_mosquitto

log_info ""
log_info "============================================"
log_info "Compilación completa. Archivos en third_party/arm/:"
find "$TP" -name "*.a" | sort | while read -r f; do
    log_info "  $(realpath --relative-to="$REPO_ROOT" "$f")  ($(du -sh "$f" | cut -f1))"
done
log_info ""
log_info "Siguiente paso:"
log_info "  git add third_party/"
log_info "  git commit -m 'chore: agregar .a ARM de dependencias (Camino A)'"
log_info "============================================"
