/**
 * @file task_actuator.c
 * @brief 执行器控制任务
 * @version 1.0
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/led.h"
#include "driver/buzzer.h"
#include "driver/servo.h"

/**
 * @brief 执行器控制任务
 * @param pvParameters 任务参数（未使用）
 */
void task_actuator(void *pvParameters)
{
    printf("[ACTUATOR] 执行器任务启动\n");

    // 初始化执行器
    led_init();
    buzzer_init();
    servo_init();

    while (1)
    {
        // TODO: 接收云平台下发的控制命令
        // TODO: 控制 LED/Buzzer/Servo

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}