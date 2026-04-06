# -----------------------------------------------------------------------
# QW3100 Modbus — Makefile
# Targets:
#   make           → compila binario ARM (default)
#   make arm       → idem explícito
#   make devlinux  → compila y ejecuta tests en el PC de desarrollo
#   make deploy    → arm + scp al dispositivo
#   make run       → deploy + ejecuta en el dispositivo
#   make clean     → elimina binarios generados
# -----------------------------------------------------------------------

# --- Prefijos de librerías por target -----------------------------------
PREFIX_MODBUS_ARM      ?= $(HOME)/opt/libmodbus-arm
PREFIX_CURL_ARM        ?= $(HOME)/opt/libcurl-arm
PREFIX_MODBUS_DEVLINUX ?= $(HOME)/opt/libmodbus-devlinux
PREFIX_CURL_DEVLINUX   ?= $(HOME)/opt/libcurl-devlinux

# --- Compiladores -------------------------------------------------------
CC_ARM = arm-linux-gnueabihf-gcc
CC     = gcc

# --- Flags comunes ------------------------------------------------------
CFLAGS = -Wall -Wextra

# --- Fuentes ------------------------------------------------------------
SRCS = src/main.c \
       src/sensor.c \
       src/modbus_comm.c \
       src/config.c \
       src/persist.c \
       src/http_sender.c \
       src/circuit_breaker.c \
       lib/cJSON.c

SRCS_TEST = test/main_test.c \
            src/sensor.c \
            src/modbus_comm.c \
            src/config.c \
            src/persist.c \
            src/http_sender.c \
            lib/cJSON.c

# --- Binarios de salida -------------------------------------------------
TARGET      = sensor_trident_modbus_ARM
TARGET_TEST = test/main_test

# --- Dispositivo remoto -------------------------------------------------
DEVICE      = qw3100-device
DEVICE_PATH = /SD/

# -----------------------------------------------------------------------
.PHONY: all arm devlinux deploy run clean

all: arm

# --- ARM: cross-compile, linking estático total -------------------------
arm: $(TARGET)

$(TARGET): $(SRCS)
	$(CC_ARM) $(SRCS) -o $@ \
		-static $(CFLAGS) \
		-I$(PREFIX_MODBUS_ARM)/include/modbus \
		-I$(PREFIX_CURL_ARM)/include \
		$(PREFIX_MODBUS_ARM)/lib/libmodbus.a \
		$(PREFIX_CURL_ARM)/lib/libcurl.a

# --- devlinux: nativo, linking estático con libs del script -------------
# Usa libmodbus.a y libcurl.a compiladas por setup_dependencies (devlinux)
# en lugar de -lcurl del sistema, para garantizar las mismas flags
# (sin nghttp2/zlib/brotli) y evitar "undefined reference" al linkear.
devlinux: $(TARGET_TEST)

$(TARGET_TEST): $(SRCS_TEST)
	$(CC) $(SRCS_TEST) -o $@ \
		-g -O0 $(CFLAGS) \
		-I$(PREFIX_MODBUS_DEVLINUX)/include/modbus \
		-I$(PREFIX_CURL_DEVLINUX)/include \
		$(PREFIX_MODBUS_DEVLINUX)/lib/libmodbus.a \
		$(PREFIX_CURL_DEVLINUX)/lib/libcurl.a

# --- Deploy y ejecución remota ------------------------------------------
deploy: arm
	scp $(TARGET) $(DEVICE):$(DEVICE_PATH)

run: deploy
	ssh $(DEVICE) $(DEVICE_PATH)$(TARGET)

# --- Limpieza -----------------------------------------------------------
clean:
	rm -f $(TARGET) $(TARGET_TEST)