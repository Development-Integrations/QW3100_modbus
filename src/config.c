#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../lib/cJSON.h"

/* Lee el contenido completo de un archivo en un buffer heap.
 * El llamador debe liberar el puntero retornado.
 * Retorna NULL en caso de error. */
static char *read_file_contents(const char *path)
{
    FILE *f = fopen(path, "r");
    if (f == NULL)
    {
        return NULL;
    }

    if (fseek(f, 0, SEEK_END) != 0)
    {
        fclose(f);
        return NULL;
    }

    long size = ftell(f);
    if (size <= 0)
    {
        fclose(f);
        return NULL;
    }

    rewind(f);

    char *buf = (char *)malloc((size_t)size + 1);
    if (buf == NULL)
    {
        fclose(f);
        return NULL;
    }

    size_t read_bytes = fread(buf, 1, (size_t)size, f);
    buf[read_bytes] = '\0';
    fclose(f);

    return buf;
}

/* ------------------------------------------------------------------ */

void config_init(AppConfig *cfg)
{
    cfg->interval_sec = CONFIG_DEFAULT_INTERVAL_SEC;
    strncpy(cfg->config_path, CONFIG_DEFAULT_PATH, sizeof(cfg->config_path) - 1);
    cfg->config_path[sizeof(cfg->config_path) - 1] = '\0';
    strncpy(cfg->serial_port, CONFIG_DEFAULT_SERIAL_PORT, sizeof(cfg->serial_port) - 1);
    cfg->serial_port[sizeof(cfg->serial_port) - 1] = '\0';
    cfg->slave_id = CONFIG_DEFAULT_SLAVE_ID;
    strncpy(cfg->persist_path, CONFIG_DEFAULT_PERSIST_PATH, sizeof(cfg->persist_path) - 1);
    cfg->persist_path[sizeof(cfg->persist_path) - 1] = '\0';
    strncpy(cfg->persist_sent_path, CONFIG_DEFAULT_PERSIST_SENT_PATH, sizeof(cfg->persist_sent_path) - 1);
    cfg->persist_sent_path[sizeof(cfg->persist_sent_path) - 1] = '\0';

    /* Gateway defaults */
    strncpy(cfg->gateway.name, "FLO-W9", sizeof(cfg->gateway.name) - 1);
    cfg->gateway.name[sizeof(cfg->gateway.name) - 1] = '\0';
    strncpy(cfg->gateway.sn, "0000", sizeof(cfg->gateway.sn) - 1);
    cfg->gateway.sn[sizeof(cfg->gateway.sn) - 1] = '\0';

    /* API defaults: deshabilitada, campos vacíos */
    cfg->api.enabled       = 0;
    cfg->api.base_url[0]   = '\0';
    cfg->api.bearer_token[0] = '\0';
    cfg->api.ca_bundle_path[0] = '\0'; /* vacío = usar CA bundle del sistema */

    /* MQTT defaults: deshabilitado, campos vacíos */
    cfg->mqtt.enabled              = 0;
    cfg->mqtt.port                 = 8883;
    cfg->mqtt.connect_timeout_sec  = 10;
    cfg->mqtt.keepalive_sec        = 60;
    cfg->mqtt.broker_url[0]        = '\0';
    cfg->mqtt.thing_name[0]        = '\0';
    cfg->mqtt.cert_path[0]         = '\0';
    cfg->mqtt.key_path[0]          = '\0';
    cfg->mqtt.ca_path[0]           = '\0';

    /* Interfaz principal */
    strncpy(cfg->primary_interface, CONFIG_DEFAULT_PRIMARY_INTERFACE,
            sizeof(cfg->primary_interface) - 1);
    cfg->primary_interface[sizeof(cfg->primary_interface) - 1] = '\0';

    /* Send / circuit breaker defaults */
    cfg->send.fifo_max_per_cycle   = CONFIG_DEFAULT_FIFO_MAX_PER_CYCLE;
    cfg->send.cb_fail_threshold    = CONFIG_DEFAULT_CB_FAIL_THRESHOLD;
    cfg->send.cb_open_timeout_sec  = CONFIG_DEFAULT_CB_OPEN_TIMEOUT;
    cfg->send.cb_backoff_max_sec   = CONFIG_DEFAULT_CB_BACKOFF_MAX;
    cfg->send.sent_retention_count = CONFIG_DEFAULT_SENT_RETENTION_COUNT;
}

ConfigFileResult config_load_file(AppConfig *cfg)
{
    char *contents = read_file_contents(cfg->config_path);
    if (contents == NULL)
    {
        /* errno == ENOENT o permiso denegado: advertir pero no abortar */
        fprintf(stderr, "[config] Archivo '%s' no encontrado, usando valores por defecto\n",
                cfg->config_path);
        return CONFIG_FILE_NOT_FOUND;
    }

    cJSON *root = cJSON_Parse(contents);
    free(contents);

    if (root == NULL)
    {
        const char *err = cJSON_GetErrorPtr();
        fprintf(stderr, "[config] JSON inválido en '%s'%s%s\n",
                cfg->config_path,
                err ? " cerca de: " : "",
                err ? err : "");
        return CONFIG_FILE_INVALID_JSON;
    }

    /* Leer "interval_sec" si existe */
    cJSON *item = cJSON_GetObjectItemCaseSensitive(root, "interval_sec");
    if (item != NULL)
    {
        if (!cJSON_IsNumber(item))
        {
            fprintf(stderr, "[config] 'interval_sec' debe ser un número entero\n");
            cJSON_Delete(root);
            return CONFIG_FILE_INVALID_VALUE;
        }

        double val = item->valuedouble;
        if (val < 1.0 || val > 65535.0)
        {
            fprintf(stderr, "[config] 'interval_sec' fuera de rango (1..65535): %.0f\n", val);
            cJSON_Delete(root);
            return CONFIG_FILE_INVALID_VALUE;
        }

        cfg->interval_sec = (uint16_t)val;
    }

    /* Leer "serial_port" si existe */
    cJSON *port_item = cJSON_GetObjectItemCaseSensitive(root, "serial_port");
    if (port_item != NULL)
    {
        if (!cJSON_IsString(port_item) || port_item->valuestring == NULL)
        {
            fprintf(stderr, "[config] 'serial_port' debe ser una cadena\n");
            cJSON_Delete(root);
            return CONFIG_FILE_INVALID_VALUE;
        }
        strncpy(cfg->serial_port, port_item->valuestring, sizeof(cfg->serial_port) - 1);
        cfg->serial_port[sizeof(cfg->serial_port) - 1] = '\0';
    }

    /* Leer "slave_id" si existe */
    cJSON *slave_item = cJSON_GetObjectItemCaseSensitive(root, "slave_id");
    if (slave_item != NULL)
    {
        if (!cJSON_IsNumber(slave_item))
        {
            fprintf(stderr, "[config] 'slave_id' debe ser un número entero\n");
            cJSON_Delete(root);
            return CONFIG_FILE_INVALID_VALUE;
        }

        double val = slave_item->valuedouble;
        if (val < 1.0 || val > 247.0)
        {
            fprintf(stderr, "[config] 'slave_id' fuera de rango (1..247): %.0f\n", val);
            cJSON_Delete(root);
            return CONFIG_FILE_INVALID_VALUE;
        }

        cfg->slave_id = (uint8_t)val;
    }

    /* Leer "persist_path" si existe */
    cJSON *persist_item = cJSON_GetObjectItemCaseSensitive(root, "persist_path");
    if (persist_item != NULL)
    {
        if (!cJSON_IsString(persist_item) || persist_item->valuestring == NULL)
        {
            fprintf(stderr, "[config] 'persist_path' debe ser una cadena\n");
            cJSON_Delete(root);
            return CONFIG_FILE_INVALID_VALUE;
        }
        strncpy(cfg->persist_path, persist_item->valuestring, sizeof(cfg->persist_path) - 1);
        cfg->persist_path[sizeof(cfg->persist_path) - 1] = '\0';
    }

    /* Leer "persist_sent_path" si existe */
    cJSON *sent_path_item = cJSON_GetObjectItemCaseSensitive(root, "persist_sent_path");
    if (sent_path_item != NULL)
    {
        if (!cJSON_IsString(sent_path_item) || sent_path_item->valuestring == NULL)
        {
            fprintf(stderr, "[config] 'persist_sent_path' debe ser una cadena\n");
            cJSON_Delete(root);
            return CONFIG_FILE_INVALID_VALUE;
        }
        strncpy(cfg->persist_sent_path, sent_path_item->valuestring, sizeof(cfg->persist_sent_path) - 1);
        cfg->persist_sent_path[sizeof(cfg->persist_sent_path) - 1] = '\0';
    }

    /* Leer objeto "api" si existe */
    cJSON *api = cJSON_GetObjectItemCaseSensitive(root, "api");
    if (api != NULL)
    {
        cJSON *enabled = cJSON_GetObjectItemCaseSensitive(api, "enabled");
        if (cJSON_IsBool(enabled))
            cfg->api.enabled = cJSON_IsTrue(enabled) ? 1 : 0;

#define PARSE_STR(key, field) \
    do { \
        cJSON *_it = cJSON_GetObjectItemCaseSensitive(api, key); \
        if (cJSON_IsString(_it) && _it->valuestring) { \
            strncpy(cfg->api.field, _it->valuestring, sizeof(cfg->api.field) - 1); \
            cfg->api.field[sizeof(cfg->api.field) - 1] = '\0'; \
        } \
    } while (0)

        PARSE_STR("base_url",       base_url);
        PARSE_STR("bearer_token",   bearer_token);
        PARSE_STR("ca_bundle_path", ca_bundle_path);
#undef PARSE_STR
    }

    /* Leer objeto "gateway" si existe */
    cJSON *gateway = cJSON_GetObjectItemCaseSensitive(root, "gateway");
    if (gateway != NULL)
    {
#define PARSE_GW_STR(key, field) \
    do { \
        cJSON *_it = cJSON_GetObjectItemCaseSensitive(gateway, key); \
        if (cJSON_IsString(_it) && _it->valuestring) { \
            strncpy(cfg->gateway.field, _it->valuestring, sizeof(cfg->gateway.field) - 1); \
            cfg->gateway.field[sizeof(cfg->gateway.field) - 1] = '\0'; \
        } \
    } while (0)

        PARSE_GW_STR("name", name);
        PARSE_GW_STR("sn",   sn);
#undef PARSE_GW_STR
    }

    /* Leer "web_interface.primary" si existe */
    cJSON *web_iface = cJSON_GetObjectItemCaseSensitive(root, "web_interface");
    if (web_iface != NULL)
    {
        cJSON *primary = cJSON_GetObjectItemCaseSensitive(web_iface, "primary");
        if (cJSON_IsString(primary) && primary->valuestring != NULL)
        {
            if (strcmp(primary->valuestring, "api") != 0 &&
                strcmp(primary->valuestring, "mqtt") != 0)
            {
                fprintf(stderr, "[config] 'web_interface.primary' debe ser \"api\" o \"mqtt\"\n");
                cJSON_Delete(root);
                return CONFIG_FILE_INVALID_VALUE;
            }
            strncpy(cfg->primary_interface, primary->valuestring,
                    sizeof(cfg->primary_interface) - 1);
            cfg->primary_interface[sizeof(cfg->primary_interface) - 1] = '\0';
        }
    }

    /* Leer objeto "mqtt" si existe */
    cJSON *mqtt = cJSON_GetObjectItemCaseSensitive(root, "mqtt");
    if (mqtt != NULL)
    {
        cJSON *enabled = cJSON_GetObjectItemCaseSensitive(mqtt, "enabled");
        if (cJSON_IsBool(enabled))
            cfg->mqtt.enabled = cJSON_IsTrue(enabled) ? 1 : 0;

        cJSON *port_item = cJSON_GetObjectItemCaseSensitive(mqtt, "port");
        if (port_item != NULL && cJSON_IsNumber(port_item))
            cfg->mqtt.port = (int)port_item->valuedouble;

        cJSON *timeout_item = cJSON_GetObjectItemCaseSensitive(mqtt, "connect_timeout_sec");
        if (timeout_item != NULL && cJSON_IsNumber(timeout_item))
            cfg->mqtt.connect_timeout_sec = (uint32_t)timeout_item->valuedouble;

        cJSON *ka_item = cJSON_GetObjectItemCaseSensitive(mqtt, "keepalive_sec");
        if (ka_item != NULL && cJSON_IsNumber(ka_item))
            cfg->mqtt.keepalive_sec = (uint32_t)ka_item->valuedouble;

#define PARSE_MQTT_STR(key, field) \
    do { \
        cJSON *_it = cJSON_GetObjectItemCaseSensitive(mqtt, key); \
        if (cJSON_IsString(_it) && _it->valuestring) { \
            strncpy(cfg->mqtt.field, _it->valuestring, sizeof(cfg->mqtt.field) - 1); \
            cfg->mqtt.field[sizeof(cfg->mqtt.field) - 1] = '\0'; \
        } \
    } while (0)

        PARSE_MQTT_STR("broker_url",  broker_url);
        PARSE_MQTT_STR("thing_name",  thing_name);
        PARSE_MQTT_STR("cert_path",   cert_path);
        PARSE_MQTT_STR("key_path",    key_path);
        PARSE_MQTT_STR("ca_path",     ca_path);
#undef PARSE_MQTT_STR
    }

    /* Leer objeto "send" si existe */
    cJSON *send = cJSON_GetObjectItemCaseSensitive(root, "send");
    if (send != NULL)
    {
#define PARSE_UINT8(key, field, min, max) \
    do { \
        cJSON *_it = cJSON_GetObjectItemCaseSensitive(send, key); \
        if (_it != NULL) { \
            if (!cJSON_IsNumber(_it) || _it->valuedouble < (min) || _it->valuedouble > (max)) { \
                fprintf(stderr, "[config] '%s' debe ser entero %d..%d\n", key, (int)(min), (int)(max)); \
                cJSON_Delete(root); \
                return CONFIG_FILE_INVALID_VALUE; \
            } \
            cfg->send.field = (uint8_t)_it->valuedouble; \
        } \
    } while (0)

#define PARSE_UINT32(key, field, min, max) \
    do { \
        cJSON *_it = cJSON_GetObjectItemCaseSensitive(send, key); \
        if (_it != NULL) { \
            if (!cJSON_IsNumber(_it) || _it->valuedouble < (min) || _it->valuedouble > (max)) { \
                fprintf(stderr, "[config] '%s' debe ser entero %d..%d\n", key, (int)(min), (int)(max)); \
                cJSON_Delete(root); \
                return CONFIG_FILE_INVALID_VALUE; \
            } \
            cfg->send.field = (uint32_t)_it->valuedouble; \
        } \
    } while (0)

        PARSE_UINT8  ("fifo_max_per_cycle",  fifo_max_per_cycle,  1,   50);
        PARSE_UINT8  ("cb_fail_threshold",   cb_fail_threshold,   1,   20);
        PARSE_UINT32 ("cb_open_timeout_sec", cb_open_timeout_sec, 10,  3600);
        PARSE_UINT32 ("cb_backoff_max_sec",  cb_backoff_max_sec,  10,  86400);

#undef PARSE_UINT8
#undef PARSE_UINT32

        /* sent_retention_count es uint16, sin macro genérica — parsear directo */
        cJSON *ret_item = cJSON_GetObjectItemCaseSensitive(send, "sent_retention_count");
        if (ret_item != NULL)
        {
            if (!cJSON_IsNumber(ret_item) ||
                ret_item->valuedouble < 1.0 || ret_item->valuedouble > 65535.0)
            {
                fprintf(stderr, "[config] 'sent_retention_count' debe ser entero 1..65535\n");
                cJSON_Delete(root);
                return CONFIG_FILE_INVALID_VALUE;
            }
            cfg->send.sent_retention_count = (uint16_t)ret_item->valuedouble;
        }
    }

    cJSON_Delete(root);
    return CONFIG_FILE_OK;
}

int config_apply_cli(AppConfig *cfg, int argc, char *argv[])
{
    int i;

    for (i = 1; i < argc; i++)
    {
        if ((strcmp(argv[i], "--interval") == 0) || (strcmp(argv[i], "-i") == 0))
        {
            if ((i + 1) >= argc)
            {
                fprintf(stderr, "[config] Falta valor para %s\n", argv[i]);
                return -1;
            }

            char *endptr = NULL;
            errno = 0;
            long value = strtol(argv[++i], &endptr, 10);

            if ((errno != 0) || (endptr == argv[i]) || (*endptr != '\0') ||
                (value < 1) || (value > 65535))
            {
                fprintf(stderr, "[config] Valor inválido para --interval: %s "
                                "(debe ser entero 1..65535)\n", argv[i]);
                return -1;
            }

            cfg->interval_sec = (uint16_t)value;
        }
        else if ((strcmp(argv[i], "--config") == 0) || (strcmp(argv[i], "-c") == 0))
        {
            /* --config ya fue consumido en el preflight de main(); consumir el valor */
            if ((i + 1) < argc) i++;
        }
        else
        {
            fprintf(stderr, "[config] Argumento no reconocido: %s\n", argv[i]);
            return -1;
        }
    }

    return 0;
}

void config_print(const AppConfig *cfg)
{
    printf("[config] interval_sec : %u s\n", cfg->interval_sec);
    printf("[config] config_path  : %s\n",   cfg->config_path);
    printf("[config] serial_port  : %s\n",   cfg->serial_port);
    printf("[config] slave_id     : %u\n",   cfg->slave_id);
    printf("[config] persist_path : %s\n",   cfg->persist_path);
    printf("[config] persist_sent : %s\n",   cfg->persist_sent_path);
    printf("[config] sent_retain  : %u archivos\n", cfg->send.sent_retention_count);
    printf("[config] gateway.name : %s\n",   cfg->gateway.name);
    printf("[config] gateway.sn   : %s\n",   cfg->gateway.sn);
    printf("[config] api.enabled  : %s\n",   cfg->api.enabled ? "true" : "false");
    if (cfg->api.enabled)
    {
        printf("[config] api.base_url : %s\n", cfg->api.base_url);
        printf("[config] api.token    : %s\n",
               cfg->api.bearer_token[0] ? "(present)" : "(empty)");
        printf("[config] api.ca_bundle: %s\n",
               cfg->api.ca_bundle_path[0] ? cfg->api.ca_bundle_path : "(sistema)");
    }
    printf("[config] primary_iface: %s\n",   cfg->primary_interface);
    printf("[config] mqtt.enabled : %s\n",   cfg->mqtt.enabled ? "true" : "false");
    if (cfg->mqtt.enabled)
    {
        printf("[config] mqtt.broker  : %s:%d\n", cfg->mqtt.broker_url, cfg->mqtt.port);
        printf("[config] mqtt.thing   : %s\n",    cfg->mqtt.thing_name);
    }
    printf("[config] fifo_max     : %u archivos/ciclo\n", cfg->send.fifo_max_per_cycle);
    printf("[config] cb_threshold : %u fallos\n",         cfg->send.cb_fail_threshold);
    printf("[config] cb_timeout   : %u s\n",              cfg->send.cb_open_timeout_sec);
    printf("[config] cb_backoff   : %u s max\n",          cfg->send.cb_backoff_max_sec);
}
