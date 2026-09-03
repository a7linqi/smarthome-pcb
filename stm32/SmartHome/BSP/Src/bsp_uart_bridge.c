#include "bsp_uart_bridge.h"
#include "stm32f1xx_hal.h"

#define RX_SIZE 256U
extern UART_HandleTypeDef huart3;
static uint8_t rx_byte, rx_buf[RX_SIZE]; static volatile uint16_t head, tail;

bsp_status_t BSP_UartBridge_Init(void)
{
    head = tail = 0;
    return HAL_UART_Receive_IT(&huart3, &rx_byte, 1) == HAL_OK ? BSP_OK : BSP_ERROR;
}
size_t BSP_UartBridge_Available(void) { return (uint16_t)(head - tail) & (RX_SIZE - 1U); }
size_t BSP_UartBridge_Read(uint8_t *data, size_t capacity)
{
    size_t n = 0; if (!data) return 0;
    while (n < capacity && tail != head) { data[n++] = rx_buf[tail]; tail = (tail + 1U) & (RX_SIZE - 1U); }
    return n;
}
bsp_status_t BSP_UartBridge_Write(const uint8_t *data, size_t length, uint32_t timeout_ms)
{
    if (!data || !length) return BSP_INVALID_ARG;
    return HAL_UART_Transmit(&huart3, (uint8_t *)data, (uint16_t)length, timeout_ms) == HAL_OK ? BSP_OK : BSP_ERROR;
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3) {
        uint16_t next = (head + 1U) & (RX_SIZE - 1U);
        if (next != tail) { rx_buf[head] = rx_byte; head = next; }
        (void)HAL_UART_Receive_IT(&huart3, &rx_byte, 1);
    }
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3) (void)HAL_UART_Receive_IT(&huart3, &rx_byte, 1);
}
