#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>

#define REGISTER_SENSOR_INFO_SIZE  12
#define REGISTER_SENSOR_DATA_SIZE  63
#define REGISTER_SENSOR_ALL_SIZE   (REGISTER_SENSOR_INFO_SIZE + REGISTER_SENSOR_DATA_SIZE)

typedef enum
{
    TYPE_UINT32,
    TYPE_UINT16,
    TYPE_FLOAT
} DataType;

typedef union
{
    uint32_t uint32_value;
    uint16_t uint16_value;
    float    float_value;
} DataValue;

typedef struct
{
    const char    name[32];
    const uint8_t ID;
    const uint8_t size_16_t;
    const char   *unit;
    const DataType type;
    const uint8_t  conversion;
    DataValue      value;
} DataSensor;

typedef enum
{
    Modbus_Address,
    Baud_Rate,
    Vendor_ID,
    Serial_Number_1,
    Serial_Number_2,
    Firmware_Version,
    Hardware_Version,
    Calibration_date,
    Can_Enabled,
    Sweep_Count,
    S0_Frequency,
    S0_Magnitude,
    S0_Phase,
    S0_Temp_Pre_Reference,
    S0_Temp_Post_Reference,
    S0_Temp_Post_Sample,
    S1_Frequency,
    S1_Magnitude,
    S1_Phase,
    S1_Temp_Pre_Reference,
    S1_Temp_Post_Reference,
    S1_Temp_Post_Sample,
    S2_Frequency,
    S2_Magnitude,
    S2_Phase,
    S2_Temp_Pre_Reference,
    S2_Temp_Post_Reference,
    S2_Temp_Post_Sample,
    S3_Frequency,
    S3_Magnitude,
    S3_Phase,
    S3_Temp_Pre_Reference,
    S3_Temp_Post_Reference,
    S3_Temp_Post_Sample,
    S4_Frequency,
    S4_Magnitude,
    S4_Phase,
    S4_Temp_Pre_Reference,
    S4_Temp_Post_Reference,
    S4_Temp_Post_Sample,
    Board_Temp,
    Oil_RH
} SensorDataType;

typedef struct
{
    char     name[32];   /* "AP2200-Gateway" */
    uint32_t sn;         /* serial del gateway */
    char     fw[16];     /* "1.20.3" */
} GatewayInfo;

typedef struct
{
    uint32_t ts;         /* epoch seconds */
    char     type[8];    /* "qw" */
    uint32_t uptime;     /* segundos */
    uint32_t sn;         /* serial del sensor */

    uint16_t fwMajor;
    uint16_t fwMinor;

    uint16_t sweepCount;

    float s0mag;
    float s0phase;
    float s0TempPre;
    float s0TempPost;

    float s1mag;
    float s1phase;
    float s1TempPre;
    float s1TempPost;

    float s2mag;
    float s2phase;
    float s2TempPre;
    float s2TempPost;

    float s3mag;
    float s3phase;
    float s3TempPre;
    float s3TempPost;

    float s4mag;
    float s4phase;
    float s4TempPre;
    float s4TempPost;

    float oilTemp;       /* regla sugerida: = s4TempPost */
    uint16_t boardTemp;  /* crudo como en tu ejemplo (4509) */
    uint16_t rh;         /* crudo como en tu ejemplo (0) */
} SensorSnapshot;

extern DataSensor    dataSensor[];
extern const size_t  dataSensor_count;

uint8_t  value_set(DataSensor *sensor, const uint16_t *src);
void     print_sensor_test_polling(const DataSensor *sensor, char *buffer, size_t buffer_size);

// Funciones (getter) para obtener los valores de los sensores
uint32_t get_uint32_value(const DataSensor *sensor);
uint16_t get_uint16_value(const DataSensor *sensor);
float    get_float_value(const DataSensor *sensor);

// Funciones para construir el snapshot y el payload JSON
int build_sensor_snapshot(const DataSensor *src, size_t n, time_t now, SensorSnapshot *out);
char *build_gateway_payload_json(const GatewayInfo *gw, const SensorSnapshot *snap);

#endif /* SENSOR_H */
