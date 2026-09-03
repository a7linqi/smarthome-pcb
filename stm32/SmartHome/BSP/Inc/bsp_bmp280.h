#ifndef BSP_BMP280_H
#define BSP_BMP280_H

#include <stdint.h>
#include "bsp_status.h"

typedef struct { int32_t temperature_centi_c; uint32_t pressure_pa; } bsp_bmp280_data_t;
bsp_status_t BSP_BMP280_Init(void);
bsp_status_t BSP_BMP280_Read(bsp_bmp280_data_t *data);

#endif
