
// Pruebas unitarias para validar capas de sensor y modbus
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include <time.h>
#include "../src/sensor.h"
#include "../src/modbus_comm.h"
#include "../src/config.h"

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

static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT(cond, msg) \
    do { \
        if (cond) { printf("  [OK] %s\n", msg); tests_passed++; } \
        else      { printf("  [FAIL] %s\n", msg); tests_failed++; } \
    } while (0)

static void test_config_defaults(void)
{
    printf("\n=== TEST: Config Defaults ===\n");
    AppConfig cfg;
    config_init(&cfg);

    ASSERT(cfg.interval_sec == CONFIG_DEFAULT_INTERVAL_SEC, "intervalo por defecto = 120");
    ASSERT(strcmp(cfg.config_path, CONFIG_DEFAULT_PATH) == 0, "ruta por defecto = /SD/qw3100-config.json");
}

static void test_config_cli_override(void)
{
    printf("\n=== TEST: Config CLI Override ===\n");
    AppConfig cfg;
    config_init(&cfg);

    /* Simular: --interval 30 */
    char *argv_interval[] = {"prog", "--interval", "30"};
    int rc = config_apply_cli(&cfg, 3, argv_interval);
    ASSERT(rc == 0, "--interval 30 aceptaé");
    ASSERT(cfg.interval_sec == 30, "intervalo = 30 tras CLI");

    /* Simular: --interval 65535 (máximo válido) */
    config_init(&cfg);
    char *argv_max[] = {"prog", "--interval", "65535"};
    rc = config_apply_cli(&cfg, 3, argv_max);
    ASSERT(rc == 0, "--interval 65535 aceptado");
    ASSERT(cfg.interval_sec == 65535, "intervalo = 65535 (max)");

    /* Simular: -i 10 (alias corto) */
    config_init(&cfg);
    char *argv_short[] = {"prog", "-i", "10"};
    rc = config_apply_cli(&cfg, 3, argv_short);
    ASSERT(rc == 0, "-i 10 aceptado");
    ASSERT(cfg.interval_sec == 10, "intervalo = 10 con alias -i");
}

static void test_config_cli_invalid(void)
{
    printf("\n=== TEST: Config CLI Valores Inválidos ===\n");
    AppConfig cfg;

    /* Valor 0 (fuera de rango) */
    config_init(&cfg);
    char *argv_zero[] = {"prog", "--interval", "0"};
    int rc = config_apply_cli(&cfg, 3, argv_zero);
    ASSERT(rc == -1, "--interval 0 rechazado");
    ASSERT(cfg.interval_sec == CONFIG_DEFAULT_INTERVAL_SEC, "intervalo no cambiado tras error");

    /* Valor 65536 (fuera de rango) */
    config_init(&cfg);
    char *argv_over[] = {"prog", "--interval", "65536"};
    rc = config_apply_cli(&cfg, 3, argv_over);
    ASSERT(rc == -1, "--interval 65536 rechazado");

    /* Texto (tipo incorrecto) */
    config_init(&cfg);
    char *argv_str[] = {"prog", "--interval", "abc"};
    rc = config_apply_cli(&cfg, 3, argv_str);
    ASSERT(rc == -1, "--interval abc rechazado");

    /* Sin valor */
    config_init(&cfg);
    char *argv_noval[] = {"prog", "--interval"};
    rc = config_apply_cli(&cfg, 2, argv_noval);
    ASSERT(rc == -1, "--interval sin valor rechazado");
}

static void test_config_file_not_found(void)
{
    printf("\n=== TEST: Config Archivo No Encontrado ===\n");
    AppConfig cfg;
    config_init(&cfg);

    /* Forzar ruta inexistente */
    strncpy(cfg.config_path, "/tmp/qw3100_nonexistent_test.json",
            sizeof(cfg.config_path) - 1);

    ConfigFileResult r = config_load_file(&cfg);
    ASSERT(r == CONFIG_FILE_NOT_FOUND, "retorna NOT_FOUND si archivo no existe");
    ASSERT(cfg.interval_sec == CONFIG_DEFAULT_INTERVAL_SEC,
           "intervalo conserva defaults cuando no hay archivo");
}

static void test_config_precedence(void)
{
    printf("\n=== TEST: Precedencia CLI > Archivo ===\n");
    AppConfig cfg;
    config_init(&cfg);

    /* Simular que el archivo estableció 60 */
    cfg.interval_sec = 60;

    /* Ahora CLI sobreescribe con 15 */
    char *argv_cli[] = {"prog", "--interval", "15"};
    int rc = config_apply_cli(&cfg, 3, argv_cli);
    ASSERT(rc == 0, "CLI aplicado sobre valor de archivo");
    ASSERT(cfg.interval_sec == 15, "CLI (15) gana sobre archivo (60)");
}

int main(void)
{
    printf("=================================\n");
    printf("Modbus Sensor Test Suite\n");
    printf("=================================\n");
    
    test_data_types();
    test_sensor_data_parsing();
    test_config_defaults();
    test_config_cli_override();
    test_config_cli_invalid();
    test_config_file_not_found();
    test_config_precedence();
    
    printf("\n=================================\n");
    printf("Resultados: %d OK  /  %d FAIL\n", tests_passed, tests_failed);
    printf("=================================\n");
    
    return (tests_failed == 0) ? 0 : 1;

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