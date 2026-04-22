# Dependencias del proyecto

El daemon usa dos conjuntos de dependencias: las precompiladas para ARM (linking estático total) y las del sistema para tests en el PC de desarrollo.

---

## Dependencias ARM (target de producción)

Compiladas desde fuente con `./scripts/build_third_party.sh`. Resultado en `third_party/arm/` (no versionado en git — ver `.gitignore`).

| Librería | Versión | Uso | Flags críticos |
|----------|---------|-----|----------------|
| **OpenSSL** | 3.3.1 | TLS para libcurl y libmosquitto | `no-shared no-engine no-dso` |
| **c-ares** | 1.33.1 | Resolución DNS sin glibc NSS | — |
| **libmodbus** | v3.1.10 | Comunicación Modbus RTU | — |
| **libcurl** | 8.8.0 | HTTP POST al endpoint fleet | `--with-openssl --enable-ares` |
| **libmosquitto** | v2.0.18 | MQTT (AWS IoT Core) | `WITH_BUNDLED_CJSON=ON` |
| **cJSON** | 1.7.19 | Serialización JSON | incluida en `lib/` del repo |

### Por qué `no-dso` en OpenSSL

OpenSSL 3.x usa una arquitectura de "providers" que por defecto carga módulos via `dlopen()` en runtime. En un binario estático ARM sobre Linux embebido, `dlopen` no puede resolver las rutas de los providers → **SIGSEGV** en el primer `curl_easy_perform`. La flag `no-dso` desactiva este mecanismo: todo queda enlazado en el binario estático.

### Por qué c-ares en lugar de glibc DNS

`getaddrinfo()` de glibc usa el sistema NSS (Name Service Switch) que carga módulos compartidos (`libnss_*.so`) en runtime. En un binario estático, estas rutas no existen → **SIGSEGV** al primer intento de resolución DNS. c-ares implementa DNS directamente con sockets, sin dependencia de glibc NSS.

### Orden de compilación

El orden importa por dependencias entre librerías:

```
OpenSSL → c-ares → libmodbus → libcurl → libmosquitto
```

libcurl necesita OpenSSL y c-ares ya compilados. libmosquitto necesita OpenSSL.

### Recompilar dependencias

Solo es necesario si `third_party/arm/` no existe (primer clone) o si se actualiza alguna versión:

```bash
./scripts/build_third_party.sh
```

El script detecta si una librería ya está compilada y la omite. Para forzar recompilación de una en particular, borrar su directorio:

```bash
rm -rf third_party/arm/curl
./scripts/build_third_party.sh   # solo recompila curl
```

---

## Dependencias devlinux (tests en el PC de desarrollo)

Paquetes del sistema — no requieren compilación manual:

```bash
sudo apt install libmodbus-dev libcurl4-openssl-dev libmosquitto-dev libssl-dev
```

Los tests en devlinux usan estas librerías compartidas (`.so`). El comportamiento es funcionalmente idéntico al binario ARM estático para la lógica de la aplicación.

---

## Dependencia incluida en el repo

| Librería | Versión | Ubicación |
|----------|---------|-----------|
| **cJSON** | 1.7.19 | `lib/cJSON.c` + `lib/cJSON.h` |

cJSON está incluida directamente en el repositorio para garantizar la misma versión en ARM y devlinux sin depender de paquetes externos.

---

## Dependencias de herramientas de build

| Herramienta | Versión mínima | Uso |
|-------------|---------------|-----|
| gcc-arm-linux-gnueabihf | cualquiera reciente | Cross-compiler ARM |
| Meson | ≥ 0.58 | Build system |
| Ninja | cualquiera | Backend de Meson |
| CMake | ≥ 3.10 | Build de libmosquitto |
| Perl | cualquiera | Requerido por OpenSSL configure |

```bash
sudo apt install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf \
                 meson ninja-build cmake perl make git curl
```

---

## Verificación rápida del entorno

```bash
# Compilador ARM disponible
arm-linux-gnueabihf-gcc --version

# Todas las .a presentes
find third_party/arm -name "*.a" | sort

# Tests pasan (devlinux)
make test

# Binario ARM compilado correctamente
make arm
file sensor_trident_modbus_ARM    # → ELF 32-bit LSB executable, ARM
ldd sensor_trident_modbus_ARM     # → not a dynamic executable
```
