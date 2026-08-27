/**
 * @file task_sensor.c
 * @brief 传感器采集任务
 * @version 1.0
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/dht11.h"

/**
 * @brief 传感器采集任务
 * @param pvParameters 任务参数（未使用）
 */
void task_sensor(void *pvParameters)
{
    printf("[SENSOR] 传感器任务启动\n");

    // 初始化 DHT11
    dht11_init();

    while (1)
    {
        // 读取温湿度数据
        float temperature = 0.0f;
        float humidity = 0.0f;

        if (dht11_read(&temperature, &humidity) == 0)
        {
            printf("[SENSOR] 温度: %.1f°C, 湿度: %.1f%%\n", temperature, humidity);
        }
        else
        {
            printf("[SENSOR] DHT11 读取失败\n");
        }

        // 每 2 秒采集一次
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}