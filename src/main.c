/*
    compilacion cruzada
    export PREFIX_ARM="$HOME/opt/libmodbus-arm"
    export PREFIX_CURL_ARM="$HOME/opt/libcurl-arm"
    arm-linux-gnueabihf-gcc ./src/main.c ./src/sensor.c ./src/modbus_comm.c ./src/config.c ./src/persist.c ./src/http_sender.c ./src/circuit_breaker.c ./lib/cJSON.c \
    -o sensor_trident_modbus_ARM \
    -static \
    -Wall -Wextra \
    -I$PREFIX_ARM/include/modbus \
    -I$PREFIX_CURL_ARM/include \
    $PREFIX_ARM/lib/libmodbus.a \
    $PREFIX_CURL_ARM/lib/libcurl.a
    transferencia de binario al validador
    scp -P 2122 sensor_trident_modbus_ARM  root@192.168.188.29:/SD/

*/

#include <stdio.h>
#include <stdlib.h>
#include <modbus.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "../lib/cJSON.h"
#include "sensor.h"
#include "modbus_comm.h"
#include "config.h"
#include "persist.h"
#include "http_sender.h"
#include "mqtt_sender.h"
#include "circuit_breaker.h"
#include "logger.h"


/*
 * Determina la interfaz activa y el CB correspondiente según primary_interface
 * y el estado de los circuit breakers.
 *
 * Regla: si el CB de la interfaz principal está OPEN → failover a la otra.
 * Retorna "api" o "mqtt" en active_iface_out (debe tener al menos 8 bytes).
 * Retorna el CB activo en cb_out.
 */
static void select_interface(const AppConfig *cfg,
                             CircuitBreaker *cb_api,
                             CircuitBreaker *cb_mqtt,
                             const char **active_iface_out,
                             CircuitBreaker **cb_out)
{
    int primary_is_api = (strcmp(cfg->primary_interface, "api") == 0);

    if (primary_is_api)
    {
        /* Failover a MQTT solo si API está OPEN *y* MQTT está habilitado */
        if (cb_api->state == CB_OPEN && cfg->mqtt.enabled)
        {
            *active_iface_out = "mqtt";
            *cb_out           = cb_mqtt;
        }
        else
        {
            *active_iface_out = "api";
            *cb_out           = cb_api;
        }
    }
    else
    {
        /* Failover a API solo si MQTT está OPEN *y* API está habilitada */
        if (cb_mqtt->state == CB_OPEN && cfg->api.enabled)
        {
            *active_iface_out = "api";
            *cb_out           = cb_api;
        }
        else
        {
            *active_iface_out = "mqtt";
            *cb_out           = cb_mqtt;
        }
    }
}

/*
 * Recorre /SD/pending/ en orden FIFO (más antiguo primero).
 * Selecciona la interfaz activa (API o MQTT) según estado de los CBs.
 * Por cada archivo: intenta enviarlo; si OK mueve a sent/.
 * Se detiene ante el primer error.
 */
static void try_send_pending(const AppConfig *cfg,
                             CircuitBreaker *cb_api,
                             CircuitBreaker *cb_mqtt)
{
    /* Tickear el CB de la interfaz primaria para que pueda transicionar
     * OPEN → HALF_OPEN aunque estemos enviando por la interfaz de failover */
    CircuitBreaker *primary_cb =
        (strcmp(cfg->primary_interface, "api") == 0) ? cb_api : cb_mqtt;
    cb_maybe_recover(primary_cb);

    const char    *active_iface;
    CircuitBreaker *cb;
    select_interface(cfg, cb_api, cb_mqtt, &active_iface, &cb);

    /* Si la interfaz activa es MQTT y está deshabilitada, salir silenciosamente */
    if (strcmp(active_iface, "mqtt") == 0 && !cfg->mqtt.enabled)
        return;

    if (!cb_allow_send(cb))
        return;

    /* En HALF_OPEN sólo se prueba con 1 archivo */
    int max = (cb->state == CB_HALF_OPEN) ? 1 : (int)cfg->send.fifo_max_per_cycle;

    PendingFileName files[50];
    int count = persist_list_pending(cfg->persist_path, files, max);
    if (count <= 0)
        return;

    int i;
    for (i = 0; i < count; i++)
    {
        char *payload = persist_read_file(cfg->persist_path, files[i]);
        if (payload == NULL)
            continue;

        int ok = 0, transient = 0;

        if (strcmp(active_iface, "api") == 0)
        {
            HttpResult r = http_post(&cfg->api, payload);
            if      (r == HTTP_OK)           ok        = 1;
            else if (r == HTTP_ERR_TRANSIENT) transient = 1;
            else if (r == HTTP_DISABLED)      { free(payload); break; }
            /* HTTP_ERR_PERSISTENT o HTTP_ERR_CURL → persistente */
        }
        else
        {
            MqttResult r = mqtt_publish(&cfg->mqtt, payload, files[i]);
            if      (r == MQTT_OK)          ok        = 1;
            else if (r == MQTT_ERR_CONNECT ||
                     r == MQTT_ERR_PUBLISH ||
                     r == MQTT_ERR_TLS)     transient = 1;
            else if (r == MQTT_DISABLED)    { free(payload); break; }
            /* MQTT_ERR_BUILD → persistente */
        }

        free(payload);

        if (ok)
        {
            persist_move_to_sent(cfg->persist_path, cfg->persist_sent_path, files[i]);
            persist_rotate_sent(cfg->persist_sent_path, cfg->send.sent_retention_count);
            LOG_INFO("[fifo] enviado(%s) → sent/: %s", active_iface, files[i]);
            cb_on_success(cb);
        }
        else if (transient)
        {
            LOG_WARN("[fifo] error transitorio(%s) — reintentará en próximo ciclo: %s",
                     active_iface, files[i]);
            cb_on_transient_fail(cb,
                                 cfg->send.cb_fail_threshold,
                                 cfg->send.cb_open_timeout_sec,
                                 cfg->send.cb_backoff_max_sec);
            break;
        }
        else
        {
            LOG_WARN("[fifo] error persistente(%s) — deteniendo cola", active_iface);
            cb_on_persistent_fail(cb,
                                  cfg->send.cb_open_timeout_sec,
                                  cfg->send.cb_backoff_max_sec);
            break;
        }
    }
}

/*
 * Preflight: recorre argv buscando --config/-c para establecer la ruta
 * del archivo de configuración antes de cargarlo.
 * El resto de args se procesan después por config_apply_cli().
 */
static void preflight_config_path(AppConfig *cfg, int argc, char *argv[])
{
    int i;
    for (i = 1; i < argc; i++)
    {
        if ((strcmp(argv[i], "--config") == 0 || strcmp(argv[i], "-c") == 0) &&
            (i + 1) < argc)
        {
            strncpy(cfg->config_path, argv[i + 1], sizeof(cfg->config_path) - 1);
            cfg->config_path[sizeof(cfg->config_path) - 1] = '\0';
            return;
        }
    }
}

int main(int argc, char *argv[])
{
    AppConfig cfg;
    config_init(&cfg);                          /* 1. defaults */
    preflight_config_path(&cfg, argc, argv);    /* 2. detectar --config antes de leer */

    ConfigFileResult file_result = config_load_file(&cfg); /* 3. archivo JSON */
    if (file_result == CONFIG_FILE_INVALID_JSON ||
        file_result == CONFIG_FILE_INVALID_VALUE)
    {
        fprintf(stderr, "Uso: %s [--interval <s>] [--config <ruta>]\n", argv[0]);
        return -1;
    }

    if (config_apply_cli(&cfg, argc, argv) != 0) /* 4. overrides CLI */
    {
        fprintf(stderr, "Uso: %s [--interval <s>] [--config <ruta>]\n", argv[0]);
        return -1;
    }

    config_print(&cfg);

    GatewayInfo gw;
    strncpy(gw.name, cfg.gateway.name, sizeof(gw.name) - 1);
    gw.name[sizeof(gw.name) - 1] = '\0';
    strncpy(gw.sn, cfg.gateway.sn, sizeof(gw.sn) - 1);
    gw.sn[sizeof(gw.sn) - 1] = '\0';
    strncpy(gw.fw, GATEWAY_FW_VERSION, sizeof(gw.fw) - 1);
    gw.fw[sizeof(gw.fw) - 1] = '\0';

    http_global_init();

    CircuitBreaker cb_api;
    CircuitBreaker cb_mqtt;
    cb_init(&cb_api);
    cb_init(&cb_mqtt);

    uint16_t register_sensor_info[REGISTER_SENSOR_INFO_SIZE];
    uint16_t register_sensor_Data[REGISTER_SENSOR_DATA_SIZE];
    uint16_t register_sensor_all[REGISTER_SENSOR_ALL_SIZE];

    int rc;
    modbus_t *ctx; // Contexto de comunicación Modbus encapsulado y oculto en la libreria

    char time_str[26];
    // char buffer[64];
    SensorSnapshot snapshot;
    char *payload_json;

    ctx = modbus_new_rtu(cfg.serial_port, 115200, 'N', 8, 1);
    if (ctx == NULL)
    {
        fprintf(stderr, "Unable to create the libmodbus context: %s\n", modbus_strerror(errno));
        return -1;
    }

    rc = modbus_set_slave(ctx, cfg.slave_id);
    if (rc == -1)
    {
        fprintf(stderr, "Invalid slave ID: %s\n", modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }

    rc = modbus_set_response_timeout(ctx, 5, 0); // 5 segundos de timeout para la respuesta
    if (rc == -1)
    {
        fprintf(stderr, " Unable to set response timeout: %s\n", modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }

    if (modbus_connect(ctx) == -1)
    {
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }

    printf("Connected to Modbus device successfully\n");

    if (modbus_wait_first_sweep(ctx) != 0)
    {
        LOG_ERROR("sensor no alcanzó estado válido, abortando");
        modbus_close(ctx);
        modbus_free(ctx);
        exit(1);
    }

    while (1)
    {
        rc = read_block_modbus(ctx, 21, MODBUS_RANGE_SIZE(21, 32), register_sensor_info);
        if (rc == -1)
        {
            LOG_WARN("lectura info block fallida — reintentando en siguiente ciclo");
            sleep(cfg.interval_sec);
            continue;
        }
        rc = read_block_modbus(ctx, 79, MODBUS_RANGE_SIZE(79, 141), register_sensor_Data);
        if (rc == -1)
        {
            sleep(1);
            continue;
        }

        memcpy(register_sensor_all, register_sensor_info, sizeof(register_sensor_info));
        memcpy(register_sensor_all + REGISTER_SENSOR_INFO_SIZE, register_sensor_Data, sizeof(register_sensor_Data));

        for (size_t i = 0; i < dataSensor_count; i++)
        {
            uint8_t offset;
            if (dataSensor[i].ID >= 21 && dataSensor[i].ID <= 32)
                offset = dataSensor[i].ID - 21;
            else if (dataSensor[i].ID >= 79 && dataSensor[i].ID <= 141)
                offset = REGISTER_SENSOR_INFO_SIZE + (dataSensor[i].ID - 79);
            else
                continue;
            value_set(&dataSensor[i], register_sensor_all + offset);
        }

        // reporta el tiempo actual
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);

        strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);
        printf("Current time: %s ", time_str);

        // // Reporta los valores de los sensores en cada iteración
        // for (size_t i = 0; i < dataSensor_count; i++)
        // {
        //     print_sensor_test_polling(&dataSensor[i], buffer, sizeof(buffer));
        //     printf("|%s", buffer);
        // }
        // printf("|\n");

        if (build_sensor_snapshot(dataSensor, dataSensor_count, now, &snapshot) == 0)
        {
            payload_json = build_gateway_payload_json(&gw, &snapshot);
            if (payload_json != NULL)
            {
                printf("%s\n", payload_json);
                persist_write(cfg.persist_path, (long)now, payload_json);
                cJSON_free(payload_json);
                try_send_pending(&cfg, &cb_api, &cb_mqtt);
            }
            else
            {
                fprintf(stderr, "No se pudo serializar snapshot a JSON\n");
            }
        }
        else
        {
            fprintf(stderr, "No se pudo construir snapshot del sensor\n");
        }

        /* Línea de estado por ciclo: ambos CBs + archivos pendientes */
        {
            int pending = persist_list_pending(cfg.persist_path, NULL, 0);
            const char *s_api  = (cb_api.state  == CB_CLOSED) ? "CLOSED" :
                                 (cb_api.state  == CB_OPEN)   ? "OPEN"   : "HALF_OPEN";
            const char *s_mqtt = (cb_mqtt.state == CB_CLOSED) ? "CLOSED" :
                                 (cb_mqtt.state == CB_OPEN)   ? "OPEN"   : "HALF_OPEN";
            LOG_INFO("[status] primary=%s  cb_api=%s  cb_mqtt=%s  "
                     "fallos_api=%d  fallos_mqtt=%d  pendientes=%d",
                     cfg.primary_interface,
                     s_api, s_mqtt,
                     cb_api.fail_count, cb_mqtt.fail_count,
                     pending < 0 ? 0 : pending);
        }

        sleep(cfg.interval_sec);
    }

    return 0;
}
