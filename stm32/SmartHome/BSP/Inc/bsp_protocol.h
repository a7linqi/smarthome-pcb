#ifndef BSP_PROTOCOL_H
#define BSP_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>
#include "bsp_status.h"

/*
 * STM32 ↔ ESP8266 串口通信协议 v1
 *
 * 帧格式（小端序）：
 *   偏移  长度  字段        说明
 *   0     1    SOF0        帧头固定 0xA5
 *   1     1    SOF1        帧头固定 0x5A
 *   2     1    version     协议版本号（当前 0x01）
 *   3     1    type        消息类型（见 bsp_message_type_t）
 *   4     1    sequence    帧序号，用于请求-应答配对
 *   5     1    reserved    保留字节（0x00）
 *   6-7   2    length      载荷长度（小端序，最大 96 字节）
 *   8..N  N    payload     载荷数据
 *   N+0   2    CRC16       从 version 到 payload 末尾的 CRC-16/CCITT
 *
 * 总帧长 = 8(头) + length + 2(CRC) = 10 + length
 */

#define BSP_PROTOCOL_VERSION 1U
#define BSP_PROTOCOL_MAX_PAYLOAD 96U
#define BSP_PROTOCOL_MAX_FRAME (BSP_PROTOCOL_MAX_PAYLOAD + 10U)

/** 消息类型定义 */
typedef enum {
    BSP_MSG_HEARTBEAT = 0x01,         /* 心跳包（双向） */
    BSP_MSG_SENSOR_REPORT = 0x10,     /* 传感器数据上报（STM32→ESP） */
    BSP_MSG_DEVICE_STATE = 0x11,      /* 设备状态上报（STM32→ESP） */
    BSP_MSG_CONTROL_COMMAND = 0x20,   /* 控制命令（ESP→STM32） */
    BSP_MSG_CONTROL_ACK = 0x21,       /* 控制应答（STM32→ESP） */
    BSP_MSG_ERROR = 0x7F              /* 错误通知 */
} bsp_message_type_t;

/** 解析完成的协议帧（去掉帧头和 CRC） */
typedef struct {
    uint8_t type;                                  /* 消息类型 */
    uint8_t sequence;                              /* 帧序号 */
    uint16_t length;                               /* 载荷实际长度 */
    uint8_t payload[BSP_PROTOCOL_MAX_PAYLOAD];     /* 载荷数据 */
} bsp_protocol_frame_t;

/** 协议解析器状态（逐字节喂入状态机） */
typedef struct {
    uint8_t buffer[BSP_PROTOCOL_MAX_FRAME];  /* 接收缓冲区 */
    uint16_t used;                           /* 当前已接收字节数 */
} bsp_protocol_parser_t;

uint16_t BSP_Protocol_Crc16(const uint8_t *data, size_t length);  /* CRC-16/CCITT 计算 */
/* 编码一帧：返回完整帧长度，失败返回 0 */
size_t BSP_Protocol_Encode(uint8_t type, uint8_t sequence, const void *payload,
                           uint16_t length, uint8_t *output, size_t capacity);
void BSP_Protocol_ParserInit(bsp_protocol_parser_t *parser);  /* 初始化解析器 */
/* 逐字节喂入，返回 BSP_OK 时 frame 已填好完整帧 */
bsp_status_t BSP_Protocol_Feed(bsp_protocol_parser_t *parser, uint8_t byte,
                               bsp_protocol_frame_t *frame);

#endif
