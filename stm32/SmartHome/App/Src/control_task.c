#include "control_task.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "stm32f1xx_hal.h"
#include "bsp_keys.h"
#include "bsp_board.h"
#include "bsp_stepper.h"
#include "bsp_sensors.h"
#include "uart_task.h"

extern QueueHandle_t controlQueue;
extern QueueHandle_t sensorQueue;
extern QueueHandle_t remoteControlQueue;
extern QueueHandle_t controlAckQueue;
extern QueueHandle_t deviceStateQueue;

static void publish_state(uint8_t mode, uint8_t door)
{
    app_device_state_t state;
    state.mode = mode;
    state.light_on = BSP_Light_Get() ? 1U : 0U;
    state.door_open = door;
    xQueueOverwrite(deviceStateQueue, &state);
}

void Control_Task(void *params)
{
    uint8_t key;
    uint8_t mode = 0;
    uint8_t door = 0;
    uint8_t dataValid = 0;
    bsp_sensor_snapshot_t data = {0};
    app_control_command_t command;
    app_control_ack_t ack;
    TickType_t last_state_tick = 0;

    (void)params;

    while (1)
    {
        key = BSP_Key_Scan();

        if (xQueueReceive(remoteControlQueue, &command, 0) == pdTRUE)
        {
            ack.sequence = command.sequence;
            ack.status = APP_ACK_OK;
            ack.device = command.device;
            ack.value = command.value;

            if (command.value > 1U)
            {
                ack.status = APP_ACK_INVALID;
            }
            else if (command.device == APP_DEVICE_LIGHT)
            {
                /* 远程控制灯时进入手动模式，避免自动逻辑立即覆盖命令。 */
                mode = 0U;
                BSP_Light_Set(command.value != 0U);
            }
            else if (command.device == APP_DEVICE_MODE)
            {
                mode = command.value;
            }
            else if (command.device == APP_DEVICE_DOOR)
            {
                if (BSP_Stepper_IsBusy())
                {
                    ack.status = APP_ACK_BUSY;
                }
                else if (door != command.value)
                {
                    bsp_stepper_direction_t direction = command.value != 0U
                        ? BSP_STEPPER_OPEN : BSP_STEPPER_CLOSE;
                    if (BSP_Stepper_Move(direction, 2048, 2) == BSP_OK)
                    {
                        door = command.value;
                    }
                    else
                    {
                        ack.status = APP_ACK_BUSY;
                    }
                }
            }
            else
            {
                ack.status = APP_ACK_INVALID;
            }

            xQueueSend(controlAckQueue, &ack, 0);
            publish_state(mode, door);
        }

        /* 有新数据就保存；没有数据也不会阻塞按键扫描。 */
        if (xQueueReceive(sensorQueue, &data, 0) == pdTRUE)
        {
            dataValid = 1;
        }

        /* 按键4：切换手动和自动模式。 */
        if (key == 4)
        {
            mode = !mode;
        }

        if (mode == 0)
        {
            /* 手动模式，按键1控制灯。 */
            if (key == 1)
            {
                BSP_Light_Set(!BSP_Light_Get());
            }
        }
        else
        {
            /* 自动模式，根据光照控制灯。 */
            if (dataValid == 1)
            {
                if (data.light_raw < 1500)
                {
                    BSP_Light_Set(true);
                }
                else if (data.light_raw > 2500)
                {
                    BSP_Light_Set(false);
                }
            }
        }

        /* 按键2：控制开门和关门。 */
        if (key == 2 && BSP_Stepper_IsBusy() == false)
        {
            if (door == 0)
            {
                if (BSP_Stepper_Move(BSP_STEPPER_OPEN, 2048, 2) == BSP_OK)
                {
                    door = 1;
                }
            }
            else
            {
                if (BSP_Stepper_Move(BSP_STEPPER_CLOSE, 2048, 2) == BSP_OK)
                {
                    door = 0;
                }
            }
        }

        /* 按键3留给后面的OLED页面切换。 */

        BSP_Stepper_Process(HAL_GetTick());

        if ((xTaskGetTickCount() - last_state_tick) >= pdMS_TO_TICKS(1000))
        {
            last_state_tick = xTaskGetTickCount();
            publish_state(mode, door);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
