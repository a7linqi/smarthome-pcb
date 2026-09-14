#include "esp8266.h"

#include <string.h>

#include "delay.h"

#define ESP_RST_PORT GPIOA
#define ESP_RST_PIN  GPIO_Pin_4

static volatile uint8_t rx_buffer[buf_len];
static volatile uint16_t rx_count;
static volatile uint16_t rx_previous_count;

void ESP8266_Init(uint32_t baud_rate)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;
    NVIC_InitTypeDef nvic;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_2;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Pin = GPIO_Pin_3;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    usart.USART_BaudRate = baud_rate;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART2, &usart);

    nvic.NVIC_IRQChannel = USART2_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 6U;
    nvic.NVIC_IRQChannelSubPriority = 0U;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART2, ENABLE);

    gpio.GPIO_Pin = ESP_RST_PIN;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(ESP_RST_PORT, &gpio);

    GPIO_ResetBits(ESP_RST_PORT, ESP_RST_PIN);
    delay_ms(200U);
    GPIO_SetBits(ESP_RST_PORT, ESP_RST_PIN);

    rx_count = 0U;
    rx_previous_count = 0U;
}

void ESP8266_SendData(const uint8_t *data, uint16_t length)
{
    uint16_t index;

    if (data == NULL) {
        return;
    }

    for (index = 0U; index < length; ++index) {
        USART_SendData(USART2, data[index]);
        while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET) { }
    }
}

uint16_t ESP8266_ReadReceived(uint8_t *output, uint16_t capacity)
{
    uint16_t count;
    uint16_t index;

    if ((output == NULL) || (capacity == 0U) || (rx_count == 0U)) {
        return 0U;
    }

    if (rx_count != rx_previous_count) {
        rx_previous_count = rx_count;
        return 0U;
    }

    USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);
    count = rx_count;
    if (count > capacity) {
        count = capacity;
    }

    for (index = 0U; index < count; ++index) {
        output[index] = rx_buffer[index];
    }

    rx_count = 0U;
    rx_previous_count = 0U;
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

    return count;
}

void USART2_IRQHandler(void)
{
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
        const uint8_t byte = (uint8_t)USART_ReceiveData(USART2);

        if (rx_count < sizeof(rx_buffer)) {
            rx_buffer[rx_count++] = byte;
        } else {
            rx_count = 0U;
            rx_previous_count = 0U;
        }

        USART_ClearITPendingBit(USART2, USART_IT_RXNE);
    }
}
