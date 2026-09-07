#include "alarm_task.h"

#include "FreeRTOS.h"
#include "queue.h"

#include "bsp_sensors.h"
#include "bsp_buzzer.h"

extern QueueHandle_t alarmQueue;


void Alarm_Task(void *params)
{
    bsp_sensor_snapshot_t data;
    (void)params;  

    while(1)
    {
        if(xQueueReceive(alarmQueue,&data,portMAX_DELAY) == pdTRUE)
        {
            if(data.temperature_centi_c >3500)
            {
                BSP_Buzzer_SetMode(BSP_BUZZER_ON,0);  //常开
            }
            else
            {
                BSP_Buzzer_SetMode(BSP_BUZZER_OFF,0);
            }
        }
    }
}
