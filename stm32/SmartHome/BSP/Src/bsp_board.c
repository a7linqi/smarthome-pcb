#include "bsp_board.h"
#include "main.h"
#include "bsp_adc.h"
#include "bsp_delay.h"
#include "bsp_uart_bridge.h"

typedef struct { GPIO_TypeDef *port; uint16_t pin; } key_gpio_t;

static const key_gpio_t keys[BSP_KEY_COUNT] = {
    {KEY_UP_GPIO_Port, KEY_UP_Pin}, {KEY_1_GPIO_Port, KEY_1_Pin},
    {KEY_2_GPIO_Port, KEY_2_Pin}, {KEY_0_GPIO_Port, KEY_0_Pin}
};

bsp_status_t BSP_Board_Init(void)
{
    BSP_Delay_Init();
    BSP_Light_Set(false);
    BSP_Buzzer_Set(false);
    if (BSP_ADC_Start() != BSP_OK) return BSP_ERROR;
    if (BSP_UartBridge_Init() != BSP_OK) return BSP_ERROR;
    return BSP_OK;
}

void BSP_Light_Set(bool on) { HAL_GPIO_WritePin(LIGHT_LED_GPIO_Port, LIGHT_LED_Pin, on ? GPIO_PIN_RESET : GPIO_PIN_SET); }
bool BSP_Light_Get(void) { return HAL_GPIO_ReadPin(LIGHT_LED_GPIO_Port, LIGHT_LED_Pin) == GPIO_PIN_RESET; }
void BSP_Buzzer_Set(bool on) { HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, on ? GPIO_PIN_SET : GPIO_PIN_RESET); }

bool BSP_Key_IsPressed(bsp_key_t key)
{
    if (key >= BSP_KEY_COUNT) return false;
    return HAL_GPIO_ReadPin(keys[key].port, keys[key].pin) == GPIO_PIN_RESET;
}

uint32_t BSP_Key_GetPressedMask(void)
{
    uint32_t mask = 0;
    for (uint32_t i = 0; i < BSP_KEY_COUNT; ++i) if (BSP_Key_IsPressed((bsp_key_t)i)) mask |= 1UL << i;
    return mask;
}
