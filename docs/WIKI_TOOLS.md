# Herramientas de diagnóstico

Binarios standalone para validar el entorno de ejecución y las librerías estáticas ARM, independientes del daemon principal.

---

## `probe_env` — Validación de librerías estáticas

Herramienta que confirma que libcurl + OpenSSL + c-ares compiladas estáticamente para ARM funcionan correctamente en el dispositivo objetivo: DNS, TLS y conectividad HTTP de extremo a extremo.

**Validado en RC9-930D81-2740** — resultado: HTTP 200 contra `fleet.nebulae.com.co`.

### Archivos

```
tools/
├── probe_env.c          ← fuente principal
└── probe_env_config.h   ← PROBE_URL y PROBE_BEARER_TOKEN (no versionado)
```

`probe_env_config.h` no se versiona (`.gitignore`). Contiene las credenciales reales del endpoint de producción.

### Compilar y desplegar

```bash
make probe          # devlinux — usa libcurl/libssl del sistema
make probe-arm      # cross-compile ARM estático
make probe-deploy   # probe-arm + scp al dispositivo
```

### Dependencias ARM

El binario ARM se linka completamente estático contra:

| Librería | Ruta | Flag crítico |
|----------|------|--------------|
| libcurl | `third_party/arm/curl/lib/libcurl.a` | compilada con `--enable-ares` |
| libcares | `third_party/arm/cares/lib/libcares.a` | DNS sin glibc NSS |
| libssl | `third_party/arm/openssl/lib/libssl.a` | compilada con `no-dso` |
| libcrypto | `third_party/arm/openssl/lib/libcrypto.a` | compilada con `no-dso` |

**c-ares y `no-dso` son obligatorios.** Sin ellos el binario hace SIGSEGV en ARM:
- `no-dso`: OpenSSL 3.x intenta `dlopen` providers en runtime → crash en binario estático
- c-ares: `getaddrinfo` de glibc NSS crashea en binarios estáticos ARM

Ver `scripts/build_third_party.sh` para los flags de compilación exactos.

### Lógica de ejecución

El binario hace exactamente lo mismo que `src/http_sender.c` del daemon:

1. Imprime reporte de entorno: arch/kernel, versión libcurl, versión OpenSSL, CA bundle encontrado
2. POST al endpoint de producción:
   - `CURLOPT_SSL_VERIFYPEER = 1`, `CURLOPT_SSL_VERIFYHOST = 2` (verificación TLS estricta)
   - Header `Authorization: Bearer <PROBE_BEARER_TOKEN>`
   - `CURLOPT_CAINFO` con el primer CA bundle disponible (lista de candidatos)
   - Timeout 30s total / 10s conexión
3. Imprime HTTP code recibido o descripción del error curl

### CA bundle — candidatos en orden de prioridad

```
/usr/local/share/ca-certificates/roots.pem       ← bundle completo del dispositivo
/usr/local/share/ca-certificates/isrgrootx1.pem  ← ISRG Root X1 (cadena LE R13)
/SD/ca-certificates.crt
/etc/ssl/certs/ca-certificates.crt               ← sistema devlinux
/etc/pki/tls/certs/ca-bundle.crt
/etc/ssl/cert.pem
```

El dispositivo tiene `/usr/local/share/ca-certificates/` con los bundles correctos de Let's Encrypt. El `/etc/ssl/certs/ca-certificates.crt` del dispositivo está desactualizado y no incluye la cadena Let's Encrypt R13.

### Salida esperada en dispositivo ARM

```
=== probe_env ===
[sistema]  arch=armv7l  kernel=Linux 4.x.x
[libcurl]  libcurl/8.x.x OpenSSL/3.x.x zlib/1.x.x c-ares/1.33.1 ...
[OpenSSL]  build=OpenSSL 3.x.x ...
[OpenSSL]  runtime=OpenSSL 3.x.x ...
[CA bundle] /usr/local/share/ca-certificates/roots.pem

POST https://fleet.nebulae.com.co/api/external-system-gateway/rest/status ...
HTTP 200 — OK — librerías estáticas validadas
```

### Criterio de éxito

`HTTP 200` confirma en una sola prueba:

| Componente | Qué valida |
|------------|------------|
| DNS (c-ares) | Resolución de nombres sin glibc NSS |
| TLS (OpenSSL `no-dso`) | Handshake sin `dlopen` de providers |
| CA bundle | Cadena Let's Encrypt R13 → ISRG Root X1 verificada |
| Red | Conectividad TCP al endpoint de producción |
| Autenticación | Bearer token aceptado por la API |

Si `curl_easy_perform` retorna error antes de obtener HTTP code, el binario imprime `CURL ERROR (N): <descripción>` y sale con código 1. El HTTP code solo se imprime si el servidor realmente respondió.
