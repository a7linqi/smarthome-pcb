#include "delay.h"

#include "FreeRTOS.h"
#include "task.h"

#include "stm32f10x.h"

static uint8_t timer_initialized;

static void DelayTimer_Init(void)
{
    if (timer_initialized != 0U) {
        return;
    }

    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
    TIM4->PSC = 72U - 1U;
    TIM4->ARR = 0xFFFFU;
    TIM4->CNT = 0U;
    TIM4->CR1 = TIM_CR1_CEN;
    timer_initialized = 1U;
}

void delay_us(uint32_t us)
{
    DelayTimer_Init();

    while (us > 0U) {
        const uint16_t chunk = (us > 60000U) ? 60000U : (uint16_t)us;
        const uint16_t start = (uint16_t)TIM4->CNT;

        while ((uint16_t)((uint16_t)TIM4->CNT - start) < chunk) { }
        us -= chunk;
    }
}

void delay_ms(uint32_t ms)
{
    if ((xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) && (ms > 0U)) {
        vTaskDelay(pdMS_TO_TICKS(ms));
        return;
    }

    while (ms-- > 0U) {
        delay_us(1000U);
    }
}

void delay_s(uint32_t s)
{
    while (s-- > 0U) {
        delay_ms(1000U);
    }
}
