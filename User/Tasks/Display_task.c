#include "FreeRTOS.h"
#include "semphr.h"
#include "cmsis_os.h"
#include "oled.h"
#include "app.h"

void DisplayTask(void *params)
{
    (void)params;

    while(1)
    {
        // 1. 快速拷贝数据，立即释放锁
        xSemaphoreTake(dataMutex, portMAX_DELAY);
        SmartHome_Data_t d = data_t;
        xSemaphoreGive(dataMutex);

        // 2. 用局部变量慢慢刷新 OLED（不持锁）
        OLED_Clear();

        // 第1行：温度
        OLED_ShowChinese(1, 1, 0);   // 温
        OLED_ShowChinese(1, 3, 1);   // 度
        OLED_ShowNum(1, 6, d.temp, 2);
        OLED_ShowString(1, 9, "C ");

        // 第2行：湿度
        OLED_ShowChinese(2, 1, 2);   // 湿
        OLED_ShowChinese(2, 3, 1);   // 度
        OLED_ShowNum(2, 6, d.humi, 2);
        OLED_ShowString(2, 9, "% ");

        // 第3行：模式 + 门状态
        if(d.mode == 0)
            OLED_ShowString(3, 1, "M:Hand ");
        else
            OLED_ShowString(3, 1, "M:Auto ");

        if(d.door == 0)
            OLED_ShowString(3, 9, "D:Off");
        else
            OLED_ShowString(3, 9, "D:On ");

        // 第4行：光照 + LED状态
        OLED_ShowNum(4, 1, d.light, 4);

        if(d.led == 0)
            OLED_ShowString(4, 9, "L:Off");
        else
            OLED_ShowString(4, 9, "L:On ");

        osDelay(500);  // 每500ms刷新一次
    }
}
