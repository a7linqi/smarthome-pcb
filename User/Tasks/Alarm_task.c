#include "FreeRTOS.h"
#include "semphr.h"
#include "cmsis_os.h"
#include "buzzer.h"
#include "app.h"

void AlarmTask(void *params)
{
    (void)params;
    uint8_t temp;

    while(1)
    {
        // 快速读温度，立即释放锁
        xSemaphoreTake(dataMutex, portMAX_DELAY);
        temp = data_t.temp;
        xSemaphoreGive(dataMutex);

        // 判断是否报警
        if(temp > 35)
            Buzzer_On();
        else
            Buzzer_Off();

        osDelay(1000);  // 1秒检查一次
    }
}
