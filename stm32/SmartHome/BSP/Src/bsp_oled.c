#include "bsp_oled.h"
#include "bsp_oled_font.h"
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

void BSP_OLED_ShowChar(uint8_t x, uint8_t y, char character)
{
    uint8_t column;
    uint8_t row;
    uint8_t glyph_index;

    if ((uint8_t)character < (uint8_t)' ' || (uint8_t)character > (uint8_t)'~') {
        character = '?';
    }
    glyph_index = (uint8_t)character - (uint8_t)' ';

    for (column = 0U; column < 8U; ++column) {
        uint16_t column_bits = (uint16_t)OLED_F8x16[glyph_index][column]
                             | ((uint16_t)OLED_F8x16[glyph_index][column + 8U] << 8U);
        for (row = 0U; row < 16U; ++row) {
            BSP_OLED_SetPixel((uint8_t)(x + column), (uint8_t)(y + row),
                              (uint8_t)((column_bits >> row) & 1U));
        }
    }
}

void BSP_OLED_ShowString(uint8_t x, uint8_t y, const char *string)
{
    if (string == 0) return;
    while (*string != '\0' && x <= (BSP_OLED_WIDTH - 8U)) {
        BSP_OLED_ShowChar(x, y, *string++);
        x = (uint8_t)(x + 8U);
    }
}

static uint32_t pow10_u32(uint8_t exponent)
{
    uint32_t result = 1U;
    while (exponent-- != 0U) result *= 10U;
    return result;
}

void BSP_OLED_ShowNum(uint8_t x, uint8_t y, uint32_t number, uint8_t length)
{
    uint8_t index;
    if (length == 0U || length > 10U) return;
    for (index = 0U; index < length; ++index) {
        uint32_t divisor = pow10_u32((uint8_t)(length - index - 1U));
        BSP_OLED_ShowChar((uint8_t)(x + index * 8U), y,
                          (char)('0' + (number / divisor) % 10U));
    }
}

void BSP_OLED_ShowSignedNum(uint8_t x, uint8_t y, int32_t number, uint8_t length)
{
    uint32_t magnitude;
    if (number < 0) {
        BSP_OLED_ShowChar(x, y, '-');
        magnitude = (uint32_t)(-(number + 1)) + 1U;
    } else {
        BSP_OLED_ShowChar(x, y, '+');
        magnitude = (uint32_t)number;
    }
    BSP_OLED_ShowNum((uint8_t)(x + 8U), y, magnitude, length);
}

bsp_status_t BSP_OLED_Refresh(void) { for(uint8_t p=0;p<8;p++){ cmd(0xB0+p); cmd(0x00); cmd(0x10); write(0x40,&fb[p*128],128); } return BSP_OK; }
uint8_t *BSP_OLED_GetBuffer(void) { return fb; }
