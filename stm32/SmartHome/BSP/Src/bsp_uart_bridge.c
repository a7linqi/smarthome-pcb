/**
 * UART 桥接驱动（USART3，连接 ESP8266）
 *
 * 接收方式：中断逐字节接收 → 环形缓冲区（256 字节）→ 上层轮询读取
 * 发送方式：阻塞式发送（HAL_UART_Transmit）
 *
 * 环形缓冲区原理：
 *   head ——写入位置（中断回调中移动）
 *   tail ——读取位置（上层调用 Read 时移动）
 *   head == tail 时为空，(head+1)%SIZE == tail 时为满
 */
#include "bsp_uart_bridge.h"
#include "stm32f1xx_hal.h"

#define RX_SIZE 256U                          /* 接收环形缓冲区大小，必须是 2 的幂（方便位运算取模） */
extern UART_HandleTypeDef huart3;             /* USART3 句柄，由 CubeMX 生成 */

static uint8_t rx_byte;                       /* 中断接收的单字节暂存 */
static uint8_t rx_buf[RX_SIZE];               /* 环形接收缓冲区 */
static volatile uint16_t head, tail;          /* 环形缓冲区头尾指针（volatile：中断中修改 */

/**
 * @brief 初始化 UART 桥接：清空缓冲区，启动中断接收
 */
bsp_status_t BSP_UartBridge_Init(void)
{
    head = tail = 0;   /* 清空环形缓冲区 */
    /* 启动 USART3 中断接收，每次收 1 字节，收完触发 HAL_UART_RxCpltCallback */
    return HAL_UART_Receive_IT(&huart3, &rx_byte, 1) == HAL_OK ? BSP_OK : BSP_ERROR;
}

/**
 * @brief 查询缓冲区中待读取的字节数
 *        利用无符号减法 + 掩码自动处理回绕
 */
size_t BSP_UartBridge_Available(void)
{
    return (uint16_t)(head - tail) & (RX_SIZE - 1U);
}

/**
 * @brief 从环形缓冲区读取数据（非阻塞）
 * @param data     目标缓冲区
 * @param capacity 最多读取字节数
 * @return 实际读取的字节数
 */
size_t BSP_UartBridge_Read(uint8_t *data, size_t capacity)
{
    size_t n = 0;
    if (!data) return 0;
    /* 逐字节从 tail 位置取出，直到读完或缓冲区空 */
    while (n < capacity && tail != head) {              /* 读取端判断环形缓冲区为空，是否还有数据 */
        data[n++] = rx_buf[tail];
        tail = (tail + 1U) & (RX_SIZE - 1U);  /* 位运算取模，避免除法 */
    }
    return n;
}
/**
 * @brief 阻塞式发送数据到 ESP8266
 * @param data       待发送数据
 * @param length     数据长度
 * @param timeout_ms 超时时间（毫秒）
 * @return BSP_OK 成功 / BSP_INVALID_ARG 参数非法 / BSP_ERROR 发送失败
 */
bsp_status_t BSP_UartBridge_Write(const uint8_t *data, size_t length, uint32_t timeout_ms)
{
    if (!data || !length) return BSP_INVALID_ARG;
    return HAL_UART_Transmit(&huart3, (uint8_t *)data, (uint16_t)length, timeout_ms)
           == HAL_OK ? BSP_OK : BSP_ERROR;
}
/**
 * @brief USART 接收完成回调（由 HAL 中断触发）
 *        每收到 1 字节自动调用，将数据存入环形缓冲区
 *        缓冲区满时丢弃该字节（不阻塞中断）
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3) {
        uint16_t next = (head + 1U) & (RX_SIZE - 1U);
        if (next != tail) {          /* 缓冲区未满（再走一步就追上tail） */
            rx_buf[head] = rx_byte;  /* 存入当前字节 */
            head = next;             /* 移动写指针 */
        }
        /* 重新启动中断接收，准备收下一个字节 */
        (void)HAL_UART_Receive_IT(&huart3, &rx_byte, 1);
    }
}

/**
 * @brief USART 错误回调
 *        发生帧错误/溢出等异常后，重新启动接收，防止中断停止
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3)
        (void)HAL_UART_Receive_IT(&huart3, &rx_byte, 1);
}
