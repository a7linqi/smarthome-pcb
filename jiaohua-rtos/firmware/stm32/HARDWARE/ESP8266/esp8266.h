#ifndef ESP8266_H
#define ESP8266_H

#include <stdint.h>
#include "stm32f10x.h"

#define buf_len 256U

void ESP8266_Init(uint32_t baud_rate);
void ESP8266_SendData(const uint8_t *data, uint16_t length);
uint16_t ESP8266_ReadReceived(uint8_t *output, uint16_t capacity);
void USART2_IRQHandler(void);

#endif
