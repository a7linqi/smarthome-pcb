#include "bsp_stepper.h"
#include "main.h"

static const uint8_t seq[8] = {1,3,2,6,4,12,8,9};
static uint32_t remaining, interval, deadline; static int8_t direction; static uint8_t index;

static void output(uint8_t bits)
{
    HAL_GPIO_WritePin(STEPPER_IN1_GPIO_Port, STEPPER_IN1_Pin, (bits & 1U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEPPER_IN2_GPIO_Port, STEPPER_IN2_Pin, (bits & 2U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEPPER_IN3_GPIO_Port, STEPPER_IN3_Pin, (bits & 4U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEPPER_IN4_GPIO_Port, STEPPER_IN4_Pin, (bits & 8U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void BSP_Stepper_Init(void) { BSP_Stepper_Stop(); }
bsp_status_t BSP_Stepper_Move(bsp_stepper_direction_t dir, uint32_t steps, uint32_t interval_ms)
{
    if ((dir != BSP_STEPPER_OPEN && dir != BSP_STEPPER_CLOSE) || steps == 0U || interval_ms == 0U) return BSP_INVALID_ARG;
    if (remaining) return BSP_BUSY;
    direction = (int8_t)dir; remaining = steps; interval = interval_ms; deadline = HAL_GetTick(); return BSP_OK;
}
void BSP_Stepper_Process(uint32_t now_ms)
{
    if (!remaining || (int32_t)(now_ms - deadline) < 0) return;
    index = (uint8_t)((index + (direction > 0 ? 1U : 7U)) & 7U); output(seq[index]); deadline += interval;
    if (--remaining == 0U) output(0);
}
bool BSP_Stepper_IsBusy(void) { return remaining != 0U; }
void BSP_Stepper_Stop(void) { remaining = 0; interval = 0; direction = 0; index = 0; output(0); }
