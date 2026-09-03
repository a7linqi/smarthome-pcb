#ifndef BSP_ADC_H
#define BSP_ADC_H

#include <stdint.h>
#include "bsp_status.h"

typedef enum { BSP_ADC_LIGHT = 0, BSP_ADC_MQ7, BSP_ADC_MQ135, BSP_ADC_MQ2, BSP_ADC_COUNT } bsp_adc_channel_t;

bsp_status_t BSP_ADC_Start(void);
uint16_t BSP_ADC_GetRaw(bsp_adc_channel_t channel);
uint16_t BSP_ADC_GetFiltered(bsp_adc_channel_t channel);

#endif
