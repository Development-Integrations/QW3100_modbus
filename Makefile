# -----------------------------------------------------------------------
# QW3100 Modbus — Makefile (wrapper de Meson)
#
# Targets:
#   make           → compila binario ARM (default)
#   make arm       → idem explícito
#   make devlinux  → compila y corre tests en el PC de desarrollo
#   make test      → alias de devlinux
#   make deploy    → arm + scp al dispositivo
#   make run       → deploy + ejecuta en el dispositivo
#   make clean     → elimina directorios de build
# -----------------------------------------------------------------------

DEVICE      = qw3100-device
DEVICE_PATH = /SD/
TARGET      = sensor_trident_modbus_ARM

.PHONY: all arm devlinux test deploy run clean

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

# --- Limpieza -----------------------------------------------------------
clean:
	rm -rf build-arm/ build-devlinux/ $(TARGET)
