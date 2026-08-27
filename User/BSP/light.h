#ifndef __LIGHT_H
#define __LIGHT_H

#include "main.h"

// 光敏传感器（PA1，ADC1 通道1）
uint16_t Light_Read(void);   // 读取光照强度（0-4095）

#endif /* __LIGHT_H */
