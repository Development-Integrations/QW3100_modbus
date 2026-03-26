// compilacion cruzada
// export PREFIX_ARM="$HOME/opt/libmodbus-arm"
// arm-linux-gnueabihf-gcc ./src/main.c ./src/sensor.c ./src/modbus_comm.c ./src/config.c ./lib/cJSON.c -o sensor_trident_modbus_ARM -static -Wall -Wextra -I$PREFIX_ARM/include/modbus $PREFIX_ARM/lib/libmodbus.a
// transferencia de binario al validador
// scp -P 2122 sensor_trident_modbus_ARM  root@192.168.188.39:/SD/

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

static const GatewayInfo gateway_info = {
    "AP2200-Gateway",
    100234,
    "1.20.3"
};

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

    while (1)
    {
        rc = read_block_modbus(ctx, 21, MODBUS_RANGE_SIZE(21, 32), register_sensor_info);
        if (rc == -1)
        {
            modbus_free(ctx);
            return -1;
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
            payload_json = build_gateway_payload_json(&gateway_info, &snapshot);
            if (payload_json != NULL)
            {
                printf("%s\n", payload_json);
                persist_write(cfg.persist_path, (long)now, payload_json);
                http_post(&cfg.api, payload_json);
                cJSON_free(payload_json);
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

        sleep(cfg.interval_sec);
    }

    return 0;
}
