#include "bsp_buzzer.h"
#include "bsp_board.h"

static bsp_buzzer_mode_t current_mode;
static uint32_t period, deadline;
static uint8_t output_on;

void BSP_Buzzer_SetMode(bsp_buzzer_mode_t mode, uint32_t half_period_ms)
{
    current_mode = mode;
    period = half_period_ms ? half_period_ms : 250U;
    deadline = 0U;
    output_on = mode == BSP_BUZZER_ON;
    BSP_Buzzer_Set(output_on != 0U);
}

void BSP_Buzzer_Process(uint32_t now_ms)
{
    if (current_mode != BSP_BUZZER_BLINK) return;
    if (deadline == 0U || (int32_t)(now_ms - deadline) >= 0) {
        output_on ^= 1U;
        BSP_Buzzer_Set(output_on != 0U);
        deadline = now_ms + period;
    }
}
