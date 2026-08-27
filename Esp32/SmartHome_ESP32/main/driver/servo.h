#ifndef __SERVO_H
#define __SERVO_H

/**
 * @brief 初始化舵机
 */
void servo_init(void);

/**
 * @brief 设置舵机角度
 * @param angle 角度（0-180）
 */
void servo_set_angle(int angle);

#endif /* __SERVO_H */