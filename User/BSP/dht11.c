#include "dht11.h"

// 微秒延时（72MHz 主频下）
static void Delay_us(uint32_t us)
{
  uint32_t i;
  while (us--)
  {
    i = 72;  // 72MHz 时钟，约 1us
    while (i--);
  }
}

// 设置 PA0 为输出模式
static void DHT11_Pin_Output(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = DHT11_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

// 设置 PA1 为输入模式
static void DHT11_Pin_Input(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = DHT11_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;  // 外部上拉，不需要内部上拉
  HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

// 初始化 DHT11（PA0 默认设为输入）
void DHT11_Init(void)
{
  DHT11_Pin_Input();
}

// 发送起始信号
void DHT11_Start(void)
{
  DHT11_Pin_Output();                                    // 切换为输出
  HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, RESET);      // 拉低
  HAL_Delay(18);                                         // 延时 18ms
  HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, SET);        // 拉高
  Delay_us(30);                                          // 延时 30us
  DHT11_Pin_Input();                                     // 切换为输入，等 DHT11 响应
}

// 等待响应（返回 0=成功，1=超时）
static uint8_t DHT11_WaitResponse(void)
{
  uint32_t timeout = 10000;
  // 等待 DHT11 拉低（应答信号）
  while(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == SET)
  {
    if(--timeout == 0) return 1;
  }
  timeout = 10000;
  // 等待 DHT11 拉高
  while(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == RESET)
  {
    if(--timeout == 0) return 1;
  }
  return 0;
}

// 读取一个字节（8位）
uint8_t DHT11_ReadByte(void)
{
  uint8_t i, byte = 0x00;
  uint32_t timeout;
  for(i = 0; i < 8; i++)
  {
    byte <<= 1;                                          // 左移一位
    timeout = 10000;
    while(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == RESET) // 等待低电平结束
    {
      if(--timeout == 0) return 0;                       // 超时返回 0
    }
    Delay_us(50);                                        // 延时 50us 判断数据位
    if(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == SET)  // 高电平持续时间长为 1
    {
      byte |= 0x01;
    }
    timeout = 10000;
    while(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == SET) // 等待高电平结束
    {
      if(--timeout == 0) return 0;                       // 超时返回 0
    }
  }
  return byte;
}

// 调试用：存储原始数据
uint8_t dht11_raw[5];  // humi_h, humi_l, temp_h, temp_l, checksum

// 读取温湿度数据
// 返回值：0=成功，1=无响应，2=校验失败
uint8_t DHT11_ReadData(uint8_t *temp, uint8_t *humi)
{
  uint8_t humi_high, humi_low, temp_high, temp_low, checksum;

  DHT11_Start();                                         // 发送起始信号
  if(DHT11_WaitResponse() != 0) return 1;               // 无响应，返回1

  // 读取 5 个字节
  humi_high = DHT11_ReadByte();                          // 湿度高位
  humi_low  = DHT11_ReadByte();                          // 湿度低位
  temp_high = DHT11_ReadByte();                          // 温度高位
  temp_low  = DHT11_ReadByte();                          // 温度低位
  checksum  = DHT11_ReadByte();                          // 校验和

  // 保存原始数据用于调试
  dht11_raw[0] = humi_high;
  dht11_raw[1] = humi_low;
  dht11_raw[2] = temp_high;
  dht11_raw[3] = temp_low;
  dht11_raw[4] = checksum;

  // 校验：前四字节之和应等于校验和
  if(checksum != humi_high + humi_low + temp_high + temp_low)
  {
    return 2;                                            // 校验失败，返回2
  }

  *humi = humi_high;                                     // 返回湿度整数部分
  *temp = temp_high;                                     // 返回温度整数部分
  return 0;                                              // 成功
}
