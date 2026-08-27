#include "Control_task.h"
#include "led.h"
#include "key.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "servo.h"
#include "cmsis_os.h"
#include "Sensor_task.h"

void ControlTask(void *params)
{
    (void)params;
    uint8_t key;
    static uint8_t state = 1;
    static int8_t dir = 1;
    RGB_Off();
    while(1)
    {
        xSemaphoreTake(keySem, portMAX_DELAY);  // 阻塞等待按键中断
        osDelay(20);                            // 消抖
        key = Key_Scan();                         // 读取按键值

        if(key == 4)            //切换模式
        {
            xSemaphoreTake(dataMutex, portMAX_DELAY);
            data_t.mode = !data_t.mode;
            xSemaphoreGive(dataMutex);
        }

        if(data_t.mode == 0)    //手动模式
        {
            if(key == 1) { LED_Toggle(); data_t.led = !data_t.led;}
            if(key == 2) { Door_Toggle(); data_t.door = !data_t.door;}
        }
        else                    //自动模式
        {
            if(key == 3)
            {
                if(state >= 3){state = 3;dir = -1;}
                if(state <= 1){state = 1;dir = 1;}
                state +=dir;
                switch (state)
                {
                case 1:RGB_Red();   break;
                case 2:RGB_Green(); break;    
                case 3:RGB_Blue();  break;
                }
            }

            if(data_t.light < 2000)
            {
                LED_On();
                data_t.led = 1;
            }
            else if(data_t.light >4000)
            {
                LED_Off();
                data_t.led = 0;
            }
            if(key == 2)
            {
                Door_Toggle();
                data_t.door = !data_t.door;
            }
        }

        // 不需要延时，xSemaphoreTake 已经阻塞等待
    }
}
