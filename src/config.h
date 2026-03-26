#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

#define CONFIG_DEFAULT_INTERVAL_SEC   120
#define CONFIG_DEFAULT_PATH           "/SD/qw3100-config.json"
#define CONFIG_DEFAULT_SERIAL_PORT    "/dev/ttymxc2"
#define CONFIG_DEFAULT_SLAVE_ID       1
#define CONFIG_DEFAULT_PERSIST_PATH   "/SD/pending"

/* Resultado de carga de configuración desde archivo */
typedef enum
{
    CONFIG_FILE_OK = 0,       /* Archivo leído y parseado correctamente */
    CONFIG_FILE_NOT_FOUND,    /* Archivo no existe (no es error fatal) */
    CONFIG_FILE_INVALID_JSON, /* Contenido no es JSON válido */
    CONFIG_FILE_INVALID_VALUE /* Valor fuera de rango */
} ConfigFileResult;

/*
 * Configuración de la aplicación.
 * Precedencia de llenado: default → archivo JSON → argumento CLI
 */
typedef struct
{
    uint16_t interval_sec;     /* Período de captura en segundos (1..65535) */
    char     config_path[128]; /* Ruta del archivo de configuración */
    char     serial_port[64];  /* Puerto serial Modbus RTU (ej: /dev/ttymxc2) */
    uint8_t  slave_id;         /* Modbus slave ID del sensor (1..247) */
    char     persist_path[128]; /* Directorio donde guardar JSON pendientes */
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
