#ifndef __LED_H
#define __LED_H

#include <stdbool.h>
#include "driver/gpio.h"

/**
 * @brief 初始化 LED
 */
void led_init(void);

/**
 * @brief 设置 LED 状态
 * @param state true=亮，false=灭
 */
void led_set(bool state);

/**
 * @brief 翻转 LED 状态
 */
void led_toggle(void);

/**
 * @brief 翻转指定 GPIO 引脚状态
 * @param gpio_num GPIO 引脚号
 */
void gpio_toggle(gpio_num_t gpio_num);

#endif /* __LED_H */