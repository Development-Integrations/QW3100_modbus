#ifndef MODBUS_COMM_H
#define MODBUS_COMM_H

#include <stdint.h>
#include <modbus.h>

#define MODBUS_RANGE_SIZE(start, end) ((end) - (start) + 1)

/* Warm-up: tiempo máximo de espera hasta sweepCount > 0 */
#define SENSOR_WARMUP_TIMEOUT_SEC  150   /* 120s esperados + 25% margen */
#define SENSOR_WARMUP_POLL_SEC      10   /* intervalo entre polls */
#define SWEEP_COUNT_REG            201   /* registro sweepCount del QW3100 */

int read_block_modbus(modbus_t *ctx, int addr, int size, uint16_t *buffer);

/*
 * Espera activamente a que el sensor complete su primer barrido EIS.
 * Retorna  0 si sweepCount > 0 antes de SENSOR_WARMUP_TIMEOUT_SEC.
 * Retorna -1 si se agotó el timeout → el caller debe hacer exit(1).
 */
int modbus_wait_first_sweep(modbus_t *ctx);

#endif /* MODBUS_COMM_H */
