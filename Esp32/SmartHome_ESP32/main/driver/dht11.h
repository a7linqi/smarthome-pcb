#ifndef __DHT11_H
#define __DHT11_H

/**
 * @brief 初始化 DHT11
 */
void dht11_init(void);

/**
 * @brief 读取 DHT11 数据
 * @param temperature 温度指针
 * @param humidity 湿度指针
 * @return 0 成功，-1 失败
 */
int dht11_read(float *temperature, float *humidity);

#endif /* __DHT11_H */