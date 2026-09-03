#ifndef BSP_BOARD_H
#define BSP_BOARD_H

#include <stdbool.h>
#include <stdint.h>
#include "bsp_status.h"

typedef enum {
    BSP_KEY_UP = 0,
    BSP_KEY_1,
    BSP_KEY_2,
    BSP_KEY_0,
    BSP_KEY_COUNT
} bsp_key_t;

bsp_status_t BSP_Board_Init(void);
void BSP_Light_Set(bool on);
bool BSP_Light_Get(void);
void BSP_Buzzer_Set(bool on);
bool BSP_Key_IsPressed(bsp_key_t key);
uint32_t BSP_Key_GetPressedMask(void);

#endif
