#ifndef BSP_OLED_H
#define BSP_OLED_H

#include <stdint.h>
#include "bsp_status.h"

#define BSP_OLED_WIDTH 128U
#define BSP_OLED_HEIGHT 64U
bsp_status_t BSP_OLED_Init(void);
void BSP_OLED_Clear(void);
void BSP_OLED_SetPixel(uint8_t x, uint8_t y, uint8_t on);
void BSP_OLED_ShowChar(uint8_t x, uint8_t y, char character);
void BSP_OLED_ShowString(uint8_t x, uint8_t y, const char *string);
void BSP_OLED_ShowNum(uint8_t x, uint8_t y, uint32_t number, uint8_t length);
void BSP_OLED_ShowSignedNum(uint8_t x, uint8_t y, int32_t number, uint8_t length);
bsp_status_t BSP_OLED_Refresh(void);
uint8_t *BSP_OLED_GetBuffer(void);

#endif
