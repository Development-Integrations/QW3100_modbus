# QW3100 Modbus — Documentación

Daemon C embebido para Linux ARM. Lee el sensor QW3100 vía Modbus RTU, persiste datos localmente y los envía a la API Scante con reintentos, cola FIFO y circuit breaker.

## Documentos

| Documento | Contenido |
|-----------|-----------|
| [Arquitectura](WIKI_ARQUITECTURA.md) | Módulos, flujo de datos, máquinas de estado, decisiones de diseño |
| [Setup del entorno](WIKI_SETUP.md) | Compilar libmodbus y libcurl para ARM, alias SSH |
| [Compilación](WIKI_COMPILACION.md) | Makefile, comandos manuales, verificación del binario |
| [Deployment](WIKI_DEPLOYMENT.md) | Deploy al dispositivo, systemd, operación en producción |
| [Testing](WIKI_TESTING.md) | Tests unitarios, mock server, escenarios de validación en hardware |
| [Flujo de desarrollo](WIKI_FLUJO_DESARROLLO.md) | Ciclo de trabajo, convenciones de commit, troubleshooting |

## Inicio rápido

Ver [Readme.md](../Readme.md) en la raíz del repositorio.

## Estado del proyecto

| Fase | Descripción | Estado |
|------|-------------|--------|
| 0 | Bugs corregidos: type JSON, offset registros, reconexión, serial/slave configurables | ✅ |
| 1 | Persistencia local: `<ts>.json` en `/SD/pending/` | ✅ |
| 2 | HTTP POST a API Scante (libcurl estática) | ✅ |
| 3 | Cola FIFO + reintentos | ✅ |
| 4 | Circuit breaker + backoff exponencial | ✅ |
| 5 | Makefile, systemd unit, logger.h con timestamps | ✅ |
| 6 | Robustez Modbus: warmup EIS, reconexión verificada | ✅ |
