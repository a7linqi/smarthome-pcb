/**
 * @file uart_bridge.c
 * @brief ESP32-STM32 UART 通信桥接实现
 * @version 1.0
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "uart_bridge.h"

static const char *TAG = "UART";

/* UART 配置 */
#define UART_PORT       UART_NUM_2
#define UART_TX_PIN     GPIO_NUM_17
#define UART_RX_PIN     GPIO_NUM_16
#define UART_BAUD_RATE  115200
#define UART_BUF_SIZE   1024

/**
 * @brief 初始化 UART2
 */
void uart_bridge_init(void)
{
    ESP_LOGI(TAG, "初始化 UART2 (TX:%d, RX:%d, %d bps)",
             UART_TX_PIN, UART_RX_PIN, UART_BAUD_RATE);

    /* UART 配置 */
    uart_config_t uart_cfg = {
        .baud_rate  = UART_BAUD_RATE,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    /* 安装 UART 驱动 */
    uart_driver_install(UART_PORT, UART_BUF_SIZE, UART_BUF_SIZE, 0, NULL, 0);
    uart_param_config(UART_PORT, &uart_cfg);
    uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    ESP_LOGI(TAG, "UART2 初始化完成");
}

/**
 * @brief 发送指令给 STM32
 */
int uart_bridge_send_cmd(const char *cmd)
{
    if (cmd == NULL) return -1;

    /* 发送指令 + 换行符 */
    char buf[64];
    snprintf(buf, sizeof(buf), "%s\n", cmd);

    int len = uart_write_bytes(UART_PORT, buf, strlen(buf));
    ESP_LOGI(TAG, "发送指令: %s", cmd);

    return (len > 0) ? 0 : -1;
}

/**
 * @brief 从 STM32 接收数据（非阻塞）
 * @param buf 接收缓冲区
 * @param buf_size 缓冲区大小
 * @return 接收到的字节数，0 表示无数据
 */
int uart_bridge_receive(char *buf, int buf_size)
{
    /* 读取数据，超时 100ms */
    int len = uart_read_bytes(UART_PORT, (uint8_t *)buf, buf_size - 1,
                              pdMS_TO_TICKS(100));
    if (len > 0)
    {
        buf[len] = '\0';  /* 添加字符串结束符 */
        ESP_LOGI(TAG, "收到数据: %s", buf);
    }

    return len;
}
