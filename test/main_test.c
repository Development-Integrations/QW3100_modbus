
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
#include "../src/mqtt_sender.h"

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

static void test_config_gateway_defaults(void)
{
    printf("\n=== TEST: Config gateway defaults ===\n");
    AppConfig cfg;
    config_init(&cfg);
    ASSERT(strcmp(cfg.gateway.name, "FLO-W9") == 0, "gateway.name = FLO-W9 por defecto");
    ASSERT(strcmp(cfg.gateway.sn,   "0000")   == 0, "gateway.sn = 0000 por defecto");
}

static void test_config_gateway_from_file(void)
{
    printf("\n=== TEST: Config gateway desde archivo ===\n");

    const char *tmp = "/tmp/qw3100_test_gw.json";
    FILE *f = fopen(tmp, "w");
    if (f == NULL) { printf("  [SKIP] No se pudo crear archivo temporal\n"); return; }
    fprintf(f, "{\"gateway\":{\"name\":\"FLO-W9-TEST\",\"sn\":\"1234\"}}");
    fclose(f);

    AppConfig cfg;
    config_init(&cfg);
    strncpy(cfg.config_path, tmp, sizeof(cfg.config_path) - 1);
    ConfigFileResult r = config_load_file(&cfg);

    ASSERT(r == CONFIG_FILE_OK,                              "gateway JSON leído OK");
    ASSERT(strcmp(cfg.gateway.name, "FLO-W9-TEST") == 0,    "gateway.name = FLO-W9-TEST");
    ASSERT(strcmp(cfg.gateway.sn,   "1234") == 0,           "gateway.sn = 1234");

    remove(tmp);
}

static void test_config_api_defaults(void)
{
    printf("\n=== TEST: Config api defaults ===\n");
    AppConfig cfg;
    config_init(&cfg);
    ASSERT(cfg.api.enabled == 0,              "api.enabled = false por defecto");
    ASSERT(cfg.api.base_url[0] == '\0',       "api.base_url vacío por defecto");
    ASSERT(cfg.api.bearer_token[0] == '\0',   "api.bearer_token vacío por defecto");
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
          "\"base_url\":\"https://fleet.nebulae.com.co/api/external-system-gateway/rest/status\","
          "\"bearer_token\":\"test-token-abc123\","
          "\"ca_bundle_path\":\"/usr/local/share/ca-certificates/isrgrootx1.pem\""
        "}"
        "}");
    fclose(f);

    AppConfig cfg;
    config_init(&cfg);
    strncpy(cfg.config_path, tmp, sizeof(cfg.config_path) - 1);
    ConfigFileResult r = config_load_file(&cfg);

    ASSERT(r == CONFIG_FILE_OK,   "api JSON leído OK");
    ASSERT(cfg.api.enabled == 1,  "api.enabled = true");
    ASSERT(strcmp(cfg.api.base_url,
                  "https://fleet.nebulae.com.co/api/external-system-gateway/rest/status") == 0,
           "api.base_url correcto");
    ASSERT(strcmp(cfg.api.bearer_token, "test-token-abc123") == 0, "api.bearer_token correcto");
    ASSERT(strcmp(cfg.api.ca_bundle_path,
                  "/usr/local/share/ca-certificates/isrgrootx1.pem") == 0,
           "api.ca_bundle_path correcto");

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

static void test_persist_move_to_sent(void)
{
    printf("\n=== TEST: persist_move_to_sent ===\n");

    const char *pending_dir = "/tmp/qw3100_test_move_pending";
    const char *sent_dir    = "/tmp/qw3100_test_move_sent";
    const char *filename    = "1700000001.json";
    const char *json        = "{\"moved\":true}";

    /* Crear archivo en pending */
    int rc = persist_write(pending_dir, 1700000001L, json);
    ASSERT(rc == 0, "persist_write previo al move OK");

    /* Mover a sent */
    rc = persist_move_to_sent(pending_dir, sent_dir, filename);
    ASSERT(rc == 0, "persist_move_to_sent retorna 0 en exito");

    /* El archivo ya no debe estar en pending */
    char path_pending[256];
    snprintf(path_pending, sizeof(path_pending), "%s/%s", pending_dir, filename);
    FILE *fp = fopen(path_pending, "r");
    ASSERT(fp == NULL, "archivo eliminado de pending tras move");
    if (fp != NULL) fclose(fp);

    /* El archivo debe estar en sent con el mismo contenido */
    char path_sent[256];
    snprintf(path_sent, sizeof(path_sent), "%s/%s", sent_dir, filename);
    FILE *fs = fopen(path_sent, "r");
    ASSERT(fs != NULL, "archivo creado en sent tras move");
    if (fs != NULL)
    {
        char buf[64] = {0};
        size_t n = fread(buf, 1, sizeof(buf) - 1, fs);
        fclose(fs);
        ASSERT(n == strlen(json) && strcmp(buf, json) == 0, "contenido en sent coincide con JSON original");
        remove(path_sent);
    }

    rmdir(sent_dir);
    rmdir(pending_dir);
}

static void test_persist_rotate_sent_removes_excess(void)
{
    printf("\n=== TEST: persist_rotate_sent elimina exceso ===\n");

    const char *sent_dir = "/tmp/qw3100_test_rotate_sent";

    /* Crear 5 archivos */
    for (int i = 1; i <= 5; i++)
        persist_write(sent_dir, 1700000000L + i, "{\"r\":1}");

    int total = persist_list_pending(sent_dir, NULL, 0);
    ASSERT(total == 5, "5 archivos creados en sent");

    /* Rotar con max=3 — debe eliminar 2 más antiguos */
    int deleted = persist_rotate_sent(sent_dir, 3);
    ASSERT(deleted == 2, "persist_rotate_sent elimina 2 archivos (exceso)");

    int remaining = persist_list_pending(sent_dir, NULL, 0);
    ASSERT(remaining == 3, "quedan 3 archivos tras rotación");

    /* Limpiar */
    PendingFileName files[5];
    int n = persist_list_pending(sent_dir, files, 5);
    for (int i = 0; i < n; i++)
    {
        char path[256];
        snprintf(path, sizeof(path), "%s/%s", sent_dir, files[i]);
        remove(path);
    }
    rmdir(sent_dir);
}

static void test_persist_rotate_sent_no_delete(void)
{
    printf("\n=== TEST: persist_rotate_sent no elimina si count <= max ===\n");

    const char *sent_dir = "/tmp/qw3100_test_rotate_nodelete";

    /* Crear 3 archivos */
    for (int i = 1; i <= 3; i++)
        persist_write(sent_dir, 1700000100L + i, "{\"r\":0}");

    /* Rotar con max=5 — no debe eliminar nada */
    int deleted = persist_rotate_sent(sent_dir, 5);
    ASSERT(deleted == 0, "persist_rotate_sent no elimina cuando count <= max");

    int remaining = persist_list_pending(sent_dir, NULL, 0);
    ASSERT(remaining == 3, "los 3 archivos siguen presentes");

    /* Limpiar */
    PendingFileName files[5];
    int n = persist_list_pending(sent_dir, files, 5);
    for (int i = 0; i < n; i++)
    {
        char path[256];
        snprintf(path, sizeof(path), "%s/%s", sent_dir, files[i]);
        remove(path);
    }
    rmdir(sent_dir);
}

static void test_config_sent_defaults(void)
{
    printf("\n=== TEST: Config persist_sent defaults ===\n");
    AppConfig cfg;
    config_init(&cfg);
    ASSERT(strcmp(cfg.persist_sent_path, CONFIG_DEFAULT_PERSIST_SENT_PATH) == 0,
           "persist_sent_path por defecto = /SD/sent");
    ASSERT(cfg.send.sent_retention_count == CONFIG_DEFAULT_SENT_RETENTION_COUNT,
           "sent_retention_count por defecto = 1000");
}

static void test_config_sent_from_file(void)
{
    printf("\n=== TEST: Config persist_sent desde archivo ===\n");

    const char *tmp = "/tmp/qw3100_test_sent_cfg.json";
    FILE *f = fopen(tmp, "w");
    if (f == NULL) { printf("  [SKIP] No se pudo crear archivo temporal\n"); return; }
    fprintf(f,
        "{"
        "\"persist_sent_path\":\"/SD/archive\","
        "\"send\":{\"sent_retention_count\":500}"
        "}");
    fclose(f);

    AppConfig cfg;
    config_init(&cfg);
    strncpy(cfg.config_path, tmp, sizeof(cfg.config_path) - 1);
    ConfigFileResult r = config_load_file(&cfg);

    ASSERT(r == CONFIG_FILE_OK, "JSON con campos sent leído OK");
    ASSERT(strcmp(cfg.persist_sent_path, "/SD/archive") == 0,
           "persist_sent_path = /SD/archive desde archivo");
    ASSERT(cfg.send.sent_retention_count == 500,
           "sent_retention_count = 500 desde archivo");

    remove(tmp);
}

static void test_mqtt_disabled(void)
{
    printf("\n=== TEST: mqtt_publish deshabilitado ===\n");
    MqttConfig mcfg;
    memset(&mcfg, 0, sizeof(mcfg));
    mcfg.enabled = 0;

    MqttResult r = mqtt_publish(&mcfg, "{\"test\":true}", "1700000001.json");
    ASSERT(r == MQTT_DISABLED, "mqtt_publish retorna MQTT_DISABLED cuando enabled=0");
}

static void test_mqtt_invalid_json(void)
{
    printf("\n=== TEST: mqtt_publish JSON inválido ===\n");
    MqttConfig mcfg;
    memset(&mcfg, 0, sizeof(mcfg));
    mcfg.enabled = 1;
    strncpy(mcfg.thing_name, "test-thing", sizeof(mcfg.thing_name) - 1);

    MqttResult r = mqtt_publish(&mcfg, "not valid json {{", "1700000001.json");
    ASSERT(r == MQTT_ERR_BUILD, "mqtt_publish retorna MQTT_ERR_BUILD con JSON inválido");
}

static void test_config_mqtt_defaults(void)
{
    printf("\n=== TEST: Config mqtt defaults ===\n");
    AppConfig cfg;
    config_init(&cfg);
    ASSERT(cfg.mqtt.enabled == 0,       "mqtt.enabled = false por defecto");
    ASSERT(cfg.mqtt.port == 8883,        "mqtt.port = 8883 por defecto");
    ASSERT(cfg.mqtt.connect_timeout_sec == 10, "mqtt.connect_timeout_sec = 10 por defecto");
    ASSERT(cfg.mqtt.keepalive_sec == 60, "mqtt.keepalive_sec = 60 por defecto");
    ASSERT(cfg.mqtt.broker_url[0] == '\0', "mqtt.broker_url vacío por defecto");
    ASSERT(strcmp(cfg.primary_interface, "api") == 0, "primary_interface = api por defecto");
}

static void test_config_mqtt_from_file(void)
{
    printf("\n=== TEST: Config mqtt desde archivo ===\n");

    const char *tmp = "/tmp/qw3100_test_mqtt_cfg.json";
    FILE *f = fopen(tmp, "w");
    if (f == NULL) { printf("  [SKIP] No se pudo crear archivo temporal\n"); return; }
    fprintf(f,
        "{"
        "\"web_interface\":{\"primary\":\"mqtt\"},"
        "\"mqtt\":{"
          "\"enabled\":true,"
          "\"broker_url\":\"abc.iot.us-east-1.amazonaws.com\","
          "\"port\":8883,"
          "\"thing_name\":\"mi-sensor\","
          "\"cert_path\":\"/SD/certs/dev.crt\","
          "\"key_path\":\"/SD/certs/dev.key\","
          "\"ca_path\":\"/SD/certs/root.crt\","
          "\"connect_timeout_sec\":15,"
          "\"keepalive_sec\":30"
        "}"
        "}");
    fclose(f);

    AppConfig cfg;
    config_init(&cfg);
    strncpy(cfg.config_path, tmp, sizeof(cfg.config_path) - 1);
    ConfigFileResult r = config_load_file(&cfg);

    ASSERT(r == CONFIG_FILE_OK,                            "JSON mqtt leído OK");
    ASSERT(strcmp(cfg.primary_interface, "mqtt") == 0,     "primary_interface = mqtt");
    ASSERT(cfg.mqtt.enabled == 1,                          "mqtt.enabled = true");
    ASSERT(strcmp(cfg.mqtt.broker_url, "abc.iot.us-east-1.amazonaws.com") == 0,
           "mqtt.broker_url correcto");
    ASSERT(cfg.mqtt.port == 8883,                          "mqtt.port = 8883");
    ASSERT(strcmp(cfg.mqtt.thing_name, "mi-sensor") == 0,  "mqtt.thing_name correcto");
    ASSERT(strcmp(cfg.mqtt.cert_path, "/SD/certs/dev.crt") == 0, "mqtt.cert_path correcto");
    ASSERT(strcmp(cfg.mqtt.key_path, "/SD/certs/dev.key") == 0,  "mqtt.key_path correcto");
    ASSERT(strcmp(cfg.mqtt.ca_path,  "/SD/certs/root.crt") == 0, "mqtt.ca_path correcto");
    ASSERT(cfg.mqtt.connect_timeout_sec == 15,             "mqtt.connect_timeout_sec = 15");
    ASSERT(cfg.mqtt.keepalive_sec == 30,                   "mqtt.keepalive_sec = 30");

    remove(tmp);
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
    test_persist_move_to_sent();
    test_persist_rotate_sent_removes_excess();
    test_persist_rotate_sent_no_delete();
    test_config_gateway_defaults();
    test_config_gateway_from_file();
    test_config_api_defaults();
    test_config_api_from_file();
    test_http_post_disabled();
    test_config_sent_defaults();
    test_config_sent_from_file();
    test_mqtt_disabled();
    test_mqtt_invalid_json();
    test_config_mqtt_defaults();
    test_config_mqtt_from_file();
    
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