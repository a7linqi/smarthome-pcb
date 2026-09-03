#include "bsp_delay.h"
#include "stm32f1xx_hal.h"

void BSP_Delay_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void BSP_DelayUs(uint32_t us)
{
    const uint32_t start = DWT->CYCCNT;
    const uint32_t ticks = us * (HAL_RCC_GetHCLKFreq() / 1000000U);
    while ((uint32_t)(DWT->CYCCNT - start) < ticks) { __NOP(); }
}
