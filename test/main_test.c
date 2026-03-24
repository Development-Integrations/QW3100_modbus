
// Pruebas unitarias para validar capas de sensor y modbus
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include <time.h>
#include "../src/sensor.h"
#include "../src/modbus_comm.h"

// Datos de prueba: registros de información del sensor
static const uint16_t register_sensor_info[] = {
    0x0001, 0x0004, 0x0002, 0x0011, 0x06ca, 0x0000,
    0x0001, 0x0014, 0x0001, 0x8800, 0xa004, 0x0000
};

// Datos de prueba: registros de datos del sensor
static const uint16_t register_sensor_data[] = {
    0x0070, 0x999a, 0x3e19, 0x0e43, 0x4e49, 0xf5ed, 0xc175, 0x0798, 0x4202, 0xe6a0, 0x4201, 0xc5b0,
    0x4201, 0x0000, 0x4120, 0x217a, 0x4d88, 0xa55e, 0xc270, 0xc5b0, 0x4201, 0xe6a0, 0x4201, 0xe6a0,
    0x4201, 0x0000, 0x42c8, 0x3239, 0x4c06, 0x4baa, 0xc2ab, 0xe6a0, 0x4201, 0xc5b0, 0x4201, 0xc5b0,
    0x4201, 0x0000, 0x447a, 0x2524, 0x4a60, 0xf205, 0xc2b0, 0xe6a0, 0x4201, 0xc5b0, 0x4201, 0xc5b0,
    0x4201, 0x5000, 0x47c3, 0x5b01, 0x4716, 0x3a1b, 0xc2a3, 0xe6a0, 0x4201, 0xc5b0, 0x4201, 0xc5b0,
    0x4201, 0x1334, 0x0a9b
};

static void test_sensor_data_parsing(void)
{
    printf("\n=== TEST: Sensor Data Parsing ==="); 
    printf("\n");
    
    uint16_t register_sensor_all[REGISTER_SENSOR_ALL_SIZE];
    
    // Copiar datos info + data en buffer unificado
    memcpy(register_sensor_all, register_sensor_info, sizeof(register_sensor_info));
    memcpy(register_sensor_all + REGISTER_SENSOR_INFO_SIZE, register_sensor_data, sizeof(register_sensor_data));
    
    // Procesar todos los sensores con offset basado en ID
    for (size_t i = 0; i < dataSensor_count; i++)
    {
        uint8_t offset;
        
        if (dataSensor[i].ID >= 21 && dataSensor[i].ID <= 32)
        {
            offset = dataSensor[i].ID - 21;  // 21→0, 32→11
        }
        else if (dataSensor[i].ID >= 79 && dataSensor[i].ID <= 141)
        {
            offset = REGISTER_SENSOR_INFO_SIZE + (dataSensor[i].ID - 79);  // 79→12, 141→74
        }
        else
        {
            fprintf(stderr, "ERROR: ID de sensor inválido: %u\n", dataSensor[i].ID);
            continue;
        }
        
        if (offset < REGISTER_SENSOR_ALL_SIZE)
        {
            value_set(&dataSensor[i], register_sensor_all + offset);
        }
    }
    
    // Mostrar resultados
    char buffer[64];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[26];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    printf("Current time: %s\n", time_str);
    printf("Sensor values:\n");
    
    for (size_t i = 0; i < dataSensor_count; i++)
    {
        print_sensor_test_polling(&dataSensor[i], buffer, sizeof(buffer));
        
        const char *unit = dataSensor[i].unit ? dataSensor[i].unit : "";
        printf("  [%2zu] %-30s = %10s %s\n", i, dataSensor[i].name, buffer, unit);
    }
}

static void test_data_types(void)
{
    printf("\n=== TEST: Data Types ==="); 
    printf("\n");
    
    printf("sizeof(float):     %lu bytes, max: %e\n", sizeof(float), FLT_MAX);
    printf("sizeof(uint16_t):  %lu bytes, max: %u\n", sizeof(uint16_t), UINT16_MAX);
    printf("sizeof(uint32_t):  %lu bytes, max: %u\n", sizeof(uint32_t), UINT32_MAX);
    printf("sizeof(DataSensor): %lu bytes\n", sizeof(DataSensor));
    printf("Total sensors: %zu\n", dataSensor_count);
}

int main(void)
{
    printf("=================================\n");
    printf("Modbus Sensor Test Suite\n");
    printf("=================================\n");
    
    test_data_types();
    test_sensor_data_parsing();
    
    printf("\n=================================\n");
    printf("Tests completed successfully\n");
    printf("=================================\n");
    
    return 0;

    //snapshot de datos JSON esperado para referencia
}


/* 
   === REFERENCIA DE DATOS JSON ===
   
   Formato esperado para serialización:
   {
     "name": "AP2200-Gateway",
     "sn": 100234,
     "fw": "1.20.3",
     "data": [
       {
         "ts": 1571775082,
         "type": "<qm/qw>",
         "uptime": 18286,
         "sn": 1,
         "fwMajor": 1,
         "fwMinor": 20,
         "sweepCount": 210,
         "s0mag": 16238944256,
         "s0phase": 0.0981170013546944,
         "s0TempPre": 25.5213317871094,
         ...
       }
     ]
   }
*/