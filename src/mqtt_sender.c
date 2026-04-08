#include "mqtt_sender.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mosquitto.h>
#include "../lib/cJSON.h"

/*
 * Extrae el basename sin extensión de un path/filename.
 * Ej: "1700000001.json" → "1700000001" (escrito en out, max out_size bytes).
 */
static void basename_no_ext(const char *filename, char *out, size_t out_size)
{
    /* Tomar solo el nombre de archivo, sin path */
    const char *slash = strrchr(filename, '/');
    const char *base  = slash ? slash + 1 : filename;

    /* Copiar hasta el primer '.' o hasta out_size-1 */
    size_t i = 0;
    while (i < out_size - 1 && base[i] != '\0' && base[i] != '.')
    {
        out[i] = base[i];
        i++;
    }
    out[i] = '\0';
}

/*
 * Construye el JSON Device Shadow para AWS IoT:
 * {
 *   "state": { "reported": <canonical_root> },
 *   "clientToken": "<thing_name>-<ts>",
 *   "version": 1
 * }
 * El llamador debe liberar el puntero con cJSON_free().
 * Retorna NULL si canonical_json no es JSON válido o si falla la memoria.
 */
static char *build_shadow_payload(const MqttConfig *cfg,
                                  const char *canonical_json,
                                  const char *filename)
{
    cJSON *canonical = cJSON_Parse(canonical_json);
    if (canonical == NULL)
    {
        LOG_ERROR("[mqtt] JSON canónico inválido");
        return NULL;
    }

    /* Construir clientToken: "<thing_name>-<ts>" */
    char ts_str[64];
    basename_no_ext(filename, ts_str, sizeof(ts_str));

    char client_token[256];
    snprintf(client_token, sizeof(client_token), "%s-%s", cfg->thing_name, ts_str);

    /* Construir wrapper shadow */
    cJSON *root       = cJSON_CreateObject();
    cJSON *state      = cJSON_CreateObject();
    cJSON *reported   = cJSON_Duplicate(canonical, 1);

    cJSON_AddItemToObject(state, "reported", reported);
    cJSON_AddItemToObject(root, "state",       state);
    cJSON_AddStringToObject(root, "clientToken", client_token);
    cJSON_AddNumberToObject(root, "version",     1);

    cJSON_Delete(canonical);

    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}

/* Callback de publicación: se invoca cuando el broker confirma QoS=1 */
static volatile int s_publish_acked = 0;

static void on_publish(struct mosquitto *mosq, void *userdata, int mid)
{
    (void)mosq;
    (void)userdata;
    (void)mid;
    s_publish_acked = 1;
}

MqttResult mqtt_publish(const MqttConfig *cfg,
                        const char *canonical_json,
                        const char *filename)
{
    if (!cfg->enabled)
        return MQTT_DISABLED;

    /* 1. Construir payload shadow */
    char *shadow = build_shadow_payload(cfg, canonical_json, filename);
    if (shadow == NULL)
        return MQTT_ERR_BUILD;

    /* 2. Construir topic */
    char topic[512];
    snprintf(topic, sizeof(topic),
             "$aws/things/%s/shadow/update", cfg->thing_name);

    /* 3. Inicializar mosquitto */
    mosquitto_lib_init();

    char client_id[160];
    snprintf(client_id, sizeof(client_id), "%s-qw3100", cfg->thing_name);

    struct mosquitto *mosq = mosquitto_new(client_id, true, NULL);
    if (mosq == NULL)
    {
        LOG_ERROR("[mqtt] mosquitto_new() falló");
        cJSON_free(shadow);
        mosquitto_lib_cleanup();
        return MQTT_ERR_CONNECT;
    }

    /* 4. Configurar TLS (solo si hay al menos un CA configurado) */
    int rc;
    if (cfg->ca_path[0] != '\0')
    {
        rc = mosquitto_tls_set(mosq,
                               cfg->ca_path,
                               NULL,                                       /* capath */
                               cfg->cert_path[0] ? cfg->cert_path : NULL, /* certfile (mTLS opcional) */
                               cfg->key_path[0]  ? cfg->key_path  : NULL, /* keyfile  (mTLS opcional) */
                               NULL);                                      /* pw_callback */
        if (rc != MOSQ_ERR_SUCCESS)
        {
            LOG_ERROR("[mqtt] TLS config error: %s", mosquitto_strerror(rc));
            mosquitto_destroy(mosq);
            cJSON_free(shadow);
            mosquitto_lib_cleanup();
            return MQTT_ERR_TLS;
        }
    }
    else
    {
        LOG_WARN("[mqtt] ca_path vacío — conectando sin TLS");
    }

    /* 5. Conectar */
    rc = mosquitto_connect(mosq, cfg->broker_url, cfg->port,
                           (int)cfg->keepalive_sec);
    if (rc != MOSQ_ERR_SUCCESS)
    {
        LOG_ERROR("[mqtt] connect error: %s", mosquitto_strerror(rc));
        mosquitto_destroy(mosq);
        cJSON_free(shadow);
        mosquitto_lib_cleanup();
        return MQTT_ERR_CONNECT;
    }

    /* 6. Registrar callback de ACK y publicar con QoS=1 */
    mosquitto_publish_callback_set(mosq, on_publish);
    s_publish_acked = 0;

    rc = mosquitto_publish(mosq, NULL, topic,
                           (int)strlen(shadow), shadow, 1, false);
    if (rc != MOSQ_ERR_SUCCESS)
    {
        LOG_ERROR("[mqtt] publish error: %s", mosquitto_strerror(rc));
        mosquitto_disconnect(mosq);
        mosquitto_destroy(mosq);
        cJSON_free(shadow);
        mosquitto_lib_cleanup();
        return MQTT_ERR_PUBLISH;
    }

    /* 7. Esperar ACK del broker (loop con timeout = connect_timeout_sec) */
    int timeout_ms = (int)(cfg->connect_timeout_sec * 1000);
    int elapsed    = 0;
    while (!s_publish_acked && elapsed < timeout_ms)
    {
        rc = mosquitto_loop(mosq, 100, 1);  /* 100 ms por iteración */
        if (rc != MOSQ_ERR_SUCCESS)
        {
            LOG_WARN("[mqtt] loop error: %s", mosquitto_strerror(rc));
            break;
        }
        elapsed += 100;
    }

    MqttResult result = s_publish_acked ? MQTT_OK : MQTT_ERR_PUBLISH;

    if (result == MQTT_OK)
        LOG_INFO("[mqtt] publicado OK → %s", topic);
    else
        LOG_WARN("[mqtt] ACK no recibido en %u s", cfg->connect_timeout_sec);

    /* 8. Cleanup */
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    cJSON_free(shadow);
    mosquitto_lib_cleanup();

    return result;
}
