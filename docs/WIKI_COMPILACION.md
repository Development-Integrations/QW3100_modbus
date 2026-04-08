# Compilación

## Makefile (forma recomendada)

```bash
make          # compila binario ARM → sensor_trident_modbus_ARM
make devlinux # compila tests en el PC de desarrollo
make test     # devlinux + ejecuta tests
make deploy   # arm + scp al dispositivo
make run      # deploy + ejecuta en el dispositivo vía SSH
make clean    # elimina build-arm/ build-devlinux/ y el binario
```

El Makefile es un wrapper delgado sobre Meson. Internamente usa:
- `build-arm/` — directorio de build ARM (cross-compile)
- `build-devlinux/` — directorio de build nativo (tests)

---

## Meson directamente

```bash
# ARM
meson setup build-arm --cross-file cross/armv7.ini
meson compile -C build-arm

# devlinux (tests)
meson setup build-devlinux
meson compile -C build-devlinux
meson test -C build-devlinux
```

---

## Verificar el binario ARM

```bash
# Arquitectura correcta
file sensor_trident_modbus_ARM
# → ELF 32-bit LSB executable, ARM

# Sin dependencias dinámicas
ldd sensor_trident_modbus_ARM
# → not a dynamic executable
```

---

## Warnings esperados

Al compilar estáticamente con libcurl aparece este warning — es inofensivo:

```
warning: Using 'getaddrinfo' in statically linked applications requires
         at runtime the shared libraries from the glibc version used for linking
```

El binario funciona correctamente en el dispositivo.
