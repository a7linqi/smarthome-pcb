#include "bsp_dht11.h"
#include "bsp_delay.h"
#include "main.h"

static void dq_mode(uint32_t mode)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = DHT11_DATA_Pin; gpio.Mode = mode; gpio.Pull = GPIO_PULLUP; gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_DATA_GPIO_Port, &gpio);
}

static bsp_status_t wait_level(GPIO_PinState level, uint32_t timeout_us)
{
    while (HAL_GPIO_ReadPin(DHT11_DATA_GPIO_Port, DHT11_DATA_Pin) != level) {
        if (timeout_us-- == 0U) return BSP_TIMEOUT;
        BSP_DelayUs(1);
    }
    return BSP_OK;
}

static bsp_status_t read_byte(uint8_t *value)
{
    uint8_t out = 0;
    for (uint8_t i = 0; i < 8; ++i) {
        if (wait_level(GPIO_PIN_SET, 100) != BSP_OK) return BSP_TIMEOUT;
        BSP_DelayUs(35); out <<= 1;
        if (HAL_GPIO_ReadPin(DHT11_DATA_GPIO_Port, DHT11_DATA_Pin) == GPIO_PIN_SET) out |= 1U;
        if (wait_level(GPIO_PIN_RESET, 100) != BSP_OK) return BSP_TIMEOUT;
    }
    *value = out; return BSP_OK;
}

bsp_status_t BSP_DHT11_Read(bsp_dht11_data_t *data)
{
    uint8_t b[5];
    if (!data) return BSP_INVALID_ARG;
    dq_mode(GPIO_MODE_OUTPUT_OD); HAL_GPIO_WritePin(DHT11_DATA_GPIO_Port, DHT11_DATA_Pin, GPIO_PIN_RESET);
    HAL_Delay(20); HAL_GPIO_WritePin(DHT11_DATA_GPIO_Port, DHT11_DATA_Pin, GPIO_PIN_SET); BSP_DelayUs(30);
    dq_mode(GPIO_MODE_INPUT);
    if (wait_level(GPIO_PIN_RESET, 100) != BSP_OK || wait_level(GPIO_PIN_SET, 100) != BSP_OK || wait_level(GPIO_PIN_RESET, 100) != BSP_OK) return BSP_TIMEOUT;
    for (uint8_t i = 0; i < 5; ++i) if (read_byte(&b[i]) != BSP_OK) return BSP_TIMEOUT;
    if ((uint8_t)(b[0] + b[1] + b[2] + b[3]) != b[4]) return BSP_ERROR;
    data->humidity_pct = b[0]; data->temperature_c = b[2]; return BSP_OK;
}
