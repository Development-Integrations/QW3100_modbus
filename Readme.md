# paso a paso para la Preparación del entorno Con libmodbus.
https://www.notion.so/desarrollo-de-binario-sensor_trident_modbus-327c054d411b80238751cb6162d9b434

## compilacion cruzada
export PREFIX_ARM="$HOME/opt/libmodbus-arm"
arm-linux-gnueabihf-gcc main.c -o sensor_trident_modbus_ARM -static -I$PREFIX_ARM/include/modbus $PREFIX_ARM/lib/libmodbus.a

## compilacion nativa
export PREFIX_NATIVE="$HOME/opt/libmodbus-native"
gcc main.c -o sensor_trident_modbus_NATIVE -static -I$PREFIX_NATIVE/include/modbus $PREFIX_NATIVE/lib/libmodbus.a

## transferencia de binario al validador de pruebas
scp -P 2122 sensor_trident_modbus_ARM root@192.168.188.39:/SD/