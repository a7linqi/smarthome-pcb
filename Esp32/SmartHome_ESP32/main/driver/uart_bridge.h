/**
 * @file uart_bridge.h
 * @brief ESP32-STM32 UART 通信桥接
 * @version 1.0
 *
 * 硬件接线：
 *   ESP32 GPIO17 (TX2) → STM32 PA10 (RX)
 *   ESP32 GPIO16 (RX2) → STM32 PA9  (TX)
 *   GND 共地
 *
 * 通信协议：
 *   ESP32 → STM32: "LED:ON\n" / "LED:OFF\n" / "DOOR:ON\n" / "DOOR:OFF\n"
 *   STM32 → ESP32: "T:25.6,L:2048,LED:ON,FAN:OFF,DOOR:OFF\n"
 */

#ifndef __UART_BRIDGE_H
#define __UART_BRIDGE_H

#include <stdint.h>

/**
 * @brief 初始化 UART（UART2, 115200, GPIO17-TX, GPIO16-RX）
 */
void uart_bridge_init(void);

/**
 * @brief 发送指令给 STM32
 * @param cmd 指令字符串，如 "LED:ON"
 * @return 0 成功，-1 失败
 */
int uart_bridge_send_cmd(const char *cmd);

/**
 * @brief 从 STM32 接收数据
 * @param buf 接收缓冲区
 * @param buf_size 缓冲区大小
 * @return 接收到的字节数，0 表示无数据，-1 表示错误
 */
int uart_bridge_receive(char *buf, int buf_size);

#endif /* __UART_BRIDGE_H */
