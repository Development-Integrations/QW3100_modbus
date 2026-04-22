## Referencia — Herramienta de diagnóstico `probe_env`

Binario C standalone que valida que las librerías estáticas ARM (libcurl + OpenSSL + c-ares)
funcionen correctamente en el dispositivo objetivo.

**Validado en RC9-930D81-2740** — resultado: HTTP 200 contra `fleet.nebulae.com.co`.

---

### Archivos

```
tools/
├── probe_env.c          ← fuente principal
└── probe_env_config.h   ← PROBE_URL y PROBE_BEARER_TOKEN hardcodeados
```

`probe_env_config.h` no está versionado (`.gitignore`). Contiene las credenciales reales.

---

### Targets

```bash
make probe          # devlinux — usa libcurl/libssl del sistema
make probe-arm      # cross-compile ARM estático
make probe-deploy   # probe-arm + scp al dispositivo
```

---

### Dependencias ARM (linking estático)

```
third_party/arm/curl/lib/libcurl.a       ← compilada con --enable-ares
third_party/arm/cares/lib/libcares.a     ← DNS sin glibc NSS
third_party/arm/openssl/lib/libssl.a     ← compilada con no-dso
third_party/arm/openssl/lib/libcrypto.a  ← compilada con no-dso
-lpthread -ldl -lz
```

c-ares y los flags `no-dso` son **obligatorios** — sin ellos el binario hace SIGSEGV en ARM.
Ver `scripts/build_third_party.sh` para los flags de compilación.

---

### Lógica de ejecución

1. Imprime reporte de entorno: arch/kernel, versión libcurl, versión OpenSSL, CA bundle encontrado
2. Hace POST idéntico a `src/http_sender.c`:
   - `CURLOPT_SSL_VERIFYPEER = 1`, `CURLOPT_SSL_VERIFYHOST = 2`
   - Header `Authorization: Bearer <PROBE_BEARER_TOKEN>`
   - `CURLOPT_CAINFO` con el primer CA bundle que exista (lista de candidatos)
   - Timeout 30s / conexión 10s
3. Imprime HTTP code recibido o error curl

**Candidatos de CA bundle** (en orden de prioridad):
```
/usr/local/share/ca-certificates/roots.pem      ← bundle local del dispositivo
/usr/local/share/ca-certificates/isrgrootx1.pem ← ISRG Root X1 (cadena LE R13)
/SD/ca-certificates.crt
/etc/ssl/certs/ca-certificates.crt              ← sistema devlinux
/etc/pki/tls/certs/ca-bundle.crt
/etc/ssl/cert.pem
```

El dispositivo tiene `/usr/local/share/ca-certificates/` con los bundles correctos de Let's Encrypt.
`/etc/ssl/certs/ca-certificates.crt` del dispositivo está desactualizado (no incluye LE R13).

---

### Salida esperada (ARM en dispositivo)

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

---

### Criterio de éxito

`HTTP 200` confirma en una sola prueba:
- DNS funcional (c-ares, sin glibc NSS)
- TLS funcional (OpenSSL `no-dso`, CA bundle válido para LE R13)
- Conectividad de red al endpoint de producción
- Bearer token aceptado por la API
