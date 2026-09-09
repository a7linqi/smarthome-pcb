#include "bsp_protocol.h"
#include <string.h>

/* ---- 帧同步字节（帧头标识） ---- */
#define SOF0 0xA5U          /* Start-of-Frame 第一字节 */
#define SOF1 0x5AU          /* Start-of-Frame 第二字节 */
#define HEADER_SIZE 8U      /* 固定帧头长度：SOF(2)+VER(1)+TYPE(1)+SEQ(1)+RES(1)+LEN(2) */

/**
 * CRC-16/CCITT 计算（多项式 0x1021，初始值 0xFFFF）
 * 计算范围：version 字段到 payload 末尾（不含 SOF 和 CRC 本身）
 */
uint16_t BSP_Protocol_Crc16(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFFU;
    while (length--) {
        crc ^= (uint16_t)*data++ << 8;
        for (uint8_t i = 0; i < 8; ++i)
            crc = (crc & 0x8000U) ? (uint16_t)((crc << 1) ^ 0x1021U) : (uint16_t)(crc << 1);
    }
    return crc;
}

/**
 * 编码一帧协议数据
 * 帧布局：[SOF0][SOF1][VER][TYPE][SEQ][0x00][LEN_L][LEN_H][payload...][CRC_L][CRC_H]
 * 返回完整帧字节数，失败返回 0
 */
size_t BSP_Protocol_Encode(uint8_t type, uint8_t sequence, const void *payload,
                           uint16_t length, uint8_t *out, size_t capacity)
{
    const size_t total = HEADER_SIZE + length + 2U;  /* 8字节头 + 载荷 + 2字节CRC */
    if (!out || length > BSP_PROTOCOL_MAX_PAYLOAD || capacity < total || (length && !payload))
        return 0;

    /* 填充帧头 */
    out[0] = SOF0;          /* 0xA5 - 帧同步字节1 */
    out[1] = SOF1;          /* 0x5A - 帧同步字节2 */
    out[2] = BSP_PROTOCOL_VERSION;  /* 协议版本 */
    out[3] = type;          /* 消息类型 */
    out[4] = sequence;      /* 帧序号 */
    out[5] = 0;             /* 保留字节 */
    out[6] = (uint8_t)(length & 0xFFU);  /* 载荷长度低字节 */
    out[7] = (uint8_t)(length >> 8);      /* 载荷长度高字节 */

    /* 拷贝载荷 */
    if (length) memcpy(out + HEADER_SIZE, payload, length);

    /* 计算并追加 CRC（范围：version 到 payload 末尾） */
    uint16_t crc = BSP_Protocol_Crc16(out + 2, HEADER_SIZE - 2U + length);
    out[HEADER_SIZE + length] = (uint8_t)(crc & 0xFFU);
    out[HEADER_SIZE + length + 1U] = (uint8_t)(crc >> 8);
    return total;
}

/** 初始化协议解析器，将接收计数清零 */
void BSP_Protocol_ParserInit(bsp_protocol_parser_t *p) { if (p) p->used = 0; }

/**
 * 逐字节喂入协议解析器（状态机）
 * 返回值：
 *   BSP_OK          - 成功解析出一帧，frame 已填充
 *   BSP_BUSY        - 还在接收中，等待更多字节
 *   BSP_ERROR       - 校验失败或格式错误，已重置解析器
 *   BSP_INVALID_ARG - 参数非法
 */
bsp_status_t BSP_Protocol_Feed(bsp_protocol_parser_t *p, uint8_t byte, bsp_protocol_frame_t *f)
{
    if (!p || !f) return BSP_INVALID_ARG;

    /* 状态 0：等待第一个帧头字节 0xA5 */
    if (p->used == 0U && byte != SOF0) return BSP_BUSY;

    /* 状态 1：等待第二个帧头字节 0x5A，若收到 0xA5 则保持在状态 1 */
    if (p->used == 1U && byte != SOF1) { p->used = (byte == SOF0); return BSP_BUSY; }

    /* 缓冲区溢出保护 */
    if (p->used >= sizeof p->buffer) { p->used = 0; return BSP_ERROR; }

    p->buffer[p->used++] = byte;

    /* 头部未接收完毕，继续等待 */
    if (p->used < HEADER_SIZE) return BSP_BUSY;

    /* 解析载荷长度（小端序，偏移 6-7） */
    uint16_t len = (uint16_t)p->buffer[6] | ((uint16_t)p->buffer[7] << 8);

    /* 校验版本号和载荷长度合法性 */
    if (p->buffer[2] != BSP_PROTOCOL_VERSION || len > BSP_PROTOCOL_MAX_PAYLOAD) {
        p->used = 0; return BSP_ERROR;
    }

    /* 载荷 + CRC 还没收完，继续等待 */
    if (p->used < HEADER_SIZE + len + 2U) return BSP_BUSY;

    /* CRC 校验（从 version 到 payload 末尾） */
    uint16_t got = (uint16_t)p->buffer[HEADER_SIZE + len] |
                   ((uint16_t)p->buffer[HEADER_SIZE + len + 1U] << 8);
    uint16_t expected = BSP_Protocol_Crc16(p->buffer + 2, HEADER_SIZE - 2U + len);
    if (got != expected) { p->used = 0; return BSP_ERROR; }

    /* 解析成功，填充输出结构 */
    f->type = p->buffer[3];
    f->sequence = p->buffer[4];
    f->length = len;
    if (len) memcpy(f->payload, p->buffer + HEADER_SIZE, len);
    p->used = 0;  /* 重置，准备接收下一帧 */
    return BSP_OK;
}
