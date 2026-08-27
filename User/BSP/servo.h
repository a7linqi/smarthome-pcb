#ifndef __SERVO_H
#define __SERVO_H

#include "main.h"

/*
 * 舵机门控制驱动
 * 控制引脚：PA6（TIM3 通道1）
 * 功能：模拟门的开关
 * 状态：0度=关门，90度=开门
 */

/* 舵机初始化
 * 功能：启动 PWM 输出，设置初始状态为关门
 * 调用时机：程序开始时调用一次
 * 示例：Servo_Init();
 */
void Servo_Init(void);

/* 开门
 * 功能：舵机转到 90 度，模拟开门
 * 示例：Door_Open();
 */
void Door_Open(void);

/* 关门
 * 功能：舵机转到 0 度，模拟关门
 * 示例：Door_Close();
 */
void Door_Close(void);

/* 切换门状态
 * 功能：如果门是开的就关，如果门是关的就开
 * 示例：Door_Toggle();  // 按一次开，再按一次关
 */
void Door_Toggle(void);

/* 获取门的状态
 * 返回值：0=关闭，1=打开
 * 示例：if (Door_GetStatus() == 1) { 门是开的 }
 */
uint8_t Door_GetStatus(void);

#endif /* __SERVO_H */
