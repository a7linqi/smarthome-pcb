#ifndef UART_TASK_H
#define UART_TASK_H

#include <stdbool.h>
#include <stdint.h>

/* CONTROL_COMMAND payload 中的设备编号。ESP8266 与 STM32 必须保持一致。 */
typedef enum {
    APP_DEVICE_LIGHT = 1,
    APP_DEVICE_DOOR = 2,
    APP_DEVICE_MODE = 3
} app_device_id_t;

/** 控制命令应答状态码 */
typedef enum {
    APP_ACK_OK = 0,      /* 操作成功 */
    APP_ACK_INVALID = 1, /* 无效设备或参数 */
    APP_ACK_BUSY = 2     /* 队列满，control_task 忙 */
} app_ack_status_t;

/**
 * 远程控制命令（ESP8266 → STM32）
 * sequence: 用于 ACK 回复时匹配请求
 * device:   目标设备编号（app_device_id_t）
 * value:    控制值（灯=0/1, 门=0/1, 模式=0手动/1自动）
 */
typedef struct {
    uint8_t sequence;
    uint8_t device;
    uint8_t value;
} app_control_command_t;

/**
 * 控制命令应答（STM32 → ESP8266）
 * sequence: 沿用原命令序号，ESP8266 据此将 ACK 与请求配对
 * status:   应答状态（app_ack_status_t）
 * device/value: 回传操作对象，方便前端展示
 */
typedef struct {
    uint8_t sequence;
    uint8_t status;
    uint8_t device;
    uint8_t value;
} app_control_ack_t;

/**
 * 设备状态快照（STM32 → ESP8266）
 * mode:      工作模式（0=手动, 1=自动）
 * light_on:  灯状态（0=关, 1=开）
 * door_open: 门锁状态（0=关, 1=开）
 */
typedef struct {
    uint8_t mode;
    uint8_t light_on;
    uint8_t door_open;
} app_device_state_t;

void Uart_Task(void *params);          /* UART 通信主任务入口 */
bool Uart_IsEspOnline(void);           /* 查询 ESP8266 是否在线 */

#endif
