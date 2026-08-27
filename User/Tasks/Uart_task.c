/**
 * @file Uart_task.c
 * @brief STM32-ESP32 UART 通信任务
 * @version 1.0
 *
 * 通信协议：
 *   STM32 → ESP32: "T:25.6,L:2048,LED:ON,FAN:OFF,DOOR:OFF\n"
 *   ESP32 → STM32: "LED:ON\n" / "LED:OFF\n" / "DOOR:ON\n" / "DOOR:OFF\n"
 *
 * 硬件接线：
 *   STM32 PA9  (USART1_TX) → ESP32 GPIO16 (RX2)
 *   STM32 PA10 (USART1_RX) → ESP32 GPIO17 (TX2)
 */

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "cmsis_os.h"
#include "usart.h"
#include "led.h"
#include "servo.h"
#include "app.h"
#include <stdio.h>
#include <string.h>

/* 接收缓冲区 */
static uint8_t uart_rx_buf[64];
static uint8_t uart_rx_byte;
static volatile uint8_t uart_rx_index = 0;
static char uart_rx_line[64];

/**
 * @brief 启动 USART1 中断接收
 */
void UART_StartReceive(void)
{
    /* 开启接收中断，每次收 1 个字节 */
    HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);
}

/**
 * @brief USART1 接收完成回调（在 stm32f1xx_it.c 中调用）
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        /* 收到换行符 = 一帧结束 */
        if (uart_rx_byte == '\n')
        {
            uart_rx_line[uart_rx_index] = '\0';
            uart_rx_index = 0;

            /* 解析指令 */
            if (strcmp(uart_rx_line, "LED:ON") == 0)
            {
                xSemaphoreTake(dataMutex, portMAX_DELAY);
                data_t.led = 1;
                xSemaphoreGive(dataMutex);
                LED_On();
            }
            else if (strcmp(uart_rx_line, "LED:OFF") == 0)
            {
                xSemaphoreTake(dataMutex, portMAX_DELAY);
                data_t.led = 0;
                xSemaphoreGive(dataMutex);
                LED_Off();
            }
            else if (strcmp(uart_rx_line, "DOOR:ON") == 0)
            {
                xSemaphoreTake(dataMutex, portMAX_DELAY);
                data_t.door = 1;
                xSemaphoreGive(dataMutex);
                Door_Open();
            }
            else if (strcmp(uart_rx_line, "DOOR:OFF") == 0)
            {
                xSemaphoreTake(dataMutex, portMAX_DELAY);
                data_t.door = 0;
                xSemaphoreGive(dataMutex);
                Door_Close();
            }
        }
        else
        {
            /* 缓存字节 */
            if (uart_rx_index < sizeof(uart_rx_line) - 1)
            {
                uart_rx_line[uart_rx_index++] = uart_rx_byte;
            }
        }

        /* 继续接收下一个字节 */
        HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);
    }
}

/**
 * @brief UART 通信任务
 *        周期性上报传感器数据给 ESP32
 */
void UartTask(void *params)
{
    (void)params;
    char tx_buf[128];

    /* 启动中断接收 */
    UART_StartReceive();

    while (1)
    {
        /* 读取数据（加锁拷贝） */
        xSemaphoreTake(dataMutex, portMAX_DELAY);
        uint8_t temp  = data_t.temp;
        uint8_t humi  = data_t.humi;
        uint16_t light = data_t.light;
        uint8_t led   = data_t.led;
        uint8_t door  = data_t.door;
        xSemaphoreGive(dataMutex);

        /* 格式化上报字符串 */
        snprintf(tx_buf, sizeof(tx_buf),
                 "T:%d,L:%u,LED:%s,DOOR:%s\n",
                 temp, light,
                 led ? "ON" : "OFF",
                 door ? "ON" : "OFF");

        /* 发送给 ESP32 */
        HAL_UART_Transmit(&huart1, (uint8_t *)tx_buf, strlen(tx_buf), 100);

        /* 每 3 秒上报一次 */
        osDelay(3000);
    }
}
