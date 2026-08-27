#include "led.h"

  // ---- 普通 LED (PB5, 高电平亮) ----
  void LED_On(void)          // 点亮
  {
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_5,GPIO_PIN_SET);
  }

  void LED_Off(void)        // 熄灭
  {
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_5,GPIO_PIN_RESET);
  }

  void LED_Toggle(void)      // 翻转
  {
    GPIO_PinState state = (GPIO_PinState)!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, state);
  }

    // ---- RGB LED (PB6/7/15, 低电平亮) ----
  void RGB_Red(void)        // 亮红色
  {
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_6,GPIO_PIN_RESET);
  }

  void RGB_Green(void)       // 亮绿色
  {
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7,GPIO_PIN_RESET);
  }
  

  void RGB_Blue(void)        // 亮蓝色
  {
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_15,GPIO_PIN_RESET);
  }

  void RGB_Off(void)        // 全灭
  {
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_15,GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_6,GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7,GPIO_PIN_SET);
  }

  void RGB_Set(uint8_t r, uint8_t g, uint8_t b)  // 自定义颜色组合
  {
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_15,(GPIO_PinState)r);
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_6,(GPIO_PinState)g);
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7,(GPIO_PinState)b);
  }

