#ifndef BSP_STEPPER_H
#define BSP_STEPPER_H

#include <stdbool.h>
#include <stdint.h>
#include "bsp_status.h"

typedef enum { BSP_STEPPER_CLOSE = -1, BSP_STEPPER_OPEN = 1 } bsp_stepper_direction_t;
void BSP_Stepper_Init(void);
bsp_status_t BSP_Stepper_Move(bsp_stepper_direction_t direction, uint32_t steps, uint32_t interval_ms);
void BSP_Stepper_Process(uint32_t now_ms);
bool BSP_Stepper_IsBusy(void);
void BSP_Stepper_Stop(void);

#endif
