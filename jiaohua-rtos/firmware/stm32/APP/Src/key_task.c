#include "key_task.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "Key.h"
#include "app_model.h"

#define KEY_SCAN_PERIOD_MS  30U
#define UI_PAGE_COUNT        2U
#define UI_ITEM_COUNT        4U

static void SendCommand(AppCommandType type, uint16_t value)
{
    AppCommand command;

    command.type = type;
    command.value = value;
    command.sequence = 0U;
    command.requires_ack = false;
    (void)xQueueSend(g_app_command_queue, &command, 0U);
}

void KeyTask(void *argument)
{
    uint8_t page = 0U;
    uint8_t selection = 0U;
    TickType_t last_wake = xTaskGetTickCount();

    (void)argument;
    AppModel_SetUi(page, selection);

    for (;;) {
        const uint8_t key = KEY_Scan(0U);
        AppSnapshot snapshot;

        if (key == KEY1_PRES) {
            page = (uint8_t)((page + 1U) % UI_PAGE_COUNT);
            AppModel_SetUi(page, selection);
        } else if ((page == 1U) && (key == KEY2_PRES)) {
            selection = (uint8_t)((selection + 1U) % UI_ITEM_COUNT);
            AppModel_SetUi(page, selection);
        } else if ((page == 1U) &&
                   ((key == KEY3_PRES) || (key == KEY4_PRES))) {
            AppModel_GetSnapshot(&snapshot);

            if (selection == 0U) {
                SendCommand((key == KEY3_PRES) ? APP_CMD_SET_AUTO
                                               : APP_CMD_SET_MANUAL,
                            0U);
            } else if (selection == 1U) {
                SendCommand((key == KEY3_PRES) ? APP_CMD_PUMP_ON
                                               : APP_CMD_PUMP_OFF,
                            0U);
            } else if (selection == 2U) {
                uint16_t value = snapshot.temperature_high;
                if ((key == KEY3_PRES) && (value < 60U)) {
                    ++value;
                } else if ((key == KEY4_PRES) && (value > 10U)) {
                    --value;
                }
                SendCommand(APP_CMD_SET_TEMPERATURE_HIGH, value);
            } else {
                uint16_t value = snapshot.soil_low;
                if ((key == KEY3_PRES) && (value < 95U)) {
                    ++value;
                } else if ((key == KEY4_PRES) && (value > 5U)) {
                    --value;
                }
                SendCommand(APP_CMD_SET_SOIL_LOW, value);
            }
        }

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(KEY_SCAN_PERIOD_MS));
    }
}
