#!/bin/bash
# =============================================================================
# setup_dependencies_Sensor_Trident.sh
#
# Instala libmodbus y libcurl estáticas para el target seleccionado.
#
# Uso:
#   ./setup_dependencies_Sensor_Trident.sh arm       # Cross-compile armv7l
#   ./setup_dependencies_Sensor_Trident.sh devlinux  # Compilación y ejecución local en el dev machine
#
# Requisitos cross:
#   sudo apt install gcc-arm-linux-gnueabihf binutils-arm-linux-gnueabihf
#
# Targets:
#   arm      → cross-compile desde devlinux, produce binario para ARM embebido
#   devlinux → compila y ejecuta en el mismo dispositivo de desarrollo (x86_64)
#
# Cada target usa su propio build dir (build-arm / build-devlinux) para
# evitar mezcla de objetos .o entre arquitecturas ("file in wrong format").
# =============================================================================

set -euo pipefail

# -----------------------------------------------------------------------------
# Colores para output
# -----------------------------------------------------------------------------
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info()  { echo -e "${GREEN}[INFO]${NC} $*"; }
log_warn()  { echo -e "${YELLOW}[WARN]${NC} $*"; }
log_error() { echo -e "${RED}[ERROR]${NC} $*" >&2; }

# -----------------------------------------------------------------------------
# Argumento de target
# -----------------------------------------------------------------------------
TARGET="${1:-}"

if [[ "$TARGET" != "arm" && "$TARGET" != "devlinux" ]]; then
    log_error "Target no válido. Uso: $0 [arm|devlinux]"
    exit 1
fi

# -----------------------------------------------------------------------------
# Rutas base
# -----------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
LIB_DIR="$REPO_ROOT/lib"

# -----------------------------------------------------------------------------
# Configuración por target
# -----------------------------------------------------------------------------
if [[ "$TARGET" == "arm" ]]; then
    CC_HOST="arm-linux-gnueabihf"
    CC="${CC_HOST}-gcc"
    AR="${CC_HOST}-ar"
    RANLIB="${CC_HOST}-ranlib"
    LD="${CC_HOST}-ld"
    STRIP="${CC_HOST}-strip"
    PREFIX_MODBUS="$HOME/opt/libmodbus-arm"
    PREFIX_CURL="$HOME/opt/libcurl-arm"
else
    CC_HOST=""
    CC="gcc"
    AR="ar"
    RANLIB="ranlib"
    LD="ld"
    STRIP="strip"
    PREFIX_MODBUS="$HOME/opt/libmodbus-devlinux"
    PREFIX_CURL="$HOME/opt/libcurl-devlinux"
fi

export CC AR RANLIB LD STRIP

log_info "Target     : $TARGET"
log_info "Compiler   : $CC"
log_info "AR         : $AR"
log_info "RANLIB     : $RANLIB"
log_info "libmodbus  → $PREFIX_MODBUS"
log_info "libcurl    → $PREFIX_CURL"

# -----------------------------------------------------------------------------
# Validar prerequisitos del toolchain
# -----------------------------------------------------------------------------
check_cmd() {
    if ! command -v "$1" &>/dev/null; then
        log_error "Comando no encontrado: $1"
        log_error "Instalar con: sudo apt install $2"
        exit 1
    fi
}

check_cmd "$CC"       "gcc  o  gcc-arm-linux-gnueabihf"
check_cmd "$AR"       "binutils  o  binutils-arm-linux-gnueabihf"
check_cmd "autoconf"  "autoconf"
check_cmd "automake"  "automake"
check_cmd "libtool"   "libtool"
check_cmd "git"       "git"

# -----------------------------------------------------------------------------
# Dependencias del sistema
# -----------------------------------------------------------------------------
log_info "Instalando dependencias del sistema..."
sudo apt-get update -q
sudo apt-get install -y --no-install-recommends \
    build-essential \
    gcc-arm-linux-gnueabihf \
    binutils-arm-linux-gnueabihf \
    autoconf \
    automake \
    libtool \
    git \
    pkg-config \
    libpsl-dev \
    perl

# -----------------------------------------------------------------------------
# Función auxiliar: flags de cross-compile
# devlinux compila nativo → sin flags extra
# arm       compila cruzado → agrega --host, CC, AR, RANLIB del toolchain ARM
# -----------------------------------------------------------------------------
cross_flags() {
    if [[ -n "$CC_HOST" ]]; then
        echo "--host=$CC_HOST CC=$CC AR=$AR RANLIB=$RANLIB"
    fi
}

# =============================================================================
# BUILD libmodbus
# Bootstrap: ./autogen.sh   (estándar de libmodbus desde git)
#
# Usa build dir separado por target para evitar mezcla de objetos .o
# entre arquitecturas.
# Estructura:
#   lib/libmodbus/             ← source (compartido, solo lectura en build)
#   lib/libmodbus/build-arm/   ← objetos ARM
#   lib/libmodbus/build-devlinux/ ← objetos x86_64
# =============================================================================
build_modbus() {
    local SRC_DIR="$LIB_DIR/libmodbus"
    local BUILD_DIR="$SRC_DIR/build-$TARGET"

    log_info "==============================="
    log_info "Compilando : libmodbus  [target: $TARGET]"
    log_info "Destino    : $PREFIX_MODBUS"

    # Si el .a ya existe para este target, no recompilar
    if [ -f "$PREFIX_MODBUS/lib/libmodbus.a" ]; then
        log_info "libmodbus.a ya existe para target '$TARGET' — omitiendo build."
        log_info "  → $PREFIX_MODBUS/lib/libmodbus.a"
        log_info "  (Borra $PREFIX_MODBUS para forzar recompilación)"
        return 0
    fi

    # Clonar fuente si no existe
    if [ ! -d "$SRC_DIR" ]; then
        log_info "Clonando libmodbus..."
        git clone --depth=1 https://github.com/stephane/libmodbus.git "$SRC_DIR"
    else
        log_warn "libmodbus fuente ya existe en $SRC_DIR — omitiendo clone."
    fi

    # Generar sistema de build (solo si falta configure)
    if [ ! -f "$SRC_DIR/configure" ]; then
        cd "$SRC_DIR"
        ./autogen.sh
        cd "$REPO_ROOT"
    fi

    # Build dir aislado por target — evita mezcla de .o entre arquitecturas
    mkdir -p "$BUILD_DIR"
    mkdir -p "$PREFIX_MODBUS"

    cd "$BUILD_DIR"
    # shellcheck disable=SC2046
    "$SRC_DIR/configure" \
        --prefix="$PREFIX_MODBUS" \
        --enable-static \
        --disable-shared \
        $(cross_flags)

    make -j"$(nproc)"
    make install
    cd "$REPO_ROOT"
}

# =============================================================================
# BUILD libcurl
#
# Usa build dir separado por target para evitar mezcla de objetos .o
# entre arquitecturas ("file in wrong format" al mezclar arm/x86_64).
# Estructura:
#   lib/curl/                  ← source (compartido)
#   lib/curl/build-arm/        ← objetos ARM
#   lib/curl/build-devlinux/   ← objetos x86_64
#
# FIXES acumulados:
#   [FIX 1] autoreconf -fi en lugar de ./autogen.sh (curl no tiene autogen.sh)
#   [FIX 2] Detección de source corrupto (falta configure.ac → reclona)
#   [FIX 3] --with-openssl=no eliminado (flag inválida)
#   [FIX 4] AR y RANLIB exportados para cross-compile
#   [FIX 5] Build dir por target → resuelve "file in wrong format"
# =============================================================================
build_curl() {
    local SRC_DIR="$LIB_DIR/curl"
    local BUILD_DIR="$SRC_DIR/build-$TARGET"

    log_info "==============================="
    log_info "Compilando : libcurl  [$TARGET]"
    log_info "Destino    : $PREFIX_CURL"

    # Si el .a ya existe, verificar que fue compilado sin dependencias no deseadas.
    # Un .a viejo puede tener nghttp2/zlib/brotli/zstd/idn2 embebidos, lo que
    # causa "undefined reference" al linkear el binario final en modo estático.
    if [ -f "$PREFIX_CURL/lib/libcurl.a" ]; then
        local DIRTY=0
        for sym in nghttp2 inflate BrotliDecoder ZSTD_ idn2_; do
            if nm "$PREFIX_CURL/lib/libcurl.a" 2>/dev/null | grep -q "U ${sym}"; then
                log_warn "libcurl.a contiene símbolo externo no deseado: ${sym}*"
                DIRTY=1
            fi
        done

        if [ "$DIRTY" -eq 0 ]; then
            log_info "libcurl.a OK para target '$TARGET' — omitiendo build."
            log_info "  → $PREFIX_CURL/lib/libcurl.a"
            log_info "  (Borra $PREFIX_CURL para forzar recompilación)"
            return 0
        else
            log_warn "libcurl.a contiene dependencias externas — recompilando limpio..."
            rm -rf "$PREFIX_CURL"
            rm -rf "$BUILD_DIR"
        fi
    fi

    # FIX 2: detectar source corrupto → reclona
    if [ -d "$SRC_DIR" ] && [ ! -f "$SRC_DIR/configure.ac" ]; then
        log_warn "Source $SRC_DIR incompleto (falta configure.ac) — reclonando..."
        rm -rf "$SRC_DIR"
    fi

    # Clonar fuente si no existe
    if [ ! -d "$SRC_DIR" ]; then
        log_info "Clonando curl..."
        git clone --depth=1 https://github.com/curl/curl.git "$SRC_DIR"
    else
        log_warn "curl fuente ya existe en $SRC_DIR — omitiendo clone."
    fi

    # FIX 1: curl desde git requiere autoreconf -fi, no ./autogen.sh
    # Solo regenerar si falta el configure
    if [ ! -f "$SRC_DIR/configure" ]; then
        log_info "Regenerando sistema de build (autoreconf -fi)..."
        cd "$SRC_DIR"
        autoreconf -fi
        cd "$REPO_ROOT"
    fi

    # FIX 5: build dir aislado por target — resuelve "file in wrong format"
    # El configure de curl soporta build fuera del source tree (out-of-tree build)
    mkdir -p "$BUILD_DIR"
    mkdir -p "$PREFIX_CURL"

    cd "$BUILD_DIR"

    # Flags mínimas para HTTP-only sin TLS
    # Apropiado para comunicación en red local (sensor → gateway)
    # shellcheck disable=SC2046
    "$SRC_DIR/configure" \
        --prefix="$PREFIX_CURL" \
        --enable-static \
        --disable-shared \
        --without-ssl \
        --without-libpsl \
        --without-zstd \
        --without-brotli \
        --without-zlib \
        --without-libidn2 \
        --without-nghttp2 \
        --disable-ftp \
        --disable-file \
        --disable-ldap \
        --disable-telnet \
        --disable-dict \
        --disable-tftp \
        --disable-pop3 \
        --disable-imap \
        --disable-smtp \
        --disable-gopher \
        --disable-mqtt \
        --disable-manual \
        --disable-netrc \
        --disable-ipv6 \
        --disable-doh \
        --disable-mime \
        --disable-cookies \
        --disable-progress-meter \
        $(cross_flags)

    make -j"$(nproc)"
    make install
    cd "$REPO_ROOT"
}

# -----------------------------------------------------------------------------
# Ejecutar builds
# -----------------------------------------------------------------------------
build_modbus

if [ -f "$PREFIX_MODBUS/lib/libmodbus.a" ]; then
    log_info "OK: $PREFIX_MODBUS/lib/libmodbus.a"
else
    log_error "No se encontró libmodbus.a en $PREFIX_MODBUS/lib/"
    exit 1
fi

build_curl

if [ -f "$PREFIX_CURL/lib/libcurl.a" ]; then
    log_info "OK: $PREFIX_CURL/lib/libcurl.a"
else
    log_error "No se encontró libcurl.a en $PREFIX_CURL/lib/"
    exit 1
fi

# -----------------------------------------------------------------------------
# Resumen final
# -----------------------------------------------------------------------------
log_info "==============================="
log_info "Dependencias instaladas para target: $TARGET"
log_info ""
log_info "  libmodbus : $PREFIX_MODBUS"
log_info "  libcurl   : $PREFIX_CURL"
log_info ""
log_info "Variables para Makefile / CMakeLists.txt:"
log_info "  MODBUS_INC = $PREFIX_MODBUS/include/modbus"
log_info "  MODBUS_LIB = $PREFIX_MODBUS/lib/libmodbus.a"
log_info "  CURL_INC   = $PREFIX_CURL/include"
log_info "  CURL_LIB   = $PREFIX_CURL/lib/libcurl.a"
log_info "==============================="