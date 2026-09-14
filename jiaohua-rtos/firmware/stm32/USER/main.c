#include "stm32f10x.h"

#include <stdbool.h>

#include "Key.h"
#include "OLED.h"
#include "adc.h"
#include "app_main.h"
#include "delay.h"
#include "dht11.h"
#include "led.h"

static bool SelectNetworkMode(void)
{
    OLED_Clear(0U);
    OLED_ShowText(0U, 0U,
                  (u8 *)"\xCA\xC7\xB7\xF1\xD0\xE8\xD2\xAA\xC1\xAA\xCD\xF8\xA3\xBF",
                  0U);
    OLED_ShowText(8U, 4U,
                  (u8 *)"1.\xCA\xC7  2.\xB7\xF1", 0U);

    for (;;) {
        const u8 key = KEY_Scan(0U);

        if (key == KEY1_PRES) {
            return true;
        }
        if (key == KEY2_PRES) {
            return false;
        }

        delay_ms(20U);
    }
}

int main(void)
{
    bool network_enabled;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    LED_Init();
    BEEP = OFF;
    Water_pump = OFF;

    Key_Init();
    OLED_Init();
    OLED_Clear(0U);
    network_enabled = SelectNetworkMode();
    OLED_Clear(0U);
    Adc_Init();
    (void)DHT11_Init();

    AppMain_Start(network_enabled);

    for (;;) { }
}
