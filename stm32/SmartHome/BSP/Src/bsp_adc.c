#include "bsp_adc.h"
#include "stm32f1xx_hal.h"

extern ADC_HandleTypeDef hadc1;
static volatile uint16_t adc_dma[BSP_ADC_COUNT];

bsp_status_t BSP_ADC_Start(void)
{
    if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK) return BSP_ERROR;
    return HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_dma, BSP_ADC_COUNT) == HAL_OK ? BSP_OK : BSP_ERROR;
}

uint16_t BSP_ADC_GetRaw(bsp_adc_channel_t channel)
{
    return channel < BSP_ADC_COUNT ? adc_dma[channel] : 0U;
}

uint16_t BSP_ADC_GetFiltered(bsp_adc_channel_t channel)
{
    static uint32_t filtered[BSP_ADC_COUNT];
    if (channel >= BSP_ADC_COUNT) return 0U;
    filtered[channel] = filtered[channel] == 0U ? ((uint32_t)adc_dma[channel] << 3) : filtered[channel] - (filtered[channel] >> 3) + adc_dma[channel];
    return (uint16_t)(filtered[channel] >> 3);
}
