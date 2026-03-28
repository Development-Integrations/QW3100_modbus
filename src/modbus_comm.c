#include "modbus_comm.h"
#include "logger.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_RECONNECT_RETRIES 5

/*
 * Lee un bloque de registros Modbus con reconexión verificada.
 *
 * Ante un fallo distingue dos casos:
 *  - Excepción Modbus (errno >= MODBUS_ENOBASE): slave responde con error de protocolo,
 *    no cerrar el puerto, solo esperar y reintentar la lectura.
 *  - Timeout / fallo de comunicación: cerrar puerto, reconectar y verificar con lectura real.
 *
 * Retorna número de registros leídos en éxito, -1 si agota todos los intentos.
 * El caller decide si continuar el ciclo o abortar — esta función no llama exit().
 */
int read_block_modbus(modbus_t *ctx, int addr, int size, uint16_t *buffer)
{
    int rc = modbus_read_registers(ctx, addr, size, buffer);
    if (rc != -1)
        return rc;

    int last_err = errno;
    fprintf(stderr, "Read failed (addr=%d): %s\n", addr, modbus_strerror(last_err));

    unsigned int delay = 2;
    int attempt;
    for (attempt = 1; attempt <= MAX_RECONNECT_RETRIES; attempt++)
    {
        fprintf(stderr, "Retry %d/%d (wait %us)...\n",
                attempt, MAX_RECONNECT_RETRIES, delay);
        sleep(delay);

        if (last_err < MODBUS_ENOBASE)
        {
            /* Timeout o error de bus — cerrar puerto y reconectar */
            modbus_close(ctx);
            if (modbus_connect(ctx) != 0)
            {
                last_err = errno;
                fprintf(stderr, "Reconnect failed: %s\n", modbus_strerror(last_err));
                delay *= 2;
                continue;
            }
        }
        /* Excepción Modbus: slave activo pero ocupado — solo reintentar lectura */

        rc = modbus_read_registers(ctx, addr, size, buffer);
        if (rc != -1)
        {
            fprintf(stderr, "Read OK after %d retry(s) (addr=%d)\n", attempt, addr);
            return rc;
        }

        last_err = errno;
        fprintf(stderr, "Read still failing (addr=%d): %s\n",
                addr, modbus_strerror(last_err));
        delay *= 2;
    }

    fprintf(stderr, "Max retries (%d) reached. Slave unreachable (addr=%d).\n",
            MAX_RECONNECT_RETRIES, addr);
    return -1;
}

int modbus_wait_first_sweep(modbus_t *ctx)
{
    int elapsed = 0;

    LOG_INFO("[warmup] esperando primer barrido EIS (timeout %ds, poll %ds)",
             SENSOR_WARMUP_TIMEOUT_SEC, SENSOR_WARMUP_POLL_SEC);

    while (elapsed < SENSOR_WARMUP_TIMEOUT_SEC)
    {
        uint16_t sweep_count = 0;
        int rc = modbus_read_registers(ctx, SWEEP_COUNT_REG, 1, &sweep_count);

        if (rc != -1)
        {
            if (sweep_count > 0)
            {
                LOG_INFO("[warmup] sensor listo, sweepCount=%u, elapsed=%ds",
                         (unsigned)sweep_count, elapsed);
                return 0;
            }
            LOG_INFO("[warmup] sensor responde, aún procesando sweepCount=0 (%ds)", elapsed);
        }
        else
        {
            int err = errno;
            if (err == EMBXILFUN)
                LOG_INFO("[warmup] registro no disponible aún (Illegal function, %ds)", elapsed);
            else if (err == EMBXSBUSY)
                LOG_INFO("[warmup] sensor ocupado (Slave busy, %ds)", elapsed);
            else
                LOG_INFO("[warmup] sin respuesta (%s, %ds)", modbus_strerror(err), elapsed);
        }

        sleep(SENSOR_WARMUP_POLL_SEC);
        elapsed += SENSOR_WARMUP_POLL_SEC;
    }

    LOG_ERROR("[warmup] timeout: sensor no alcanzó sweepCount>0 en %ds",
              SENSOR_WARMUP_TIMEOUT_SEC);
    return -1;
}
