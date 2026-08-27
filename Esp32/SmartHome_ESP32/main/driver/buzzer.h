#ifndef __BUZZER_H
#define __BUZZER_H

#include <stdbool.h>

/**
 * @brief 初始化蜂鸣器
 */
void buzzer_init(void);

/**
 * @brief 设置蜂鸣器状态
 * @param state true=响，false=停
 */
void buzzer_set(bool state);

/**
 * @brief 蜂鸣器鸣响指定时长
 * @param duration_ms 鸣响时长（毫秒）
 */
void buzzer_beep(int duration_ms);

#endif /* __BUZZER_H */