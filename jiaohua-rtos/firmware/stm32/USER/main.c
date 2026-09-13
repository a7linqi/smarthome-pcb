#include "stm32f10x.h"

#include "Key.h"
#include "OLED.h"
#include "adc.h"
#include "app_main.h"
#include "delay.h"
#include "dht11.h"
#include "led.h"

int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    LED_Init();
    BEEP = OFF;
    Water_pump = OFF;

    Key_Init();
    OLED_Init();
    OLED_Clear(0U);
    Adc_Init();
    (void)DHT11_Init();

    AppMain_Start();

    for (;;) { }
}
