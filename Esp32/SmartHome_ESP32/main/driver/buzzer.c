/**
 * @file buzzer.c
 * @brief 蜂鸣器驱动
 * @version 1.0
 */

#include <stdio.h>
#include <stdbool.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BUZZER_GPIO_PIN    GPIO_NUM_39  // 根据实际接线修改

static gpio_num_t buzzer_pin;

/**
 * @brief 初始化蜂鸣器
 */
void buzzer_init(void)
{
    buzzer_pin = BUZZER_GPIO_PIN;

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << buzzer_pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    printf("[BUZZER] 初始化完成，引脚: GPIO%d\n", buzzer_pin);
}

/**
 * @brief 设置蜂鸣器状态
 * @param state true=响，false=停
 */
void buzzer_set(bool state)
{
    gpio_set_level(buzzer_pin, state ? 1 : 0);
}

/**
 * @brief 蜂鸣器鸣响指定时长
 * @param duration_ms 鸣响时长（毫秒）
 */
void buzzer_beep(int duration_ms)
{
    buzzer_set(true);
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    buzzer_set(false);
}
