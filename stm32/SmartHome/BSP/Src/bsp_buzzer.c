#include "bsp_buzzer.h"
#include "bsp_board.h"

static bsp_buzzer_mode_t current_mode;
static uint32_t period, deadline;
static uint8_t output_on;

/* 设置蜂鸣器模式：常开/关闭/闪烁，half_period_ms 为闪烁半周期（传0默认250ms） */
void BSP_Buzzer_SetMode(bsp_buzzer_mode_t mode, uint32_t half_period_ms)
{
    current_mode = mode;
    period = half_period_ms ? half_period_ms : 250U;   /* 半周期，0则用默认250ms */
    deadline = 0U;
    output_on = mode == BSP_BUZZER_ON;                  /* 常开模式直接开，闪烁模式从开开始 */
    BSP_Buzzer_Set(output_on != 0U);
}

/* 定时器调用，处理蜂鸣器闪烁逻辑，now_ms 为当前系统毫秒 */
void BSP_Buzzer_Process(uint32_t now_ms)
{
    if (current_mode != BSP_BUZZER_BLINK) return;      /* 非闪烁模式直接返回 */
    /* 到达翻转时刻：首次(deadline=0) 或 当前时间 >= 下次截止时间 */
    if (deadline == 0U || (int32_t)(now_ms - deadline) >= 0) {
        output_on ^= 1U;                                /* 翻转开关状态 */
        BSP_Buzzer_Set(output_on != 0U);
        deadline = now_ms + period;                      /* 设置下一次翻转时间 */
    }
}
