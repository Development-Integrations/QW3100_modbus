
// Pruebas unitarias para validar capas de sensor y modbus
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include <time.h>
#include <unistd.h>
#include "../src/sensor.h"
#include "../src/modbus_comm.h"
#include "../src/config.h"
#include "../src/persist.h"
#include "../src/http_sender.h"

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
    ASSERT(strcmp(cfg.serial_port, CONFIG_DEFAULT_SERIAL_PORT) == 0, "serial_port por defecto = /dev/ttymxc2");
    ASSERT(cfg.slave_id == CONFIG_DEFAULT_SLAVE_ID, "slave_id por defecto = 1");
}

static void test_config_new_fields_from_file(void)
{
    printf("\n=== TEST: Config serial_port y slave_id desde archivo ===\n");

    const char *tmp_path = "/tmp/qw3100_test_newfields.json";
    FILE *f = fopen(tmp_path, "w");
    if (f == NULL) { printf("  [SKIP] No se pudo crear archivo temporal\n"); return; }
    fprintf(f, "{\"interval_sec\":30,\"serial_port\":\"/dev/ttyUSB0\",\"slave_id\":5}");
    fclose(f);

    AppConfig cfg;
    config_init(&cfg);
    strncpy(cfg.config_path, tmp_path, sizeof(cfg.config_path) - 1);

    ConfigFileResult r = config_load_file(&cfg);
    ASSERT(r == CONFIG_FILE_OK, "archivo con serial_port y slave_id leído OK");
    ASSERT(cfg.interval_sec == 30, "interval_sec = 30 desde archivo");
    ASSERT(strcmp(cfg.serial_port, "/dev/ttyUSB0") == 0, "serial_port = /dev/ttyUSB0 desde archivo");
    ASSERT(cfg.slave_id == 5, "slave_id = 5 desde archivo");

    /* slave_id fuera de rango */
    FILE *f2 = fopen(tmp_path, "w");
    fprintf(f2, "{\"slave_id\":248}");
    fclose(f2);
    config_init(&cfg);
    strncpy(cfg.config_path, tmp_path, sizeof(cfg.config_path) - 1);
    r = config_load_file(&cfg);
    ASSERT(r == CONFIG_FILE_INVALID_VALUE, "slave_id 248 rechazado (fuera de 1..247)");

    remove(tmp_path);
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

static void test_config_persist_path_default(void)
{
    printf("\n=== TEST: Config persist_path default ===\n");
    AppConfig cfg;
    config_init(&cfg);
    ASSERT(strcmp(cfg.persist_path, CONFIG_DEFAULT_PERSIST_PATH) == 0,
           "persist_path por defecto = /SD/pending");
}

static void test_persist_write(void)
{
    printf("\n=== TEST: persist_write ===\n");

    const char *dir  = "/tmp/qw3100_test_pending";
    const char *json = "{\"test\":true}";
    long ts = 1700000000L;

    /* Escritura debe tener éxito */
    int rc = persist_write(dir, ts, json);
    ASSERT(rc == 0, "persist_write retorna 0 en exito");

    /* Verificar que el archivo existe y contiene el JSON */
    char path[256];
    snprintf(path, sizeof(path), "%s/%ld.json", dir, ts);
    FILE *f = fopen(path, "r");
    ASSERT(f != NULL, "archivo <ts>.json creado");

    if (f != NULL)
    {
        char buf[64] = {0};
        size_t n = fread(buf, 1, sizeof(buf) - 1, f);
        fclose(f);
        ASSERT(n == strlen(json) && strcmp(buf, json) == 0, "contenido del archivo coincide con JSON");
        remove(path);
    }

    /* Segunda llamada con mismo directorio: no debe fallar (EEXIST tolerado) */
    rc = persist_write(dir, ts + 1, json);
    ASSERT(rc == 0, "persist_write reutiliza directorio existente");

    char path2[256];
    snprintf(path2, sizeof(path2), "%s/%ld.json", dir, ts + 1);
    remove(path2);
    rmdir(dir);
}

static void test_config_api_defaults(void)
{
    printf("\n=== TEST: Config api defaults ===\n");
    AppConfig cfg;
    config_init(&cfg);
    ASSERT(cfg.api.enabled == 0,          "api.enabled = false por defecto");
    ASSERT(cfg.api.base_url[0] == '\0',   "api.base_url vacío por defecto");
    ASSERT(cfg.api.scante_token[0] == '\0', "api.scante_token vacío por defecto");
}

static void test_config_api_from_file(void)
{
    printf("\n=== TEST: Config api desde archivo ===\n");

    const char *tmp = "/tmp/qw3100_test_api.json";
    FILE *f = fopen(tmp, "w");
    if (f == NULL) { printf("  [SKIP] No se pudo crear archivo temporal\n"); return; }
    fprintf(f,
        "{"
        "\"api\":{"
          "\"enabled\":true,"
          "\"base_url\":\"http://localhost:8080/IOTData\","
          "\"item_guid\":\"guid-item\","
          "\"pull_type_guid\":\"guid-pull\","
          "\"scante_token\":\"tok123\","
          "\"scante_appid\":\"app456\","
          "\"scante_sgid\":\"sgid789\""
        "}"
        "}");
    fclose(f);

    AppConfig cfg;
    config_init(&cfg);
    strncpy(cfg.config_path, tmp, sizeof(cfg.config_path) - 1);
    ConfigFileResult r = config_load_file(&cfg);

    ASSERT(r == CONFIG_FILE_OK,                                   "api JSON leído OK");
    ASSERT(cfg.api.enabled == 1,                                  "api.enabled = true");
    ASSERT(strcmp(cfg.api.base_url, "http://localhost:8080/IOTData") == 0, "api.base_url correcto");
    ASSERT(strcmp(cfg.api.item_guid, "guid-item") == 0,          "api.item_guid correcto");
    ASSERT(strcmp(cfg.api.scante_token, "tok123") == 0,          "api.scante_token correcto");
    ASSERT(strcmp(cfg.api.scante_appid, "app456") == 0,          "api.scante_appid correcto");
    ASSERT(strcmp(cfg.api.scante_sgid,  "sgid789") == 0,         "api.scante_sgid correcto");

    remove(tmp);
}

static void test_http_post_disabled(void)
{
    printf("\n=== TEST: http_post deshabilitado ===\n");
    HttpConfig hcfg;
    memset(&hcfg, 0, sizeof(hcfg));
    hcfg.enabled = 0;

    HttpResult res = http_post(&hcfg, "{\"test\":true}");
    ASSERT(res == HTTP_DISABLED, "http_post retorna HTTP_DISABLED cuando enabled=0");
}

int main(void)
{
    printf("=================================\n");
    printf("Modbus Sensor Test Suite\n");
    printf("=================================\n");
    
    test_data_types();
    test_sensor_data_parsing();
    test_config_defaults();
    test_config_new_fields_from_file();
    test_config_cli_override();
    test_config_cli_invalid();
    test_config_file_not_found();
    test_config_precedence();
    test_config_persist_path_default();
    test_persist_write();
    test_config_api_defaults();
    test_config_api_from_file();
    test_http_post_disabled();
    
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