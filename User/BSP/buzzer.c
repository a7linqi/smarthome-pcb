#include "buzzer.h"

// ---- 蜂鸣器 (PB14, 高电平响) ----
void Buzzer_On(void)       // 响（低电平触发）
{
  HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
}

void Buzzer_Off(void)      // 不响（高电平）
{
  HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
}

void Buzzer_Toggle(void)   // 翻转
{
  GPIO_PinState state = (GPIO_PinState)!HAL_GPIO_ReadPin(BUZZER_GPIO_Port, BUZZER_Pin);
  HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, state);
}

void Buzzer_Beep(uint16_t ms)  // 响指定毫秒后自动停
{
  Buzzer_On();
  HAL_Delay(ms);
  Buzzer_Off();
}
