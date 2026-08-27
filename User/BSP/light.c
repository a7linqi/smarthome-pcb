#include "light.h"
#include "adc.h"

// 读取光照强度（0-4095，值越大越亮）
uint16_t Light_Read(void)
{
  uint16_t adc_value;

  HAL_ADC_Start(&hadc1);                    // 启动 ADC 转换
  HAL_ADC_PollForConversion(&hadc1, 100);   // 等待转换完成
  adc_value = HAL_ADC_GetValue(&hadc1);     // 读取 ADC 值
  HAL_ADC_Stop(&hadc1);                     // 停止 ADC

  return adc_value;
}
