// compilacion cruzada
// export PREFIX_ARM="$HOME/opt/libmodbus-arm"
// arm-linux-gnueabihf-gcc main.c -o sensor_trident_modbus -static -I$PREFIX_ARM/include/modbus $PREFIX_ARM/lib/libmodbus.a
// transferencia de binario al validador
// scp sensor_trident_modbus root@192.168.188.39:/SD/

#include <stdio.h>
#include <stdlib.h>
#include <modbus.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <time.h>
#include "cJSON.h"
#include "cJSON.c"

// --- ESTRUCTURAS ---
typedef struct {
    float s[5][4]; // Muestras s0-s4, cada una con 4 variables
    float oilTemp;
} SensorData;

// --- PROTOTIPOS DE FUNCIONES ---
int leer_datos_sensor(SensorData *dest);
char* generar_reporte_json(SensorData *data);
void guardar_en_disco(const char *json_str, const char *ruta);

// --- IMPLEMENTACIÓN DE FUNCIONES ---

int leer_datos_sensor(SensorData *dest) {
    uint16_t registros[60];
    modbus_t *ctx = modbus_new_rtu("/dev/ttymxc2", 115200, 'N', 8, 1);
    if (!ctx) return -1;

    modbus_set_slave(ctx, 1);
    modbus_set_response_timeout(ctx, 5, 0);

    if (modbus_connect(ctx) == -1) {
        modbus_free(ctx);
        return -1;
    }

    // Lectura de 60 registros desde la dirección 80
    int rc = modbus_read_registers(ctx, 80, 60, registros);
    modbus_close(ctx);
    modbus_free(ctx);

    if (rc == -1) return -1;

    // Convertir registros a floats y guardar en la estructura
    // Índices basados en tu mapeo previo (s0=0, s1=6, s2=12, s3=18, s4=24)
    int offset[] = {0, 6, 12, 18, 24};
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 4; j++) {
            uint16_t val[2] = {registros[(offset[i] + j) * 2], registros[(offset[i] + j) * 2 + 1]};
            dest->s[i][j] = modbus_get_float_cdab(val);
        }
    }
    uint16_t oil_val[2] = {registros[58], registros[59]}; // Registro 138-139 (índice 58-59)
    dest->oilTemp = modbus_get_float_cdab(oil_val);

    return 0;
}

char* generar_reporte_json(SensorData *data) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "name", "AP2200-Gateway");
    cJSON_AddNumberToObject(root, "sn", 100234);
    cJSON_AddStringToObject(root, "fw", "1.20.3");

    cJSON *data_array = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "data", data_array);

    cJSON *item = cJSON_CreateObject();
    cJSON_AddItemToArray(data_array, item);

    // Sistema
    struct sysinfo si;
    sysinfo(&si);
    cJSON_AddNumberToObject(item, "ts", (double)time(NULL));
    cJSON_AddStringToObject(item, "type", "qw");
    cJSON_AddNumberToObject(item, "uptime", (double)si.uptime);
    cJSON_AddNumberToObject(item, "sn", 1);
    cJSON_AddNumberToObject(item, "fwMajor", 1);
    cJSON_AddNumberToObject(item, "fwMinor", 20);
    cJSON_AddNumberToObject(item, "sweepCount", 210);

    // Dinámico S0-S4
    char key[24];
    const char *vars[] = {"mag", "phase", "TempPre", "TempPost"};
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 4; j++) {
            sprintf(key, "s%d%s", i, vars[j]);
            cJSON_AddNumberToObject(item, key, data->s[i][j]);
        }
    }

    cJSON_AddNumberToObject(item, "oilTemp", data->oilTemp);
    cJSON_AddNumberToObject(item, "boardTemp", 4509);
    cJSON_AddNumberToObject(item, "rh", 0);

    char *out = cJSON_Print(root);
    cJSON_Delete(root);
    return out;
}

void guardar_en_disco(const char *json_str, const char *ruta) {
    FILE *f = fopen(ruta, "w");
    if (f) {
        fputs(json_str, f);
        fclose(f);
        printf("Archivo guardado en: %s\n", ruta);
    } else {
        perror("Error al guardar archivo");
    }
}

// --- MAIN (La "simple línea" de ejecución) ---
int main() {
    SensorData mis_datos;
    
    printf("Iniciando ciclo de lectura...\n");

    if (leer_datos_sensor(&mis_datos) == 0) {
        char *json_final = generar_reporte_json(&mis_datos);
        
        if (json_final) {
            guardar_en_disco(json_final, "/SD/sensor_data.json");
            printf("Proceso completado con éxito.\n");
            free(json_final); 
        }
    } else {
        fprintf(stderr, "Error: No se pudo obtener datos del sensor.\n");
    }

    return 0;
}