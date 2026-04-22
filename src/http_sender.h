#ifndef HTTP_SENDER_H
#define HTTP_SENDER_H

typedef struct
{
    int  enabled;                  /* 0 = skip sending */
    char base_url[256];
    char bearer_token[2048];
    char ca_bundle_path[256];      /* Ruta al CA bundle. Vacío = usar default del sistema */
} HttpConfig;

typedef enum
{
    HTTP_OK = 0,            /* 200 recibido */
    HTTP_ERR_TRANSIENT,     /* timeout, 5xx, DNS fail */
    HTTP_ERR_PERSISTENT,    /* 400, 401, 403 */
    HTTP_ERR_CURL,          /* error interno de libcurl */
    HTTP_DISABLED           /* api.enabled = 0 */
} HttpResult;

/*
 * Envía json_payload via HTTP POST al endpoint configurado en cfg.
 * Header: Authorization: Bearer <bearer_token>.
 * Retorna HttpResult según el código HTTP o error curl.
 */
void      http_global_init(void);
HttpResult http_post(const HttpConfig *cfg, const char *json_payload);

#endif /* HTTP_SENDER_H */
