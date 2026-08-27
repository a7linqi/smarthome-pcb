/**
 * @file task_display.c
 * @brief OLED显示任务
 * @version 1.0
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @brief OLED显示任务
 * @param pvParameters 任务参数（未使用）
 */
void task_display(void *pvParameters)
{
    printf("[DISPLAY] 显示任务启动\n");

    // TODO: 初始化 OLED

    while (1)
    {
        // TODO: 更新显示内容
        // - 温湿度数据
        // - 设备状态
        // - 网络状态

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}