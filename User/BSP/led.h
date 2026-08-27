#ifndef __LED_H
#define __LED_H

#include "main.h"

  // ---- 普通 LED (PB5, 高电平亮) ----
  void LED_On(void);          // 点亮
  void LED_Off(void);         // 熄灭
  void LED_Toggle(void);      // 翻转

  // ---- RGB LED (PB6/7/15, 低电平亮) ----
  void RGB_Red(void);         // 亮红色
  void RGB_Green(void);       // 亮绿色
  void RGB_Blue(void);        // 亮蓝色
  void RGB_Off(void);         // 全灭
  void RGB_Set(uint8_t r, uint8_t g, uint8_t b);  // 自定义颜色组合


#endif /* __LED_H */
