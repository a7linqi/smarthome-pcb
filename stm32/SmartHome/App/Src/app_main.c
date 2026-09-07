#include "app_main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "sensor_task.h"
#include "bsp_sensors.h"
#include "display_task.h"
#include "control_task.h"
#include "alarm_task.h"
#include "uart_task.h"

/*
 * This file belongs to the application layer.
 * Replace this empty implementation with task/queue/mutex creation.
 * BSP headers under BSP/Inc are the only hardware interface needed by APP.
 */

QueueHandle_t sensorQueue;
QueueHandle_t displayQueue;
QueueHandle_t controlQueue;
QueueHandle_t alarmQueue;
QueueHandle_t uartSensorQueue;
QueueHandle_t remoteControlQueue;
QueueHandle_t controlAckQueue;
QueueHandle_t deviceStateQueue;

void APP_Init(void)
{
    //创建队列
    sensorQueue  = xQueueCreate(1, sizeof(bsp_sensor_snapshot_t));
    controlQueue = xQueueCreate(1, sizeof(bsp_sensor_snapshot_t));
    displayQueue = xQueueCreate(1, sizeof(bsp_sensor_snapshot_t));
    alarmQueue   = xQueueCreate(1, sizeof(bsp_sensor_snapshot_t)); 
    uartSensorQueue = xQueueCreate(1, sizeof(bsp_sensor_snapshot_t));
    remoteControlQueue = xQueueCreate(4, sizeof(app_control_command_t));
    controlAckQueue = xQueueCreate(4, sizeof(app_control_ack_t));
    deviceStateQueue = xQueueCreate(1, sizeof(app_device_state_t));

    //创建任务
    xTaskCreate(Sensor_Task, "Sensor", 128, NULL, 4, NULL);
    xTaskCreate(Display_Task,"Display",128, NULL, 1, NULL);
    xTaskCreate(Control_Task,"Control",128, NULL, 3, NULL);
    xTaskCreate(Alarm_Task,"Alarm",128, NULL, 2, NULL);
    xTaskCreate(Uart_Task,"Uart",192, NULL, 3, NULL);
}
