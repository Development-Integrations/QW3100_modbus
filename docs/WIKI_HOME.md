# QW3100 Modbus — Documentación

Daemon C embebido para Linux ARM. Lee el sensor QW3100 vía Modbus RTU, persiste datos localmente y los envía a la API Scante o AWS IoT (MQTT) con reintentos, cola FIFO y circuit breaker.

## Documentos

| Documento | Contenido |
|-----------|-----------|
| [Arquitectura](WIKI_ARQUITECTURA.md) | Módulos, flujo de datos, máquinas de estado, decisiones de diseño |
| [Setup del entorno](WIKI_SETUP.md) | Compilar dependencias ARM, paquetes devlinux, alias SSH |
| [Compilación](WIKI_COMPILACION.md) | Makefile/Meson, verificación del binario |
| [Deployment](WIKI_DEPLOYMENT.md) | Deploy al dispositivo, systemd, operación en producción |
| [Testing](WIKI_TESTING.md) | Tests unitarios, mock server MQTT/HTTP, escenarios de validación |
| [Flujo de desarrollo](WIKI_FLUJO_DESARROLLO.md) | Ciclo de trabajo, convenciones de commit, troubleshooting |

## Inicio rápido

Ver [Readme.md](../Readme.md) en la raíz del repositorio.

## Estado del proyecto

| Fase | Descripción | Estado |
|------|-------------|--------|
| 0 | Bugs corregidos: type JSON, offset registros, reconexión, serial/slave configurables | ✅ |
| 1 | Persistencia local: `<ts>.json` en `/SD/pending/` | ✅ |
| 2 | HTTP POST a API Scante (libcurl estática, TLS) | ✅ |
| 3 | Cola FIFO + reintentos + directorio `sent/` con rotación | ✅ |
| 4 | Circuit breaker + backoff exponencial (API y MQTT independientes) | ✅ |
| 5 | Makefile/Meson, systemd unit, logger.h con timestamps | ✅ |
| 6 | Robustez Modbus: warmup EIS, reconexión verificada | ✅ |
| 7 | MQTT dual con failover: AWS IoT Device Shadow, TLS mTLS | ✅ |
| 8 | Build system: Meson + third_party/ (deps ARM sin multiarch) | ✅ |
