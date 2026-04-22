/*
 * probe_env — valida que las librerías estáticas (curl + OpenSSL) funcionen
 * en el dispositivo ARM objetivo.
 *
 * Hace exactamente lo mismo que http_sender.c del daemon principal:
 *   curl_global_init → curl_easy_init → setopt → perform → getinfo
 *
 * Si este binario llega a HTTP 200, las .a compiladas son correctas.
 * Si crashea en el mismo punto que el daemon, el problema es de las libs,
 * no del código de la aplicación.
 *
 * make probe        → devlinux
 * make probe-arm    → ARM estático
 * make probe-deploy → ARM estático + scp
 */

#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>

#include <curl/curl.h>
#include <openssl/opensslv.h>
#include <openssl/crypto.h>

#include "probe_env_config.h"

static const char PROBE_PAYLOAD[] =
    "{\"name\":\"AP2200-Gateway\",\"sn\":100234,\"fw\":\"1.20.3\","
    "\"data\":[{\"ts\":1775503676,\"type\":\"qw\",\"uptime\":52665,"
    "\"sn\":113901568,\"fwMajor\":1,\"fwMinor\":20,\"sweepCount\":541,"
    "\"s0mag\":2186896896,\"s0phase\":-18.46,\"s0TempPre\":29.70,\"s0TempPost\":29.66,"
    "\"s1mag\":342745664,\"s1phase\":-75.35,\"s1TempPre\":29.63,\"s1TempPost\":29.63,"
    "\"s2mag\":35639284,\"s2phase\":-87.50,\"s2TempPre\":29.70,\"s2TempPost\":29.73,"
    "\"s3mag\":3708683.75,\"s3phase\":-88.81,\"s3TempPre\":29.73,\"s3TempPost\":29.70,"
    "\"s4mag\":38764.26,\"s4phase\":-81.57,\"s4TempPre\":29.70,\"s4TempPost\":29.70,"
    "\"oilTemp\":29.70,\"boardTemp\":4645,\"rh\":3121}]}";

static size_t discard_body(void *ptr, size_t size, size_t nmemb, void *ud)
{
    (void)ptr; (void)ud;
    return size * nmemb;
}

int main(void)
{
    struct utsname uts;
    uname(&uts);

    /* Reporte de entorno — antes del POST */
    printf("=== probe_env ===\n");
    printf("[sistema]  arch=%s  kernel=%s %s\n",
           uts.machine, uts.sysname, uts.release);
    printf("[libcurl]  %s\n", curl_version());
    printf("[OpenSSL]  build=%s\n", OPENSSL_VERSION_TEXT);
    printf("[OpenSSL]  runtime=%s\n", OpenSSL_version(OPENSSL_VERSION));

    /* CA bundle */
    const char *ca_candidates[] = {
        "/usr/local/share/ca-certificates/roots.pem",     /* bundle local del dispositivo */
        "/usr/local/share/ca-certificates/isrgrootx1.pem",/* ISRG Root X1 — cadena LE R13 */
        "/SD/ca-certificates.crt",                        /* copiado desde RPi */
        "/etc/ssl/certs/ca-certificates.crt",             /* sistema (devlinux) */
        "/etc/pki/tls/certs/ca-bundle.crt",
        "/etc/ssl/cert.pem",
        NULL
    };
    const char *ca_bundle = NULL;
    {
        int i;
        for (i = 0; ca_candidates[i] != NULL; i++) {
            FILE *f = fopen(ca_candidates[i], "r");
            if (f) { fclose(f); ca_bundle = ca_candidates[i]; break; }
        }
    }
    printf("[CA bundle] %s\n\n",
           ca_bundle ? ca_bundle : "NOT FOUND — TLS verificará con bundle interno de libcurl");

    /* POST — idéntico a http_sender.c */
    printf("POST %s ...\n", PROBE_URL);

    curl_global_init(CURL_GLOBAL_ALL);

    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "ERROR: curl_easy_init() falló\n");
        return 1;
    }

    static char h_auth[4096];
    snprintf(h_auth, sizeof(h_auth), "Authorization: Bearer %s", PROBE_BEARER_TOKEN);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, h_auth);

    curl_easy_setopt(curl, CURLOPT_URL,           PROBE_URL);
    curl_easy_setopt(curl, CURLOPT_POST,           1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,     PROBE_PAYLOAD);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,     headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  discard_body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        30L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    if (ca_bundle)
        curl_easy_setopt(curl, CURLOPT_CAINFO, ca_bundle);

    CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    if (res != CURLE_OK) {
        fprintf(stderr, "CURL ERROR (%d): %s\n", res, curl_easy_strerror(res));
        return 1;
    }

    printf("HTTP %ld — %s\n", http_code,
           http_code == 200 ? "OK — librerías estáticas validadas" :
           http_code  >= 500 ? "5xx servidor" :
           http_code == 401 ? "401 token inválido/expirado" :
           http_code == 403 ? "403 sin permisos" : "inesperado");

    return (http_code == 200) ? 0 : 1;
}
