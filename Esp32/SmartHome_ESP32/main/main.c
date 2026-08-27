#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led.h"
#include "app/task_cloud.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "智能家居系统启动");

    /* 初始化 LED */
    led_init();

    /* 创建云平台通信任务（包含 WiFi + MQTT + UART 桥接） */
    xTaskCreate(task_cloud, "cloud", 4096, NULL, 5, NULL);

    /* 主循环：LED 心跳 */
    while (1)
    {
        led_toggle();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
