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
            /* 各队列只保留最新快照，慢消费者不会阻塞整个采集任务。 */
            xQueueOverwrite(sensorQueue, &snap);
            xQueueOverwrite(displayQueue, &snap);
            xQueueOverwrite(alarmQueue, &snap);
            xQueueOverwrite(uartSensorQueue, &snap);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
