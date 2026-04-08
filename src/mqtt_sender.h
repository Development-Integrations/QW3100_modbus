#ifndef MQTT_SENDER_H
#define MQTT_SENDER_H

#include <stdint.h>

typedef struct
{
    int      enabled;               /* 0 = skip */
    char     broker_url[256];       /* ej: a2g1ow8hkm0df8-ats.iot.us-east-1.amazonaws.com */
    int      port;                  /* default: 8883 */
    char     thing_name[128];       /* nombre del device en AWS IoT */
    char     cert_path[256];        /* /SD/certs/device.crt */
    char     key_path[256];         /* /SD/certs/device.key */
    char     ca_path[256];          /* /SD/certs/root-CA.crt */
    uint32_t connect_timeout_sec;   /* timeout de conexión, default: 10 */
    uint32_t keepalive_sec;         /* keepalive MQTT, default: 60 */
} MqttConfig;

typedef enum
{
    MQTT_OK = 0,
    MQTT_ERR_CONNECT,   /* no pudo conectar al broker */
    MQTT_ERR_PUBLISH,   /* error al publicar */
    MQTT_ERR_TLS,       /* error de certificados */
    MQTT_ERR_BUILD,     /* error construyendo el payload shadow (JSON inválido) */
    MQTT_DISABLED       /* mqtt.enabled = 0 */
} MqttResult;

/*
 * Envía canonical_json (JSON canónico del sensor) al broker MQTT de Scante.
 * Construye el wrapper AWS Device Shadow en memoria antes de publicar.
 * Abre conexión, publica con QoS=1, cierra conexión (stateless por ciclo).
 * filename se usa para extraer el ts y construir el clientToken.
 * Retorna MqttResult.
 */
MqttResult mqtt_publish(const MqttConfig *cfg,
                        const char *canonical_json,
                        const char *filename);

#endif /* MQTT_SENDER_H */
