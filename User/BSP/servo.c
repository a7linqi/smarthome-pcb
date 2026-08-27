#include "servo.h"
#include "tim.h"

/* 外部声明 TIM3 句柄 */
extern TIM_HandleTypeDef htim3;

/* 门的状态 */
static uint8_t door_status = 0;  /* 0=关闭，1=打开 */

/**
  * @brief  舵机初始化
  * @param  无
  * @retval 无
  * @note   启动 TIM3 的 PWM 输出（通道1，PA6）
  *         初始状态：门关闭（0度）
  */
void Servo_Init(void)
{
    /* 启动 PWM 输出 */
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);

    /* 初始状态：门关闭 */
    Door_Close();
}

/**
  * @brief  开门
  * @param  无
  * @retval 无
  * @note   舵机转到 90 度，模拟开门
  */
void Door_Open(void)
{
    uint16_t pulse = 500 + 90 * 2000 / 180;  /* 90度对应脉宽 */
    __HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_1, pulse);
    door_status = 1;  /* 更新状态为打开 */
}

/**
  * @brief  关门
  * @param  无
  * @retval 无
  * @note   舵机转到 0 度，模拟关门
  */
void Door_Close(void)
{
    uint16_t pulse = 500 + 0 * 2000 / 180;  /* 0度对应脉宽 */
    __HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_1, pulse);
    door_status = 0;  /* 更新状态为关闭 */
}

/**
  * @brief  切换门状态
  * @param  无
  * @retval 无
  * @note   如果门是开的就关，如果门是关的就开
  */
void Door_Toggle(void)
{
    if (door_status == 0)
    {
        Door_Open();   /* 当前是关的，打开 */
    }
    else
    {
        Door_Close();  /* 当前是开的，关闭 */
    }
}

/**
  * @brief  获取门的状态
  * @param  无
  * @retval 门的状态：0=关闭，1=打开
  */
uint8_t Door_GetStatus(void)
{
    return door_status;
}
