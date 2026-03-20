#include "sensor.h"

#include <string.h>
#include <stdio.h>
#include <modbus.h>

DataSensor dataSensor[] = {
    // name                     ID  size  unit     type         factor  value
    {"Modbus Address",          21, 1,    NULL,    TYPE_UINT16,   1,      {.uint16_value = 0}},
    {"Baud Rate",               22, 1,    NULL,    TYPE_UINT16,   1,      {.uint16_value = 0}},
    {"Vendor ID",               23, 1,    NULL,    TYPE_UINT16,   1,      {.uint16_value = 0}},
    {"Serial Number #1",        24, 1,    NULL,    TYPE_UINT16,   1,      {.uint16_value = 0}},
    {"Serial Number #2",        25, 2,    NULL,    TYPE_UINT32,   1,      {.uint32_value = 0}},
    {"Firmware Version",        27, 2,    NULL,    TYPE_UINT32,   1,      {.uint32_value = 0}},
    {"Hardware Version",        29, 1,    NULL,    TYPE_UINT16,   1,      {.uint32_value = 0}},
    {"Calibration date",        30, 2,    "s",     TYPE_UINT32,   1,      {.uint16_value = 0}},
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
    {"Oil RH",                 141, 1,    "%",     TYPE_UINT16,   100,    {.uint16_value = 0}}
};

const size_t dataSensor_count = sizeof(dataSensor) / sizeof(dataSensor[0]);

uint8_t value_set(DataSensor *sensor, const uint16_t *src)
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
            memcpy(data, src, 4);
            sensor->value.float_value = modbus_get_float_cdab(data);
        }
        else if (sensor->type == TYPE_UINT32)
        {
            uint16_t data[2];
            memcpy(data, src, 4);
            sensor->value.uint32_value = ((uint32_t)data[0] << 16) | data[1];
        }
    }

    return sensor->size_16_t;
}

void print_sensor_test_polling(const DataSensor *sensor, char *buffer, size_t buffer_size)
{
    if (sensor->type == TYPE_UINT16)
    {
        snprintf(buffer, buffer_size, "%u", sensor->value.uint16_value);
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

uint32_t get_uint32_value(const DataSensor *sensor)
{
    if (sensor->type == TYPE_UINT32)
    {
        return sensor->value.uint32_value;
    }
    return 0;
}

uint16_t get_uint16_value(const DataSensor *sensor)
{
    if (sensor->type == TYPE_UINT16)
    {
        return sensor->value.uint16_value;
    }
    return 0;
}

float get_float_value(const DataSensor *sensor)
{
    if (sensor->type == TYPE_FLOAT)
    {
        return sensor->value.float_value;
    }
    return 0.0f;
}
