#ifndef __BUZZER_H
#define __BUZZER_H

#include "main.h"

// ---- 蜂鸣器 (PB14, 高电平响) ----
void Buzzer_On(void);       // 响
void Buzzer_Off(void);      // 不响
void Buzzer_Toggle(void);   // 翻转
void Buzzer_Beep(uint16_t ms);  // 响指定毫秒后自动停

#endif /* __BUZZER_H */
