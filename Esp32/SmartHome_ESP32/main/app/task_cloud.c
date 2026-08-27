/**
 * @file task_cloud.c
 * @brief 云平台通信任务（MQTT + UART 桥接）
 * @version 1.0
 *
 * 数据流：
 *   STM32 → UART → ESP32 → MQTT → 巴法云（上报）
 *   巴法云 → MQTT → ESP32 → UART → STM32（下发）
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "cloud/wifi_manager.h"
#include "cloud/mqtt_handler.h"
#include "driver/uart_bridge.h"

static const char *TAG = "CLOUD";

/* 外部声明（在 main.c 中定义的 WiFi 配置） */
extern const char *wifi_ssid;
extern const char *wifi_password;

/**
 * @brief 云平台通信任务
 */
void task_cloud(void *pvParameters)
{
    ESP_LOGI(TAG, "云平台任务启动");

    /* 初始化 UART 桥接 */
    uart_bridge_init();

    /* 连接 WiFi */
    wifi_manager_init();
    if (wifi_manager_connect("Xiaomi14", "qwer123456") != 0)
    {
        ESP_LOGE(TAG, "WiFi 连接失败");
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "WiFi 连接成功");

    /* 连接巴法云 MQTT */
    mqtt_client_init();
    if (mqtt_client_start() != 0)
    {
        ESP_LOGE(TAG, "MQTT 连接失败");
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "MQTT 连接成功");

    /* 主循环：接收 STM32 数据 → 上报巴法云 */
    char uart_buf[256];
    while (1)
    {
        /* 从 UART 接收 STM32 上报的数据 */
        int len = uart_bridge_receive(uart_buf, sizeof(uart_buf));
        if (len > 0)
        {
            /* 转发到巴法云 */
            ESP_LOGI(TAG, "上报数据: %s", uart_buf);
            mqtt_client_publish("smarthome_data", uart_buf);
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
