#include "protocol.h"

#include <string.h>

namespace Protocol {

static constexpr uint8_t SOF0 = 0xA5;
static constexpr uint8_t SOF1 = 0x5A;
static constexpr size_t HEADER_SIZE = 8;

uint16_t crc16(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFF;
    while (length-- > 0) {
        crc ^= static_cast<uint16_t>(*data++) << 8;          //<uint16_t>强制类型转换、^=异或
        for (uint8_t bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x8000U)
                ? static_cast<uint16_t>((crc << 1) ^ 0x1021U)
                : static_cast<uint16_t>(crc << 1);
        }
    }
    return crc;
}

/**
 * 编码函数：将数据打包成帧格式
 *
 * 帧结构：
 * [SOF0][SOF1][VER][TYPE][SEQ][FLAGS][LEN_L][LEN_H][PAYLOAD...][CRC_L][CRC_H]
 *   0     1    2    3    4    5     6      7     8..8+L-1   8+L    8+L+1
 *
 * @param type      帧类型（心跳、传感器报告、控制命令等）
 * @param sequence  帧序号（用于匹配请求和响应）
 * @param payload   负载数据指针
 * @param length    负载数据长度
 * @param output    输出缓冲区
 * @param capacity  输出缓冲区容量
 * @return          编码后的总字节数，失败返回0
 */
size_t encode(uint8_t type, uint8_t sequence,
              const uint8_t *payload, uint16_t length,
              uint8_t *output, size_t capacity)
{
    // 计算总长度：头部(8) + 负载(length) + CRC(2)
    const size_t total = HEADER_SIZE + length + 2;

    // 参数检查：缓冲区不能为空、负载不能超长、容量要够、有负载时指针不能为空
    if (output == nullptr || length > MAX_PAYLOAD || capacity < total ||
        (length > 0 && payload == nullptr)) {
        return 0;
    }

    // 填充帧头（8字节）
    output[0] = SOF0;           // 帧头第1字节 0xA5
    output[1] = SOF1;           // 帧头第2字节 0x5A
    output[2] = VERSION;        // 版本号
    output[3] = type;           // 帧类型
    output[4] = sequence;       // 帧序号
    output[5] = 0;              // flags，当前保留
    output[6] = static_cast<uint8_t>(length);       // 负载长度低字节
    output[7] = static_cast<uint8_t>(length >> 8);  // 负载长度高字节

    // 复制负载数据到帧中
    if (length > 0) {
        memcpy(output + HEADER_SIZE, payload, length);
    }

    // 计算CRC校验码（从版本号开始，包含头部剩余部分和负载）
    const uint16_t crc = crc16(output + 2, HEADER_SIZE - 2 + length);
    output[HEADER_SIZE + length] = static_cast<uint8_t>(crc);         // CRC低字节
    output[HEADER_SIZE + length + 1] = static_cast<uint8_t>(crc >> 8); // CRC高字节

    return total;  // 返回编码后的总字节数
}

Parser::Parser() : used_(0) {}

void Parser::reset()
{
    used_ = 0;
}

bool Parser::feed(uint8_t byte, Frame &frame)
{
    // 状态 1：寻找第一个帧头字节 A5。
    if (used_ == 0 && byte != SOF0) {
        return false;
    }

    // 状态 2：确认第二个帧头字节 5A；连续 A5 时保留新帧头。
    if (used_ == 1 && byte != SOF1) {
        used_ = (byte == SOF0) ? 1 : 0;
        return false;
    }

    // 检查缓冲区是否已满，防止溢出
    if (used_ >= sizeof(buffer_)) {
        reset();    // 缓冲区满了，重置解析器
        return false;
    }

    // 把收到的字节存到缓冲区，然后位置指针+1
    buffer_[used_++] = byte;

    // 固定头没有收完，还不知道 payload 长度。
    if (used_ < HEADER_SIZE) {
        return false;
    }

    // 从头部第7、8字节读取 payload 长度（小端序）
    const uint16_t length = getU16LE(buffer_ + 6);

    // 校验版本号和 payload 长度是否合法
    if (buffer_[2] != VERSION || length > MAX_PAYLOAD) {
        reset();    // 不合法，重置解析器
        return false;
    }

    // 计算这帧的总长度：头部(8) + payload(length) + CRC(2)
    const size_t total = HEADER_SIZE + length + 2;

    // 还没收齐所有数据，继续等待
    if (used_ < total) {
        return false;
    }

    // 读取接收到的 CRC 校验码（帧尾2字节）
    const uint16_t received_crc = getU16LE(buffer_ + HEADER_SIZE + length);

    // 计算期望的 CRC 校验码（从版本号开始计算）
    const uint16_t expected_crc = crc16(buffer_ + 2, HEADER_SIZE - 2 + length);

    // CRC 校验失败，数据可能出错，丢弃这帧
    if (received_crc != expected_crc) {
        reset();
        return false;
    }

    // CRC 校验通过，解析帧的各个字段
    frame.type = buffer_[3];        // 帧类型
    frame.sequence = buffer_[4];    // 帧序号
    frame.length = length;          // payload 长度

    // 复制 payload 数据到 frame 结构体
    // memcpy 是 C 标准库函数，用于内存复制
    // 参数：(目标地址, 源地址, 复制字节数)
    // buffer_ + HEADER_SIZE 指向 payload 在缓冲区中的起始位置
    if (length > 0) {
        memcpy(frame.payload, buffer_ + HEADER_SIZE, length);
    }

    reset();    // 重置解析器，准备接收下一帧
    return true; // 帧完整且校验通过
}

/**
 * 读取16位小端序整数
 * 小端序：低字节在前，高字节在后
 * 例如：[0x18][0x00] → 0x0018 = 24
 */
uint16_t getU16LE(const uint8_t *src)
{
    return static_cast<uint16_t>(src[0]) |           // 低字节
           (static_cast<uint16_t>(src[1]) << 8);     // 高字节左移8位
}

/**
 * 读取32位小端序整数
 * 例如：[0x01][0x02][0x03][0x04] → 0x04030201
 */
uint32_t getU32LE(const uint8_t *src)
{
    return static_cast<uint32_t>(src[0]) |           // 字节0（最低位）
           (static_cast<uint32_t>(src[1]) << 8) |    // 字节1
           (static_cast<uint32_t>(src[2]) << 16) |   // 字节2
           (static_cast<uint32_t>(src[3]) << 24);    // 字节3（最高位）
}

/**
 * 写入32位小端序整数
 * 把32位整数拆成4字节，低字节在前
 * 例如：0x04030201 → [0x01][0x02][0x03][0x04]
 */
void putU32LE(uint8_t *dst, uint32_t value)
{
    dst[0] = static_cast<uint8_t>(value);            // 低8位
    dst[1] = static_cast<uint8_t>(value >> 8);       // 次低8位
    dst[2] = static_cast<uint8_t>(value >> 16);      // 次高8位
    dst[3] = static_cast<uint8_t>(value >> 24);      // 高8位
}

} // namespace Protocol
