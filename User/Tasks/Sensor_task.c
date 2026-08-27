#include "FreeRTOS.h"
#include "semphr.h"
#include "dht11.h"
#include "light.h"
#include "cmsis_os.h"
#include "Sensor_task.h"

// data_t 和 dataMutex 在 freertos.c 中定义，这里通过 app.h 的 extern 引用

void SensorTask (void *params)
{
    (void)params;
    while(1)
    {
        uint8_t temp,humi;
        uint16_t light;  

        //读取温湿度
        if(DHT11_ReadData(&temp, &humi) == 0)
        {
            xSemaphoreTake(dataMutex, portMAX_DELAY);  //上锁
            data_t.temp = temp;
            data_t.humi = humi;
            xSemaphoreGive(dataMutex);
        }
        //读光敏
        light = Light_Read();
        xSemaphoreTake(dataMutex, portMAX_DELAY);
        data_t.light = light;
        xSemaphoreGive(dataMutex);

        //读led
        xSemaphoreTake(dataMutex, portMAX_DELAY);
        uint8_t led;
        if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_5) == RESET)
        led = 0;
        else led = 1;
        data_t.led = led;
        xSemaphoreGive(dataMutex);

        //读舵机状态
        xSemaphoreTake(dataMutex, portMAX_DELAY);
        uint8_t door;
        data_t.door = Door_GetStatus();
        xSemaphoreGive(dataMutex);
        
        osDelay(2000);
    }
}