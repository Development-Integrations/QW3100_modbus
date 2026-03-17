// compilacion cruzada
// export PREFIX_ARM="$HOME/opt/libmodbus-arm"
// arm-linux-gnueabihf-gcc main.c -o sensor_trident_modbus   -static   -I$PREFIX_ARM/include/modbus   $PREFIX_ARM/lib/libmodbus.a
// transferencia de binario al validador
// scp sensor_trident_modbus root@192.168.188.39:/SD/

#include <stdio.h>
#include <modbus.h>
#include <errno.h>
#include <stdint.h>
#include <memory.h>

typedef enum
{
    sn,
    BaudRate,
    VebndorID,
    SN1,
    SN2,
    fwMajor,
    fwMinor
} mapping_Register_Info_QW3100;

int sweepCount = 79;

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
    oilTemp,
    boardTemp,
    rh
} mapping_Register_Data_QW3100;

int offset_Register_Data_QW3100 = 80;
int lenght_Register_Data_QW3100 = 2;





int main()
{

    // Definición de variables de registro (Igual que en tu Go)
    int numRegistro = 80;
    int cantRegistros = 60; // debe ser par (obligatorio)
    uint16_t results[60];   // Array para guardar los registros de 16 bits
    uint16_t *ptrresult = results;
    float item[30];

    // Configuración del puerto serial (Equivalente al RTUClientHandler)
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
    int rc = modbus_read_registers(ctx, numRegistro, cantRegistros, results);
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

    // Bucle de visualización (Equivalente a tu for i := 0...)
    for (int i = 0; i < rc; i++)
    {
        int regAddr = numRegistro + i;
        // %04X muestra el registro completo de 16 bits en 4 dígitos hex
        printf("Registro %d: %04X\n", regAddr, results[i]);
    }

    // se hacer el ordenamiento de formato “CD AB” a “ABCD”."
    uint16_t *ptr = results;

    for (size_t i = 0; i < 30; i++)
    {
        uint16_t Valor[2];                                // bufer temporal
        memcpy(Valor, ptr + 2 * i, 2 * sizeof(uint16_t)); // obtiene un para de registro de sensor
        printf("Item %d: %02x%02x\n", i, Valor[0], Valor[1]);
        item[i] = modbus_get_float_cdab(Valor); // libmodbus hacer el ordenamiento de formato “CD AB” a “ABCD”." y conversion de float IEEE a decimal.
    }

    printf("\nPrueba\n");

    for (int i = 0; i < 30; i++)
    {
        // %04X muestra el registro completo de 16 bits en 4 dígitos hex
        printf("Item %d: %f\n", i, item[i]);
    }

    return 0;
}