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

    /* API defaults: deshabilitada, campos vacíos */
    cfg->api.enabled = 0;
    cfg->api.base_url[0]        = '\0';
    cfg->api.item_guid[0]       = '\0';
    cfg->api.pull_type_guid[0]  = '\0';
    cfg->api.scante_token[0]    = '\0';
    cfg->api.scante_appid[0]    = '\0';
    cfg->api.scante_sgid[0]     = '\0';
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
        PARSE_STR("item_guid",      item_guid);
        PARSE_STR("pull_type_guid", pull_type_guid);
        PARSE_STR("scante_token",   scante_token);
        PARSE_STR("scante_appid",   scante_appid);
        PARSE_STR("scante_sgid",    scante_sgid);
#undef PARSE_STR
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
    printf("[config] api.enabled  : %s\n",   cfg->api.enabled ? "true" : "false");
    if (cfg->api.enabled)
        printf("[config] api.base_url : %s\n", cfg->api.base_url);
}
