#ifndef HTTP_SENDER_H
#define HTTP_SENDER_H

typedef struct
{
    int  enabled;               /* 0 = skip sending */
    char base_url[256];
    char item_guid[64];
    char pull_type_guid[64];
    char scante_token[64];
    char scante_appid[64];
    char scante_sgid[64];
    char ca_bundle_path[256];   /* Ruta al CA bundle. Vacío = usar default del sistema */
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
 * Construye la URL con item_guid y pull_type_guid como query params.
 * Headers: scante_token, scante_appid, scante_utype=api_token, scante_sgid.
 * Retorna HttpResult según el código HTTP o error curl.
 */
HttpResult http_post(const HttpConfig *cfg, const char *json_payload);

#endif /* HTTP_SENDER_H */
