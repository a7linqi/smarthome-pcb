#include "bsp_oled.h"
#include "bsp_delay.h"
#include "main.h"
#include <string.h>

static uint8_t fb[BSP_OLED_WIDTH * BSP_OLED_HEIGHT / 8U];
static void tick(void) { HAL_GPIO_WritePin(OLED_SCL_GPIO_Port,OLED_SCL_Pin,GPIO_PIN_SET); BSP_DelayUs(1); HAL_GPIO_WritePin(OLED_SCL_GPIO_Port,OLED_SCL_Pin,GPIO_PIN_RESET); }
static void byte(uint8_t v) { for(uint8_t i=0;i<8;i++){ HAL_GPIO_WritePin(OLED_SDA_GPIO_Port,OLED_SDA_Pin,(v&0x80)?GPIO_PIN_SET:GPIO_PIN_RESET); tick(); v<<=1; } }
static void start(void) { HAL_GPIO_WritePin(OLED_SDA_GPIO_Port,OLED_SDA_Pin,GPIO_PIN_SET); HAL_GPIO_WritePin(OLED_SCL_GPIO_Port,OLED_SCL_Pin,GPIO_PIN_SET); BSP_DelayUs(1); HAL_GPIO_WritePin(OLED_SDA_GPIO_Port,OLED_SDA_Pin,GPIO_PIN_RESET); HAL_GPIO_WritePin(OLED_SCL_GPIO_Port,OLED_SCL_Pin,GPIO_PIN_RESET); }
static void stop(void) { HAL_GPIO_WritePin(OLED_SDA_GPIO_Port,OLED_SDA_Pin,GPIO_PIN_RESET); HAL_GPIO_WritePin(OLED_SCL_GPIO_Port,OLED_SCL_Pin,GPIO_PIN_SET); BSP_DelayUs(1); HAL_GPIO_WritePin(OLED_SDA_GPIO_Port,OLED_SDA_Pin,GPIO_PIN_SET); }
static void write(uint8_t control,const uint8_t *p,uint16_t n) { start(); byte(0x78); byte(control); while(n--) byte(*p++); stop(); }
static void cmd(uint8_t c) { write(0,&c,1); }

bsp_status_t BSP_OLED_Init(void)
{
    static const uint8_t init[]={0xAE,0x20,0x02,0xB0,0xC8,0x00,0x10,0x40,0x81,0x7F,0xA1,0xA6,0xA8,0x3F,0xA4,0xD3,0x00,0xD5,0x80,0xD9,0xF1,0xDA,0x12,0xDB,0x40,0x8D,0x14,0xAF};
    HAL_Delay(100); for(uint8_t i=0;i<sizeof init;i++) cmd(init[i]); BSP_OLED_Clear(); return BSP_OLED_Refresh();
}
void BSP_OLED_Clear(void) { memset(fb,0,sizeof fb); }
void BSP_OLED_SetPixel(uint8_t x,uint8_t y,uint8_t on) { if(x>=128||y>=64)return; if(on)fb[x+(y>>3)*128]|=1U<<(y&7); else fb[x+(y>>3)*128]&=~(1U<<(y&7)); }
bsp_status_t BSP_OLED_Refresh(void) { for(uint8_t p=0;p<8;p++){ cmd(0xB0+p); cmd(0x00); cmd(0x10); write(0x40,&fb[p*128],128); } return BSP_OK; }
uint8_t *BSP_OLED_GetBuffer(void) { return fb; }
