#ifndef __DHT11_H
#define __DHT11_H

#include "main.h"

// DHT11 引脚定义（PA1）
#define DHT11_PORT      GPIOA
#define DHT11_PIN       GPIO_PIN_1

// 调试用：存储原始数据
extern uint8_t dht11_raw[5];

// 函数声明
void DHT11_Init(void);          // 初始化
void DHT11_Start(void);         // 发送起始信号
uint8_t DHT11_ReadByte(void);   // 读取一个字节
uint8_t DHT11_ReadData(uint8_t *temp, uint8_t *humi);  // 读取温湿度

#endif /* __DHT11_H */
