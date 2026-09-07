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

typedef enum {
    APP_ACK_OK = 0,
    APP_ACK_INVALID = 1,
    APP_ACK_BUSY = 2
} app_ack_status_t;

typedef struct {
    uint8_t sequence;
    uint8_t device;
    uint8_t value;
} app_control_command_t;

typedef struct {
    uint8_t sequence;
    uint8_t status;
    uint8_t device;
    uint8_t value;
} app_control_ack_t;

typedef struct {
    uint8_t mode;
    uint8_t light_on;
    uint8_t door_open;
} app_device_state_t;

void Uart_Task(void *params);
bool Uart_IsEspOnline(void);

#endif
