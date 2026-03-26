#include "modbus_comm.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_RECONNECT_RETRIES 5

// Lee un bloque de registros Modbus con manejo de errores y reconexión automática.
// Si falla la lectura, intenta reconectar hasta MAX_RECONNECT_RETRIES veces con
// backoff exponencial (2s, 4s, 8s, 16s, 32s). Si agota los intentos, llama exit(1).
int read_block_modbus(modbus_t *ctx, int addr, int size, uint16_t *buffer)
{
    int rc = modbus_read_registers(ctx, addr, size, buffer);

    if (rc == -1)
    {
        fprintf(stderr, "Read failed (addr=%d): %s\n", addr, modbus_strerror(errno));

        modbus_close(ctx);

        unsigned int delay = 2;
        for (int attempt = 1; attempt <= MAX_RECONNECT_RETRIES; attempt++)
        {
            fprintf(stderr, "Reconnect attempt %d/%d (wait %us)...\n",
                    attempt, MAX_RECONNECT_RETRIES, delay);
            sleep(delay);
            if (modbus_connect(ctx) == 0)
            {
                printf("Reconnected successfully\n");
                return -1;
            }
            fprintf(stderr, "Reconnect failed: %s\n", modbus_strerror(errno));
            delay *= 2;
        }

        fprintf(stderr, "Max reconnect attempts (%d) reached. Exiting.\n",
                MAX_RECONNECT_RETRIES);
        exit(1);
    }

    return rc;
}
