#include "modbus_comm.h"

#include <stdio.h>
#include <errno.h>
#include <unistd.h>

// Lee un bloque de registros Modbus con manejo de errores y reconexión automática
int read_block_modbus(modbus_t *ctx, int addr, int size, uint16_t *buffer)
{
    int rc = modbus_read_registers(ctx, addr, size, buffer);

    if (rc == -1)
    {
        fprintf(stderr, "Read failed (addr=%d): %s\n", addr, modbus_strerror(errno));

        modbus_close(ctx);

        while (modbus_connect(ctx) == -1)
        {
            fprintf(stderr, "Reconnect failed: %s\n", modbus_strerror(errno));
            sleep(2);
        }

        printf("Reconnected successfully\n");
        return -1;
    }

    return rc;
}
