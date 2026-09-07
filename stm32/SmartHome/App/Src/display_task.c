#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "bsp_sensors.h"
#include "bsp_oled.h"
#include "sensor_task.h"
#include "uart_task.h"

extern QueueHandle_t displayQueue;

void Display_Task(void *params)
{
    BSP_OLED_Init();
    bsp_sensor_snapshot_t snap;
    while(1)
    {
    BSP_OLED_Clear();
    if(xQueueReceive(displayQueue,&snap,portMAX_DELAY) == pdTRUE)
    {
        BSP_OLED_ShowString(0, 0, "Temp:");
        BSP_OLED_ShowSignedNum(
            48, 0,
            snap.temperature_centi_c / 100,
            2
        );

        BSP_OLED_ShowString(0, 16, "Humi:");
        BSP_OLED_ShowNum(
            48, 16,
            snap.humidity_centi_pct / 100,
            2
        );

        BSP_OLED_ShowString(0, 32, "Light:");
        BSP_OLED_ShowNum(
            56, 32,
            snap.light_raw,
            4
        );
        BSP_OLED_ShowString(0, 48, Uart_IsEspOnline() ? "ESP:OK " : "ESP:OFF");
        BSP_OLED_Refresh();
    }
    }

}
