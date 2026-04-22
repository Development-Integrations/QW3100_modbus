#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include "http_sender.h"
#include "mqtt_sender.h"

#define GATEWAY_FW_VERSION                "0.1.0"

#define CONFIG_DEFAULT_INTERVAL_SEC       120
#define CONFIG_DEFAULT_PATH               "/SD/qw3100-config.json"
#define CONFIG_DEFAULT_SERIAL_PORT        "/dev/ttymxc2"
#define CONFIG_DEFAULT_SLAVE_ID           1
#define CONFIG_DEFAULT_PERSIST_PATH       "/SD/pending"
#define CONFIG_DEFAULT_PERSIST_SENT_PATH  "/SD/sent"
#define CONFIG_DEFAULT_PRIMARY_INTERFACE  "api"

/* Defaults del módulo de envío y circuit breaker */
#define CONFIG_DEFAULT_FIFO_MAX_PER_CYCLE   10   /* archivos por ciclo */
#define CONFIG_DEFAULT_CB_FAIL_THRESHOLD    5    /* fallos para abrir circuito */
#define CONFIG_DEFAULT_CB_OPEN_TIMEOUT      60   /* segundos antes de HALF_OPEN */
#define CONFIG_DEFAULT_CB_BACKOFF_MAX       300  /* máximo backoff exponencial (5 min) */
#define CONFIG_DEFAULT_SENT_RETENTION_COUNT 1000 /* máximo archivos en sent/ */

/* Resultado de carga de configuración desde archivo */
typedef enum
{
    CONFIG_FILE_OK = 0,       /* Archivo leído y parseado correctamente */
    CONFIG_FILE_NOT_FOUND,    /* Archivo no existe (no es error fatal) */
    CONFIG_FILE_INVALID_JSON, /* Contenido no es JSON válido */
    CONFIG_FILE_INVALID_VALUE /* Valor fuera de rango */
} ConfigFileResult;

/* Identidad del gateway — bloque "gateway" en el JSON de config */
typedef struct
{
    char name[32];   /* Nombre del gateway, ej: "FLO-W9" */
    char sn[8];      /* Serial de 4 dígitos como string, ej: "0000" */
} GatewayConfig;

/* Parámetros de envío y circuit breaker — bloque "send" en el JSON de config */
typedef struct
{
    uint8_t  fifo_max_per_cycle;    /* archivos pendientes a procesar por ciclo */
    uint8_t  cb_fail_threshold;     /* fallos consecutivos para abrir el circuito */
    uint32_t cb_open_timeout_sec;   /* segundos de espera antes de pasar a HALF_OPEN */
    uint32_t cb_backoff_max_sec;    /* límite máximo del backoff exponencial */
    uint16_t sent_retention_count;  /* máximo de archivos en sent/ antes de rotar */
} SendConfig;

/*
 * Configuración de la aplicación.
 * Precedencia de llenado: default → archivo JSON → argumento CLI
 */
typedef struct
{
    uint16_t   interval_sec;      /* Período de captura en segundos (1..65535) */
    char       config_path[128];  /* Ruta del archivo de configuración */
    char       serial_port[64];   /* Puerto serial Modbus RTU (ej: /dev/ttymxc2) */
    uint8_t    slave_id;          /* Modbus slave ID del sensor (1..247) */
    char       persist_path[128];      /* Directorio donde guardar JSON pendientes */
    char       persist_sent_path[128]; /* Directorio para JSONs ya enviados */
    GatewayConfig gateway;             /* Identidad del gateway en el payload JSON */
    HttpConfig api;                    /* Configuración HTTP POST */
    MqttConfig mqtt;                   /* Configuración MQTT (AWS IoT) */
    SendConfig send;                   /* Parámetros de envío y circuit breaker */
    char       primary_interface[8];   /* Interfaz principal: "api" o "mqtt" */
} AppConfig;

/*
 * Inicializa la configuración con valores por defecto.
 */
void config_init(AppConfig *cfg);

/*
 * Carga y aplica los valores del archivo JSON en *cfg.
 * Sólo sobreescribe los campos presentes en el archivo.
 * Retorna un ConfigFileResult para que main() decida cómo proceder.
 */
ConfigFileResult config_load_file(AppConfig *cfg);

/*
 * Aplica overrides desde los argumentos de línea de comandos.
 * Sólo sobreescribe los campos que el usuario indicó explícitamente.
 *
 * Argumentos reconocidos:
 *   --interval | -i  <segundos>    Período de captura (1..65535)
 *   --config   | -c  <ruta>        Ruta del archivo de configuración
 *
 * Retorna  0 en éxito, -1 si hay argumento inválido.
 */
int config_apply_cli(AppConfig *cfg, int argc, char *argv[]);

/*
 * Imprime la configuración final en stdout (útil para diagnóstico inicio).
 */
void config_print(const AppConfig *cfg);

#endif /* CONFIG_H */
