// compilacion cruzada
// export PREFIX_ARM="$HOME/opt/libmodbus-arm"
// arm-linux-gnueabihf-gcc main.c -o sensor_trident_modbus -static -I$PREFIX_ARM/include/modbus $PREFIX_ARM/lib/libmodbus.a
// transferencia de binario al validador
// scp sensor_trident_modbus root@192.168.188.39:/SD/

#include <stdio.h>
#include <modbus.h>
#include <errno.h>
#include <stdint.h>
#include <memory.h>
#include <sys/sysinfo.h> // Librería para el uptime
#include <time.h>        // Librería para el timestamp real

// lenght 1 for uint_16
int sn = 21;
int BaudRate = 22;
int VendorID = 23;
int SN1 = 24;
int fwMajor = 27;
int fwMinor = 28;
int sweepCount = 79;
int Board_Temperature = 140;
int Oil_Rh = 141;

// lenght 2 for uint_16
int SN2 = 25;

/*
typedef enum
{
    s0Frec,
    s0mag,
    s0phase,
    s0TempPre,
    s0TempPost,
    s0TempPostSample,
    s1Frec,
    s1mag,
    s1phase,
    s1TempPre,
    s1TempPost,
    s1TempPostSample,
    s2Frec,
    s2mag,
    s2phase,
    s2TempPre,
    s2TempPost,
    s2TempPostSample,
    s3Frec,
    s3mag,
    s3phase,
    s3TempPre,
    s3TempPost,
    s3TempPostSample,
    s4Frec,
    s4mag,
    s4phase,
    s4TempPre,
    s4TempPost,
    s4TempPostSample,
} mapping_Register_Data_QW3100;

int offset_Register_Data_QW3100 = 80;
int lenght_Register_Data_QW3100 = 2;*/

/**
 * Genera el JSON con la estructura estricta requerida.
 * @param data_sensor El array con los 30 floats procesados.
 * @param output_buffer El buffer donde se guardará el string JSON.
 */

void generar_json_sensor(float *data_sensor, char *output_buffer)
{
    // Obtener Uptime del sistema
    struct sysinfo s_info;
    long uptime_real = 0;
    if (sysinfo(&s_info) == 0)
        uptime_real = s_info.uptime;

    // Obtener Timestamp
    long int ts_real = (long int)time(NULL);

    sprintf(output_buffer,
            "{\n"
            "  \"name\": \"AP2200-Gateway\",\n"
            "  \"sn\": 100234,\n"
            "  \"fw\": \"1.20.3\",\n"
            "  \"data\": [\n"
            "    {\n"
            "      \"ts\": %ld,\n"
            "      \"type\": \"qw\",\n"
            "      \"uptime\": %ld,\n"
            "      \"sn\": 1,\n"
            "      \"fwMajor\": 1,\n"
            "      \"fwMinor\": 20,\n"
            "      \"sweepCount\": 210,\n"
            "      \"s0mag\": %.8f,\n"
            "      \"s0phase\": %.8f,\n"
            "      \"s0TempPre\": %.4f,\n"
            "      \"s0TempPost\": %.4f,\n"
            "      \"s1mag\": %.8f,\n"
            "      \"s1phase\": %.8f,\n"
            "      \"s1TempPre\": %.4f,\n"
            "      \"s1TempPost\": %.4f,\n"
            "      \"s2mag\": %.8f,\n"
            "      \"s2phase\": %.8f,\n"
            "      \"s2TempPre\": %.4f,\n"
            "      \"s2TempPost\": %.4f,\n"
            "      \"s3mag\": %.8f,\n"
            "      \"s3phase\": %.8f,\n"
            "      \"s3TempPre\": %.4f,\n"
            "      \"s3TempPost\": %.4f,\n"
            "      \"s4mag\": %.8f,\n"
            "      \"s4phase\": %.8f,\n"
            "      \"s4TempPre\": %.4f,\n"
            "      \"s4TempPost\": %.4f,\n"
            "      \"oilTemp\": %.4f,\n"
            "      \"boardTemp\": 4509,\n"
            "      \"rh\": 0\n"
            "    }\n"
            "  ]\n"
            "}",
            ts_real, uptime_real,
            data_sensor[0], data_sensor[1], data_sensor[2], data_sensor[3],     // s0
            data_sensor[6], data_sensor[7], data_sensor[8], data_sensor[9],     // s1
            data_sensor[12], data_sensor[13], data_sensor[14], data_sensor[15], // s2
            data_sensor[18], data_sensor[19], data_sensor[20], data_sensor[21], // s3
            data_sensor[24], data_sensor[25], data_sensor[26], data_sensor[27], // s4
            data_sensor[29]                                                     // oilTemp
    );
}

int main()
{

    // Definición de variables de registro
    int numRegistro = 80;
    int cantRegistros = 60;
    uint16_t Sweep_Results[62]; // Array para guardar los registros de 16 bits
    uint16_t *ptrSweep_Results = Sweep_Results;
    float DataSensorEIS[30];
    uint16_t DataSensor[2];

    // Configuración del puerto serial
    modbus_t *ctx = modbus_new_rtu("/dev/ttymxc2", 115200, 'N', 8, 1);
    if (ctx == NULL)
    {
        fprintf(stderr, "No se pudo crear el contexto Modbus\n");
        return -1;
    }

    // Configuración de Slave ID (handler.SlaveId = 1)
    modbus_set_slave(ctx, 1);

    // Configuración de Timeout (handler.Timeout = 5s)
    modbus_set_response_timeout(ctx, 5, 0);

    // Conexión con el sensor (handler.Connect())
    if (modbus_connect(ctx) == -1)
    {
        fprintf(stderr, "Error al conectar: %s\n", modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }

    // Lectura de registros (client.ReadHoldingRegisters)
    // rc devuelve la cantidad de registros leídos con éxito
    int rc = modbus_read_registers(ctx, numRegistro, cantRegistros, Sweep_Results);
    if (rc == -1)
    {
        fprintf(stderr, "Error en la lectura: %s\n", modbus_strerror(errno));
        modbus_close(ctx);
        modbus_free(ctx);
        return -1;
    }

    // Limpieza de recursos (Equivalente al defer)
    modbus_close(ctx);
    modbus_free(ctx);

    /*
    // --------- printf vervose ------------
    for (int i = 0; i < rc; i++)
    {
        int regAddr = numRegistro + i;
        // %04X muestra el registro completo de 16 bits en 4 dígitos hex
        printf("Registro %d: %04X\n", regAddr, Sweep_Results[i]);
    }
    */

    // se hacer el ordenamiento de formato “CD AB” a “ABCD”."

    for (size_t i = 0; i < 30; i++)
    {
        uint16_t Valor[2];                                             // bufer temporal
        memcpy(Valor, ptrSweep_Results + 2 * i, 2 * sizeof(uint16_t)); // obtiene un para de registro de sensor
        // printf("Item %d: %02x%02x\n", i, Valor[0], Valor[1]);
        DataSensorEIS[i] = modbus_get_float_cdab(Valor); // libmodbus hacer el ordenamiento de formato “CD AB” a “ABCD”." y conversion de float IEEE a decimal.
    }

    // for (size_t i = 0; i < 2; i++)
    // {
    //     uint16_t Valor[2];                                // bufer temporal
    //     memcpy(Valor, ptrSweep_Results + 2 * i, 2 * sizeof(uint16_t)); // obtiene un para de registro de sensor
    //     // printf("Item %d: %02x%02x\n", i, Valor[0], Valor[1]);
    //     DataSensorEIS[i] = modbus_get_float_cdab(Valor); // libmodbus hacer el ordenamiento de formato “CD AB” a “ABCD”." y conversion de float IEEE a decimal.
    // }

    printf("\nPrueba\n");
    for (int i = 0; i < 30; i++)
    {
        // %04X muestra el registro completo de 16 bits en 4 dígitos hex
        printf("Item %d: %f\n", i, DataSensorEIS[i]);
    }

    // Buffer para guardar el resultado de la función
    char json_final[5120];

    // Llamamos a la función pasando el array de datos y el buffer de salida
    generar_json_sensor(DataSensorEIS, json_final);

    // Guardar a archivo
    FILE *f = fopen("/SD/sensor_data.json", "w");
    if (f == NULL)
    {
        perror("Error fopen");
        return -1;
    }

    if (fputs(json_final, f) == EOF)
    {
        perror("Error fputs");
    }

    fclose(f);

    return 0;
}