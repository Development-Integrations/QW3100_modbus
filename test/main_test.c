
// aprendizaje de uso de unios y structs para organizar los datos del sensor y sus funciones de procesamiento
#include <stdio.h>
#include <stdint.h>
#include <memory.h>
#include <modbus.h>
#include <limits.h>
#include <float.h>

#define register_sensor_info_size 12 // Cantidad de registros de información del sensor
#define register_sensor_data_size 63 // Cantidad de registros de datos del sensor
#define register_sensor_all_size (register_sensor_info_size + register_sensor_data_size) // Cantidad total de registros del sensor (información + datos)

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
    float float_value;
} DataValue;

typedef struct
{
    const char name[32]; // nombre del dato del sensor
    const uint8_t ID; // posicion de registro.
    const uint8_t size_16_t; // cantidad de registros de 16 bits que ocupa el dato
    const char *unit; // unidad de medida del dato
    const DataType type; // tipo de dato
    const uint8_t Conversion; // factor de conversión 
    DataValue value; // valor del dato
} DataSensor;

uint8_t Value_set(DataSensor *sensor, const uint16_t *src)
{ 
    if (sensor->size_16_t == 1)
    {
        sensor->value.uint16_value = src[0];
    }
    else if (sensor->size_16_t == 2)
    {
        if (sensor->type == TYPE_FLOAT)
        {
            uint16_t data[2];
            memcpy(data, src, 4); // Copia los 4 bytes del registro al buffer de datos
            sensor->value.float_value = modbus_get_float_cdab(data);
        } else if (sensor->type == TYPE_UINT32)
        {
            uint16_t data[2];
            memcpy(data, src, 4); // Copia los 4 bytes del registro al
            sensor->value.uint32_value = (data[0] << 16) | data[1]; // Combina los dos registros de 16 bits en un valor de 32 bits
        }
    } 
    return sensor->size_16_t; // Retorna la cantidad de registros de 16 bits que se han leído para este dato
} 

void print_sensor_value(const DataSensor *sensor,char *buffer,size_t buffer_size)
{
    if (sensor->type == TYPE_UINT16)
    {
        snprintf(buffer, buffer_size, "name: %s, value: %u\n", sensor->name, sensor->value.uint16_value);
    }
    else if (sensor->type == TYPE_UINT32)
    {
        snprintf(buffer, buffer_size, "name: %s, value: %u\n", sensor->name, sensor->value.uint32_value);
    }
    else if (sensor->type == TYPE_FLOAT)
    {
        snprintf(buffer, buffer_size, "name: %s, value: %f\n", sensor->name, sensor->value.float_value);
    }
}


void print_sensor_test_polling(const DataSensor *sensor,char *buffer,size_t buffer_size)
{
    if (sensor->type == TYPE_UINT16)
    {
        snprintf(buffer, buffer_size,"%u", sensor->value.uint16_value);
    }
    else if (sensor->type == TYPE_UINT32)
    {
        snprintf(buffer, buffer_size, "%u", sensor->value.uint32_value);
    }
    else if (sensor->type == TYPE_FLOAT)
    {
        snprintf(buffer, buffer_size, "%f", sensor->value.float_value);
    }
}

//----------------- getter type -----------------------------
uint32_t get_uint32_value(const DataSensor *sensor)
{
    if (sensor->type == TYPE_UINT32)
    {
        return sensor->value.uint32_value;
    }
    return 0; // Valor predeterminado si el tipo no coincide
}
uint16_t get_uint16_value(const DataSensor *sensor)
{
    if (sensor->type == TYPE_UINT16)
    {
        return sensor->value.uint16_value;
    }
    return 0; // Valor predeterminado si el tipo no coincide
}
float get_float_value(const DataSensor *sensor)
{
    if (sensor->type == TYPE_FLOAT)
    {
        return sensor->value.float_value;
    }
    return 0.0f; // Valor predeterminado si el tipo no coincide
}



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

const uint8_t sensor_data_type_JSON[] = {
    
};

DataSensor dataSensor[] = {

    // name                     ID  size  unit     type         factor  value
    {"Modbus Address",          21, 1,    NULL,    TYPE_UINT16,   1,      {.uint16_value = 0}},
    {"Baud Rate",               22, 1,    NULL,    TYPE_UINT16,   1,      {.uint16_value = 0}},
    {"Vendor ID",               23, 1,    NULL,    TYPE_UINT16,   1,      {.uint16_value = 0}},
    {"Serial Number #1",        24, 1,    NULL,    TYPE_UINT16,   1,      {.uint16_value = 0}},
    {"Serial Number #2",        25, 2,    NULL,    TYPE_UINT32,   1,      {.uint32_value = 0}},
    {"Firmware Version",        27, 2,    NULL,    TYPE_UINT32,   1,      {.uint32_value = 0}},
    {"Hardware Version",        29, 2,    NULL,    TYPE_UINT32,   1,      {.uint32_value = 0}},
    {"Calibration date",        31, 1,    "s",     TYPE_UINT16,   1,      {.uint16_value = 0}},
    {"Can Enabled",             32, 1,    NULL,    TYPE_UINT16,   1,      {.uint16_value = 0}},
    {"Sweep Count",             79, 1,    "count", TYPE_UINT16,   1,      {.uint16_value = 0}},
    {"S0 Frequency",            80, 2,    "Hz",    TYPE_FLOAT,    1,      {.float_value = 0}},
    {"S0 Magnitude",            82, 2,    "Ohm",   TYPE_FLOAT,    1,      {.float_value = 0}},
    {"S0 Phase",                84, 2,    "deg",   TYPE_FLOAT,    1,      {.float_value = 0}},
    {"S0 Temp Pre Reference",   86, 2,    "C",     TYPE_FLOAT,    1,      {.float_value = 0}},
    {"S0 Temp Post Reference",  88, 2,    "C",     TYPE_FLOAT,    1,      {.float_value = 0}},
    {"S0 Temp Post Sample",     90, 2,    "C",     TYPE_FLOAT,    1,      {.float_value = 0}},
    {"S1 Frequency",            92, 2,    "Hz",    TYPE_FLOAT,    1,      {.float_value = 0}},
    {"S1 Magnitude",            94, 2,    "Ohm",   TYPE_FLOAT,    1,      {.float_value = 0}},
    {"S1 Phase",                96, 2,    "deg",   TYPE_FLOAT,    1,      {.float_value = 0}},
    {"S1 Temp Pre Reference",   98, 2,    "C",     TYPE_FLOAT,    1,      {.float_value = 0}},
    {"S1 Temp Post Reference", 100, 2,    "C",     TYPE_FLOAT,    1,      {.float_value = 0}},
    {"S1 Temp Post Sample",    102, 2,    "C",     TYPE_FLOAT,    1,      {.float_value = 0}},
    {"S2 Frequency",           104, 2,    "Hz",    TYPE_FLOAT,    1,      {.float_value = 0}},
    {"S2 Magnitude",           106, 2,    "Ohm",   TYPE_FLOAT,    1,      {.float_value = 0}},
    {"S2 Phase",               108, 2,    "deg",   TYPE_FLOAT,    1,      {.float_value = 0}},
    {"S2 Temp Pre Reference",  110, 2,    "C",     TYPE_FLOAT,    1,      {.float_value = 0}},
    {"S2 Temp Post Reference", 112, 2,    "C",     TYPE_FLOAT,    1,      {.float_value = 0}},
    {"S2 Temp Post Sample",    114, 2,    "C",     TYPE_FLOAT,    1,      {.float_value = 0}},
    {"S3 Frequency",           116, 2,    "Hz",    TYPE_FLOAT,    1,      {.float_value = 0}},
    {"S3 Magnitude",           118, 2,    "Ohm",   TYPE_FLOAT,    1,      {.float_value = 0}},
    {"S3 Phase",               120, 2,    "deg",   TYPE_FLOAT,    1,      {.float_value = 0}},
    {"S3 Temp Pre Reference",  122, 2,    "C",     TYPE_FLOAT,    1,      {.float_value = 0}},
    {"S3 Temp Post Reference", 124, 2,    "C",     TYPE_FLOAT,    1,      {.float_value = 0}},
    {"S3 Temp Post Sample",    126, 2,    "C",     TYPE_FLOAT,    1,      {.float_value = 0}},
    {"S4 Frequency",           128, 2,    "Hz",    TYPE_FLOAT,    1,      {.float_value = 0}},
    {"S4 Magnitude",           130, 2,    "Ohm",   TYPE_FLOAT,    1,      {.float_value = 0}},
    {"S4 Phase",               132, 2,    "deg",   TYPE_FLOAT,    1,      {.float_value = 0}},
    {"S4 Temp Pre Reference",  134, 2,    "C",     TYPE_FLOAT,    1,      {.float_value = 0}},
    {"S4 Temp Post Reference", 136, 2,    "C",     TYPE_FLOAT,    1,      {.float_value = 0}},  
    {"S4 Temp Post Sample",    138, 2,    "C",     TYPE_FLOAT,    1,      {.float_value = 0}},
    {"Board Temp",             140, 1,    "C",     TYPE_UINT16,   100,    {.uint16_value = 0}},
    {"Oil RH",                 141, 1,    "P",     TYPE_UINT16,   100,    {.uint16_value = 0}}
};

int main()
{
    char time_str[26];

    printf("Hello, Modbus!\n");
    printf("tamaño de un float: %lu bytes,value max: %f\n", sizeof(float), FLT_MAX);
    printf("tamaño de un uint16_t: %lu bytes,value max: %u\n", sizeof(uint16_t), UINT16_MAX);
    printf("tamaño de un uint32_t: %lu bytes,value max: %u\n", sizeof(uint32_t), UINT32_MAX);

    //muestra datos obtenidos del sensor para register_sensor_info
    //                 |0001|0002|0003|0004|  00005  |   0006  |   0007  |0008|0009|
    //cabecera de datos|0001|0002|0003|0004|0005|0006|0007|0008|0009|0010|0011|0012|
    //                 |0001|0004|0002|0011|06ca|0000|0001|0014|0001|8800|a004|0000|

    //muestra datos obtenidos del sensor para register_sensor_Data
    //cabecera de datos|0079|0080|0081|0082|0083|0084|0085|0086|0087|0088|0089|0090|0091|0092|0093|0094|0095|0096|0097|0098|0099|0100|0101|0102|0103|0104|0105|0106|0107|0108|0109|0110|0111|0112|0113|0114|0115|0116|0117|0118|0119|0120|0121|0122|0123|0124|0125|0126|0127|0128|0129|0130|0131|0132|0133|0134|0135|0136|0137|0138|0139|0140|0141|
    //lista de datos   |0070|999a|3e19|0e43|4e49|f5ed|c175|0798|4202|e6a0|4201|c5b0|4201|0000|4120|217a|4d88|a55e|c270|c5b0|4201|e6a0|4201|e6a0|4201|0000|42c8|3239|4c06|4baa|c2ab|e6a0|4201|c5b0|4201|c5b0|4201|0000|447a|2524|4a60|f205|c2b0|e6a0|4201|c5b0|4201|c5b0|4201|5000|47c3|5b01|4716|3a1b|c2a3|e6a0|4201|c5b0|4201|c5b0|4201|1334|0a9b|
    
    uint16_t register_sensor_info[register_sensor_info_size] = {0x0001,0x0004,0x0002,0x0011,0x06ca,0x0000,0x0001,0x0014,0x0001,0x8800,0xa004,0x0000}; // Buffer de registros de información del sensor
    uint16_t register_sensor_Data[register_sensor_data_size] = {0x0070,0x999a,0x3e19,0x0e43,0x4e49,0xf5ed,0xc175,0x0798,0x4202,0xe6a0,0x4201,0xc5b0,0x4201,0x0000,0x4120,0x217a,0x4d88,0xa55e,0xc270,0xc5b0,0x4201,0xe6a0,0x4201,0xe6a0,0x4201,0x0000,0x42c8,0x3239,0x4c06,0x4baa,0xc2ab,0xe6a0,0x4201,0xc5b0,0x4201,0xc5b0,0x4201,0x0000,0x447a,0x2524,0x4a60,0xf205,0xc2b0,0xe6a0,0x4201,0xc5b0,0x4201,0xc5b0,0x4201,0x5000,0x47c3,0x5b01,0x4716,0x3a1b,0xc2a3,0xe6a0,0x4201,0xc5b0,0x4201,0xc5b0,0x4201,0x1334,0x0a9b}; // Buffer de registros de los datos del sensor  
    uint16_t register_sensor_all[register_sensor_all_size]; // Buffer para almacenar todos los registros del sensor (información + datos)
    
    memcpy(register_sensor_all, register_sensor_info, sizeof(register_sensor_info)); 
    memcpy(register_sensor_all + register_sensor_info_size, register_sensor_Data, sizeof(register_sensor_Data));

    uint8_t counter = 0;
    char buffer[64];

    for (size_t i = 0; i < sizeof(dataSensor) / sizeof(dataSensor[0]); i++)
    {
        counter += Value_set(&dataSensor[i], register_sensor_all + counter);
        // print_sensor_value(&dataSensor[i], buffer, sizeof(buffer));
        // printf("%s", buffer);
    }
    // encabezado
    // printf("|0001|0002|0003|0004|  00005  |   0006  |   0007  |0008|0009|0010|0011|0012|");
    for (size_t i = 0; i < sizeof(dataSensor) / sizeof(dataSensor[0]); i++)
    {
        
        // reporta el tiempo actual
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);

        strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);
        printf("Current time: %s ", time_str);
        
        print_sensor_test_polling(&dataSensor[i], buffer, sizeof(buffer));
        printf("|%s", buffer);
    }
    printf("|\n");
    
    
    return 0;
}


// formato para serializara json 
/*
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
			"s0TempPost": 25.8350219726562,
			"s1mag": 4165757184,
			"s1phase": -92.2834701538086,
			"s1TempPre": 24.8896484375,
			"s1TempPost": 25.5213317871094,
			"s2mag": 337540128,
			"s2phase": -89.2349319458008,
			"s2TempPre": 25.6260375976562,
			"s2TempPost": 25.030517578125,
			"s3mag": 35320552,
			"s3phase": -89.8871994018555,
			"s3TempPre": 25.135986328125,
			"s3TempPost": 25.2061767578125,
			"s4mag": 453008.96875,
			"s4phase": -55.3989562988281,
			"s4TempPre": 25.2412719726562,
			"s4TempPost": 25.451416015625,
			"oilTemp": 25.451416015625,
			"boardTemp": 4509,
			"rh": 0
    }
  ]
}*/