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
QueueHandle_t alarmQueue;
QueueHandle_t uartSensorQueue;
QueueHandle_t remoteControlQueue;
QueueHandle_t controlAckQueue;
QueueHandle_t deviceStateQueue;

void APP_Init(void)
{
    BaseType_t result;

    /* 长度为1的队列保存各消费者需要的最新传感器快照。 */
    sensorQueue  = xQueueCreate(1, sizeof(bsp_sensor_snapshot_t));
    displayQueue = xQueueCreate(1, sizeof(bsp_sensor_snapshot_t));
    alarmQueue   = xQueueCreate(1, sizeof(bsp_sensor_snapshot_t)); 
    uartSensorQueue = xQueueCreate(1, sizeof(bsp_sensor_snapshot_t));
    remoteControlQueue = xQueueCreate(4, sizeof(app_control_command_t));
    controlAckQueue = xQueueCreate(4, sizeof(app_control_ack_t));
    deviceStateQueue = xQueueCreate(1, sizeof(app_device_state_t));

    /* 创建失败时立即停在断言处，避免系统以“缺任务”的状态继续运行。 */
    configASSERT(sensorQueue != NULL);
    configASSERT(displayQueue != NULL);
    configASSERT(alarmQueue != NULL);
    configASSERT(uartSensorQueue != NULL);
    configASSERT(remoteControlQueue != NULL);
    configASSERT(controlAckQueue != NULL);
    configASSERT(deviceStateQueue != NULL);

    result = xTaskCreate(Sensor_Task, "Sensor", 128, NULL, 4, NULL);
    configASSERT(result == pdPASS);
    result = xTaskCreate(Display_Task, "Display", 128, NULL, 1, NULL);
    configASSERT(result == pdPASS);
    result = xTaskCreate(Control_Task, "Control", 128, NULL, 3, NULL);
    configASSERT(result == pdPASS);
    result = xTaskCreate(Alarm_Task, "Alarm", 128, NULL, 2, NULL);
    configASSERT(result == pdPASS);
    result = xTaskCreate(Uart_Task, "Uart", 192, NULL, 3, NULL);
    configASSERT(result == pdPASS);
}
