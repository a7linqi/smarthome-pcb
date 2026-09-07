#ifndef BSP_BUZZER_H
#define BSP_BUZZER_H

#include <stdint.h>

/* 蜂鸣器模式：OFF=关闭, ON=常响, BLINK=闪烁（需配合 Process 函数定时调用） */
typedef enum { BSP_BUZZER_OFF = 0, BSP_BUZZER_ON, BSP_BUZZER_BLINK } bsp_buzzer_mode_t;
void BSP_Buzzer_SetMode(bsp_buzzer_mode_t mode, uint32_t half_period_ms);
void BSP_Buzzer_Process(uint32_t now_ms);

#endif
