#include "bsp_sensors.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "bsp_oled.h"

extern QueueHandle_t sensorQueue;
extern QueueHandle_t displayQueue;
extern QueueHandle_t alarmQueue;
extern QueueHandle_t uartSensorQueue;

void Sensor_Task(void *params)
{
    BSP_Sensors_Init();
    bsp_sensor_snapshot_t snap;
    if (BSP_Sensors_Init() != BSP_OK)
    {
        BSP_OLED_Clear();
        BSP_OLED_ShowString(0,0,"error");  
    }
    (void)params;
    while (1)
    {       
        if(BSP_Sensors_Read(&snap) == BSP_OK)
        {
            xQueueSend(sensorQueue,&snap,portMAX_DELAY);
            xQueueSend(displayQueue,&snap,portMAX_DELAY);
            xQueueSend(alarmQueue,&snap,portMAX_DELAY);
            xQueueOverwrite(uartSensorQueue, &snap);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
