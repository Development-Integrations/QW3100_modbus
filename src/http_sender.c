#include "http_sender.h"

#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

/* Descarta el cuerpo de la respuesta HTTP — solo nos interesa el código */
static size_t discard_response(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    (void)ptr;
    (void)userdata;
    return size * nmemb;
}

HttpResult http_post(const HttpConfig *cfg, const char *json_payload)
{
    if (!cfg->enabled)
        return HTTP_DISABLED;

    /* Construir URL con query params */
    char url[512];
    int n = snprintf(url, sizeof(url),
                     "%s?item_guid=%s&pull_type_guid=%s",
                     cfg->base_url, cfg->item_guid, cfg->pull_type_guid);
    if (n < 0 || (size_t)n >= sizeof(url))
    {
        fprintf(stderr, "[http] URL demasiado larga\n");
        return HTTP_ERR_CURL;
    }

    CURL *curl = curl_easy_init();
    if (curl == NULL)
    {
        fprintf(stderr, "[http] curl_easy_init() falló\n");
        return HTTP_ERR_CURL;
    }

    /* Headers */
    struct curl_slist *headers = NULL;
    char h_token[128], h_appid[128], h_sgid[128];

    snprintf(h_token, sizeof(h_token), "scante_token: %s",  cfg->scante_token);
    snprintf(h_appid, sizeof(h_appid), "scante_appid: %s",  cfg->scante_appid);
    snprintf(h_sgid,  sizeof(h_sgid),  "scante_sgid: %s",   cfg->scante_sgid);

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, h_token);
    headers = curl_slist_append(headers, h_appid);
    headers = curl_slist_append(headers, "scante_utype: api_token");
    headers = curl_slist_append(headers, h_sgid);

    curl_easy_setopt(curl, CURLOPT_URL,            url);
    curl_easy_setopt(curl, CURLOPT_POST,            1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,      json_payload);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,      headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,   discard_response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,         30L);   /* 30 s total */
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT,  10L);   /* 10 s conexión */

    CURLcode res = curl_easy_perform(curl);

    HttpResult result;

    if (res != CURLE_OK)
    {
        fprintf(stderr, "[http] curl error: %s\n", curl_easy_strerror(res));

        /* Timeout o DNS fail → transitorio */
        if (res == CURLE_OPERATION_TIMEDOUT ||
            res == CURLE_COULDNT_RESOLVE_HOST ||
            res == CURLE_COULDNT_CONNECT)
        {
            result = HTTP_ERR_TRANSIENT;
        }
        else
        {
            result = HTTP_ERR_CURL;
        }
    }
    else
    {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        if (http_code == 200)
        {
            printf("[http] POST OK (200)\n");
            result = HTTP_OK;
        }
        else if (http_code >= 500)
        {
            fprintf(stderr, "[http] Error transitorio HTTP %ld\n", http_code);
            result = HTTP_ERR_TRANSIENT;
        }
        else if (http_code == 400 || http_code == 401 || http_code == 403)
        {
            fprintf(stderr, "[http] Error persistente HTTP %ld — revisar credenciales/config\n",
                    http_code);
            result = HTTP_ERR_PERSISTENT;
        }
        else
        {
            fprintf(stderr, "[http] HTTP %ld inesperado\n", http_code);
            result = HTTP_ERR_TRANSIENT;
        }
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return result;
}
