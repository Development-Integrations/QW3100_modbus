#include "sensor.h"

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/sysinfo.h>
#include "../lib/cJSON.h"
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

int build_sensor_snapshot(const DataSensor *src, size_t n, time_t now, SensorSnapshot *out)
{
    struct sysinfo sys_info;
    uint32_t fw_raw;

    if ((src == NULL) || (out == NULL))
    {
        return -1;
    }

    if (n <= Oil_RH)
    {
        return -1;
    }

    memset(out, 0, sizeof(*out));

    out->ts = (uint32_t)now;
    strncpy(out->type, "qw", sizeof(out->type) - 1);
    out->type[sizeof(out->type) - 1] = '\0';

    if (sysinfo(&sys_info) == 0)
    {
        out->uptime = (uint32_t)sys_info.uptime;
    }

    out->sn = get_uint32_value(&src[Serial_Number_2]);

    fw_raw = get_uint32_value(&src[Firmware_Version]);
    out->fwMajor = (uint16_t)((fw_raw >> 16) & 0xFFFF);
    out->fwMinor = (uint16_t)(fw_raw & 0xFFFF);

    out->sweepCount = get_uint16_value(&src[Sweep_Count]);

    out->s0mag = get_float_value(&src[S0_Magnitude]);
    out->s0phase = get_float_value(&src[S0_Phase]);
    out->s0TempPre = get_float_value(&src[S0_Temp_Pre_Reference]);
    out->s0TempPost = get_float_value(&src[S0_Temp_Post_Reference]);

    out->s1mag = get_float_value(&src[S1_Magnitude]);
    out->s1phase = get_float_value(&src[S1_Phase]);
    out->s1TempPre = get_float_value(&src[S1_Temp_Pre_Reference]);
    out->s1TempPost = get_float_value(&src[S1_Temp_Post_Reference]);

    out->s2mag = get_float_value(&src[S2_Magnitude]);
    out->s2phase = get_float_value(&src[S2_Phase]);
    out->s2TempPre = get_float_value(&src[S2_Temp_Pre_Reference]);
    out->s2TempPost = get_float_value(&src[S2_Temp_Post_Reference]);

    out->s3mag = get_float_value(&src[S3_Magnitude]);
    out->s3phase = get_float_value(&src[S3_Phase]);
    out->s3TempPre = get_float_value(&src[S3_Temp_Pre_Reference]);
    out->s3TempPost = get_float_value(&src[S3_Temp_Post_Reference]);

    out->s4mag = get_float_value(&src[S4_Magnitude]);
    out->s4phase = get_float_value(&src[S4_Phase]);
    out->s4TempPre = get_float_value(&src[S4_Temp_Pre_Reference]);
    out->s4TempPost = get_float_value(&src[S4_Temp_Post_Reference]);

    out->oilTemp = out->s4TempPost;
    out->boardTemp = get_uint16_value(&src[Board_Temp]);
    out->rh = get_uint16_value(&src[Oil_RH]);

    return 0;
}

char *build_gateway_payload_json(const GatewayInfo *gw, const SensorSnapshot *snap)
{
    cJSON *root;
    cJSON *data;
    cJSON *sample;
    char *json_str;

    if ((gw == NULL) || (snap == NULL))
    {
        return NULL;
    }

    root = cJSON_CreateObject();
    if (root == NULL)
    {
        return NULL;
    }

    if ((cJSON_AddStringToObject(root, "name", gw->name) == NULL) ||
        (cJSON_AddStringToObject(root, "sn",   gw->sn)  == NULL) ||
        (cJSON_AddStringToObject(root, "fw",   gw->fw)  == NULL))
    {
        cJSON_Delete(root);
        return NULL;
    }

    data = cJSON_AddArrayToObject(root, "data");
    if (data == NULL)
    {
        cJSON_Delete(root);
        return NULL;
    }

    sample = cJSON_CreateObject();
    if (sample == NULL)
    {
        cJSON_Delete(root);
        return NULL;
    }
    cJSON_AddItemToArray(data, sample);

    if ((cJSON_AddNumberToObject(sample, "ts", snap->ts) == NULL) ||
        (cJSON_AddStringToObject(sample, "type", snap->type) == NULL) ||
        (cJSON_AddNumberToObject(sample, "uptime", snap->uptime) == NULL) ||
        (cJSON_AddNumberToObject(sample, "sn", snap->sn) == NULL) ||
        (cJSON_AddNumberToObject(sample, "fwMajor", snap->fwMajor) == NULL) ||
        (cJSON_AddNumberToObject(sample, "fwMinor", snap->fwMinor) == NULL) ||
        (cJSON_AddNumberToObject(sample, "sweepCount", snap->sweepCount) == NULL) ||
        (cJSON_AddNumberToObject(sample, "s0mag", snap->s0mag) == NULL) ||
        (cJSON_AddNumberToObject(sample, "s0phase", snap->s0phase) == NULL) ||
        (cJSON_AddNumberToObject(sample, "s0TempPre", snap->s0TempPre) == NULL) ||
        (cJSON_AddNumberToObject(sample, "s0TempPost", snap->s0TempPost) == NULL) ||
        (cJSON_AddNumberToObject(sample, "s1mag", snap->s1mag) == NULL) ||
        (cJSON_AddNumberToObject(sample, "s1phase", snap->s1phase) == NULL) ||
        (cJSON_AddNumberToObject(sample, "s1TempPre", snap->s1TempPre) == NULL) ||
        (cJSON_AddNumberToObject(sample, "s1TempPost", snap->s1TempPost) == NULL) ||
        (cJSON_AddNumberToObject(sample, "s2mag", snap->s2mag) == NULL) ||
        (cJSON_AddNumberToObject(sample, "s2phase", snap->s2phase) == NULL) ||
        (cJSON_AddNumberToObject(sample, "s2TempPre", snap->s2TempPre) == NULL) ||
        (cJSON_AddNumberToObject(sample, "s2TempPost", snap->s2TempPost) == NULL) ||
        (cJSON_AddNumberToObject(sample, "s3mag", snap->s3mag) == NULL) ||
        (cJSON_AddNumberToObject(sample, "s3phase", snap->s3phase) == NULL) ||
        (cJSON_AddNumberToObject(sample, "s3TempPre", snap->s3TempPre) == NULL) ||
        (cJSON_AddNumberToObject(sample, "s3TempPost", snap->s3TempPost) == NULL) ||
        (cJSON_AddNumberToObject(sample, "s4mag", snap->s4mag) == NULL) ||
        (cJSON_AddNumberToObject(sample, "s4phase", snap->s4phase) == NULL) ||
        (cJSON_AddNumberToObject(sample, "s4TempPre", snap->s4TempPre) == NULL) ||
        (cJSON_AddNumberToObject(sample, "s4TempPost", snap->s4TempPost) == NULL) ||
        (cJSON_AddNumberToObject(sample, "oilTemp", snap->oilTemp) == NULL) ||
        (cJSON_AddNumberToObject(sample, "boardTemp", snap->boardTemp) == NULL) ||
        (cJSON_AddNumberToObject(sample, "rh", snap->rh) == NULL))
    {
        cJSON_Delete(root);
        return NULL;
    }

    json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_str;
}
