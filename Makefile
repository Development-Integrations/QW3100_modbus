# -----------------------------------------------------------------------
# QW3100 Modbus — Makefile (wrapper de Meson)
#
# Targets:
#   make              → compila binario ARM (default)
#   make arm          → idem explícito
#   make devlinux     → compila y corre tests en el PC de desarrollo
#   make test         → alias de devlinux
#   make deploy       → arm + scp al dispositivo
#   make run          → deploy + ejecuta en el dispositivo
#   make probe        → compila probe_env para devlinux
#   make probe-arm    → compila probe_env_ARM para ARM (cross-compile)
#   make probe-deploy → probe-arm + scp al dispositivo
#   make clean        → elimina directorios de build
# -----------------------------------------------------------------------

DEVICE      = FLO-W9-YYYY
DEVICE_PATH = /SD/
TARGET      = sensor_trident_modbus_ARM
PROBE       = probe_env
PROBE_ARM   = probe_env_ARM

ARM_CC      = arm-linux-gnueabihf-gcc
TP_ARM      = third_party/arm

.PHONY: all arm devlinux test deploy run clean probe probe-arm probe-deploy

all: arm

# --- ARM: cross-compile via Meson ---------------------------------------
arm:
	meson setup build-arm --cross-file cross/armv7.ini --wipe 2>/dev/null || \
	meson setup build-arm --cross-file cross/armv7.ini
	meson compile -C build-arm
	cp build-arm/$(TARGET) .

# --- devlinux: nativo, compila y ejecuta tests -------------------------
devlinux:
	meson setup build-devlinux --wipe 2>/dev/null || \
	meson setup build-devlinux
	meson compile -C build-devlinux

test: devlinux
	meson test -C build-devlinux --print-errorlogs

# --- Deploy y ejecución remota -----------------------------------------
deploy: arm
	scp $(TARGET) $(DEVICE):$(DEVICE_PATH)

run: deploy
	ssh $(DEVICE) $(DEVICE_PATH)$(TARGET)

# --- probe_env: herramienta de diagnóstico TLS/curl --------------------
probe:
	gcc -Wall -Wextra -O2 \
	    -Itools \
	    tools/probe_env.c \
	    -o $(PROBE) \
	    -lcurl -lssl -lcrypto -lpthread -ldl
	@echo "→ binario: ./$(PROBE)"

probe-arm:
	$(ARM_CC) -Wall -Wextra -O2 \
	    -Itools \
	    -I$(TP_ARM)/curl/include \
	    -I$(TP_ARM)/openssl/include \
	    tools/probe_env.c \
	    -o $(PROBE_ARM) \
	    -static \
	    $(TP_ARM)/curl/lib/libcurl.a \
	    $(TP_ARM)/cares/lib/libcares.a \
	    $(TP_ARM)/openssl/lib/libssl.a \
	    $(TP_ARM)/openssl/lib/libcrypto.a \
	    -lpthread -ldl -lz
	@echo "→ binario: ./$(PROBE_ARM)"

probe-deploy: probe-arm
	scp $(PROBE_ARM) $(DEVICE):$(DEVICE_PATH)
	@echo "→ copiado a $(DEVICE):$(DEVICE_PATH)$(PROBE_ARM)"

# --- Limpieza -----------------------------------------------------------
clean:
	rm -rf build-arm/ build-devlinux/ $(TARGET) $(PROBE) $(PROBE_ARM)
