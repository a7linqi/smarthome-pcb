#ifndef BSP_DHT11_H
#define BSP_DHT11_H

#include <stdint.h>
#include "bsp_status.h"

typedef struct { uint8_t temperature_c; uint8_t humidity_pct; } bsp_dht11_data_t;
bsp_status_t BSP_DHT11_Read(bsp_dht11_data_t *data);

#endif
