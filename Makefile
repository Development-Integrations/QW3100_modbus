# -----------------------------------------------------------------------
# QW3100 Modbus — Makefile
# Targets:
#   make          → compila binario ARM (default)
#   make arm      → idem explícito
#   make native   → compila tests nativos
#   make deploy   → arm + scp al dispositivo
#   make run      → deploy + ejecuta en el dispositivo
#   make clean    → elimina binarios generados
# -----------------------------------------------------------------------

PREFIX_ARM    ?= $(HOME)/opt/libmodbus-arm
PREFIX_CURL   ?= $(HOME)/opt/libcurl-arm
PREFIX_NATIVE ?= $(HOME)/opt/libmodbus-native

CC_ARM  = arm-linux-gnueabihf-gcc
CC      = gcc
CFLAGS  = -Wall -Wextra

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

TARGET      = sensor_trident_modbus_ARM
TARGET_TEST = test/main_test

DEVICE      = qw3100-device
DEVICE_PATH = /SD/

# -----------------------------------------------------------------------

.PHONY: all arm native deploy run clean

all: arm

arm: $(TARGET)

$(TARGET): $(SRCS)
	$(CC_ARM) $(SRCS) -o $@ \
		-static $(CFLAGS) \
		-I$(PREFIX_ARM)/include/modbus \
		-I$(PREFIX_CURL)/include \
		$(PREFIX_ARM)/lib/libmodbus.a \
		$(PREFIX_CURL)/lib/libcurl.a

native: $(TARGET_TEST)

$(TARGET_TEST): $(SRCS_TEST)
	$(CC) $(SRCS_TEST) -o $@ \
		-g -O0 $(CFLAGS) \
		-I$(PREFIX_NATIVE)/include/modbus \
		-I/usr/include/x86_64-linux-gnu \
		$(PREFIX_NATIVE)/lib/libmodbus.a \
		-lcurl

deploy: arm
	scp $(TARGET) $(DEVICE):$(DEVICE_PATH)

run: deploy
	ssh $(DEVICE) $(DEVICE_PATH)$(TARGET)

clean:
	rm -f $(TARGET) $(TARGET_TEST)
