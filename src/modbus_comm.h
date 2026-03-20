#ifndef MODBUS_COMM_H
#define MODBUS_COMM_H

#include <stdint.h>
#include <modbus.h>

#define MODBUS_RANGE_SIZE(start, end) ((end) - (start) + 1)

int read_block_modbus(modbus_t *ctx, int addr, int size, uint16_t *buffer);

#endif /* MODBUS_COMM_H */
