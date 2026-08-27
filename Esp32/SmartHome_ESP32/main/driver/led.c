/**
 * @file led.c
 * @brief LED 驱动
 * @version 1.0
 */

#include <stdio.h>
#include <stdbool.h>
#include "driver/gpio.h"

#define LED_GPIO_PIN    GPIO_NUM_38  // 根据实际接线修改

static gpio_num_t led_pin;

/**
 * @brief 初始化 LED
 */
void led_init(void)
{
    led_pin = LED_GPIO_PIN;

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << led_pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    printf("[LED] 初始化完成，引脚: GPIO%d\n", led_pin);
}

/**
 * @brief 设置 LED 状态
 * @param state true=亮，false=灭
 */
void led_set(bool state)
{
    gpio_set_level(led_pin, state ? 1 : 0);
}

/**
 * @brief 翻转 LED 状态
 */
void led_toggle(void)
{
    static bool state = false;
    state = !state;
    led_set(state);
}

/**
 * @brief 翻转指定 GPIO 引脚状态
 * @param gpio_num GPIO 引脚号
 */
void gpio_toggle(gpio_num_t gpio_num)
{
    if (gpio_get_level(gpio_num) == 1)
    {
        gpio_set_level(gpio_num, 0);
    }
    else
    {
        gpio_set_level(gpio_num, 1);
    }
}
