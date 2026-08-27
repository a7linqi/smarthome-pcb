/**
 * @file servo.c
 * @brief 舵机驱动
 * @version 1.0
 */

#include <stdio.h>
#include "driver/ledc.h"

#define SERVO_GPIO_PIN      GPIO_NUM_40  // 根据实际接线修改
#define SERVO_LEDC_CHANNEL  LEDC_CHANNEL_0
#define SERVO_LEDC_TIMER    LEDC_TIMER_0
#define SERVO_FREQ_HZ       50           // 50Hz = 20ms 周期
#define SERVO_RESOLUTION    LEDC_TIMER_14_BIT  // ESP32-S3 最大支持 14 位

/**
 * @brief 初始化舵机
 */
void servo_init(void)
{
    // 配置 LEDC 定时器
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = SERVO_RESOLUTION,
        .timer_num = SERVO_LEDC_TIMER,
        .freq_hz = SERVO_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_conf);

    // 配置 LEDC 通道
    ledc_channel_config_t channel_conf = {
        .gpio_num = SERVO_GPIO_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = SERVO_LEDC_CHANNEL,
        .timer_sel = SERVO_LEDC_TIMER,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&channel_conf);

    printf("[SERVO] 初始化完成，引脚: GPIO%d\n", SERVO_GPIO_PIN);
}

/**
 * @brief 设置舵机角度
 * @param angle 角度（0-180）
 */
void servo_set_angle(int angle)
{
    // 将角度转换为占空比
    // 0.5ms = 0°，1.5ms = 90°，2.5ms = 180°
    // 20ms 周期，16位分辨率
    uint32_t duty = (uint32_t)((0.5 + angle / 90.0) / 20.0 * 16384);
    
    ledc_set_duty(LEDC_LOW_SPEED_MODE, SERVO_LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, SERVO_LEDC_CHANNEL);
}
