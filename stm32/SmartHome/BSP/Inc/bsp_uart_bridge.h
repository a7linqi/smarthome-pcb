#ifndef BSP_UART_BRIDGE_H
#define BSP_UART_BRIDGE_H

#include <stddef.h>
#include <stdint.h>
#include "bsp_status.h"

bsp_status_t BSP_UartBridge_Init(void);
size_t BSP_UartBridge_Available(void);
size_t BSP_UartBridge_Read(uint8_t *data, size_t capacity);
bsp_status_t BSP_UartBridge_Write(const uint8_t *data, size_t length, uint32_t timeout_ms);

#endif
