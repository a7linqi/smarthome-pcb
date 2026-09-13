#ifndef _ESP8266_H_
#define _ESP8266_H_
//单片机头文件
#include "stm32f10x.h"
#include "esp8266_config.h"
#define REV_OK		0	//接收完成标志
#define REV_WAIT	1	//接收未完成标志
#define buf_len    256  //串口接受总长度

#define Bamfa_USART1		        0
#define Bamfa_USART2		        1
#define Bamfa_USART3		        0

#define Bamfa_USART		            USART2


//ESP-01S复位引脚定义，需要改的地方
#define ESP01S_RST_RCC_CLK		RCC_APB2Periph_GPIOA
#define ESP01S_RST_PROT		    GPIOA
#define ESP01S_RST_PIN		    GPIO_Pin_4


//WiFi名称密码修改
#define ESP8266_WIFI_INFO  "AT+CWJAP=\"" ESP8266_WIFI_SSID "\",\"" ESP8266_WIFI_PASSWORD "\"\r\n"

//巴法云网络端口（不用改）
#define ESP8266_ONENET_INFO	"AT+CIPSTART=\"TCP\",\"tcp.bemfa.com\",8344\r\n"

//巴法云用户秘钥修改
#define BEMFA_ID  ESP8266_DEVICE_UID

#define DATA_TOPIC "data"

//巴法云订阅主题指令
#define ESP8266_TOPIC  "cmd=1&uid=" ESP8266_DEVICE_UID "&topic=data,control\r\n"
                         

void ESP8266_Clear(void);

_Bool ESP8266_WaitRecive(void);

_Bool ESP8266_SendCmd(char *cmd, char *res);

void Usart_SendString(USART_TypeDef *USARTx, unsigned char *str, unsigned short len);

void ESP8266_SendData(unsigned char *data);

void ESP8266_Init(unsigned int bound);

void USART2_IRQHandler(void);

void mode_choice(void);

#endif
