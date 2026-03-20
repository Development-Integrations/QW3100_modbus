// compilacion cruzada
// export PREFIX_ARM="$HOME/opt/libmodbus-arm"
// arm-linux-gnueabihf-gcc ./src/main.c ./src/sensor.c ./src/modbus_comm.c ./lib/cJSON.c -o sensor_trident_modbus_ARM -static -Wall -Wextra -I$PREFIX_ARM/include/modbus $PREFIX_ARM/lib/libmodbus.a
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
// #include "../lib/cJSON.h"
#include "sensor.h"
#include "modbus_comm.h"

#define TIME_POLLING_INTERVAL_DEFAULT 5 // Intervalo de tiempo en segundos para la lectura periódica de datos del sensor

const int REMOTE_ID = 1;

static int parse_interval_arg(int argc, char *argv[], uint16_t *interval_sec)
{
    int i;

    *interval_sec = TIME_POLLING_INTERVAL_DEFAULT;

    for (i = 1; i < argc; i++)
    {
        if ((strcmp(argv[i], "--interval") == 0) || (strcmp(argv[i], "-i") == 0))
        {
            char *endptr = NULL;
            long value;

            if ((i + 1) >= argc)
            {
                fprintf(stderr, "Falta valor para %s\n", argv[i]);
                return -1;
            }

            errno = 0;
            value = strtol(argv[i + 1], &endptr, 10);

            if ((errno != 0) || (endptr == argv[i + 1]) || (*endptr != '\0') || (value <= 0) || (value > 65535))
            {
                fprintf(stderr, "Valor invalido para --interval: %s\n", argv[i + 1]);
                return -1;
            }

            *interval_sec = (uint16_t)value;
            i++; /* Consumimos el valor */
        }
        else
        {
            fprintf(stderr, "Argumento no reconocido: %s\n", argv[i]);
            return -1;
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    uint16_t time_polling_interval;
    if (parse_interval_arg(argc, argv, &time_polling_interval) != 0)
    {
        fprintf(stderr, "Uso: %s [--interval <segundos>]\n", argv[0]);
        return -1;
    }

    uint16_t register_sensor_info[REGISTER_SENSOR_INFO_SIZE];
    uint16_t register_sensor_Data[REGISTER_SENSOR_DATA_SIZE];
    uint16_t register_sensor_all[REGISTER_SENSOR_ALL_SIZE];

    int rc;
    modbus_t *ctx; // Contexto de comunicación Modbus encapsulado y oculto en la libreria

    char time_str[26];
    char buffer[64];

    ctx = modbus_new_rtu("/dev/ttymxc2", 115200, 'N', 8, 1);
    if (ctx == NULL)
    {
        fprintf(stderr, "Unable to create the libmodbus context: %s\n", modbus_strerror(errno));
        return -1;
    }

    rc = modbus_set_slave(ctx, REMOTE_ID);
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

        uint8_t counter = 0;
        for (size_t i = 0; i < dataSensor_count; i++)
        {
            counter += value_set(&dataSensor[i], register_sensor_all + counter);
        }

        // reporta el tiempo actual
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);

        strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);
        printf("Current time: %s ", time_str);

        for (size_t i = 0; i < dataSensor_count; i++)
        {
            print_sensor_test_polling(&dataSensor[i], buffer, sizeof(buffer));
            printf("|%s", buffer);
        }
        printf("|\n");

        sleep(time_polling_interval);
    }

    return 0;
}
