#include "bsp_protocol.h"
#include <string.h>

#define SOF0 0xA5U
#define SOF1 0x5AU
#define HEADER_SIZE 8U

uint16_t BSP_Protocol_Crc16(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFFU;
    while (length--) {
        crc ^= (uint16_t)*data++ << 8;
        for (uint8_t i = 0; i < 8; ++i) crc = (crc & 0x8000U) ? (uint16_t)((crc << 1) ^ 0x1021U) : (uint16_t)(crc << 1);
    }
    return crc;
}

size_t BSP_Protocol_Encode(uint8_t type, uint8_t sequence, const void *payload,
                           uint16_t length, uint8_t *out, size_t capacity)
{
    const size_t total = HEADER_SIZE + length + 2U;
    if (!out || length > BSP_PROTOCOL_MAX_PAYLOAD || capacity < total || (length && !payload)) return 0;
    out[0]=SOF0; out[1]=SOF1; out[2]=BSP_PROTOCOL_VERSION; out[3]=type; out[4]=sequence;
    out[5]=0; out[6]=(uint8_t)(length & 0xFFU); out[7]=(uint8_t)(length >> 8);
    if (length) memcpy(out+HEADER_SIZE,payload,length);
    uint16_t crc=BSP_Protocol_Crc16(out+2,HEADER_SIZE-2U+length);
    out[HEADER_SIZE+length]=(uint8_t)(crc&0xFFU); out[HEADER_SIZE+length+1U]=(uint8_t)(crc>>8);
    return total;
}

void BSP_Protocol_ParserInit(bsp_protocol_parser_t *p) { if (p) p->used=0; }
bsp_status_t BSP_Protocol_Feed(bsp_protocol_parser_t *p, uint8_t byte, bsp_protocol_frame_t *f)
{
    if (!p || !f) return BSP_INVALID_ARG;
    if (p->used==0U && byte!=SOF0) return BSP_BUSY;
    if (p->used==1U && byte!=SOF1) { p->used=(byte==SOF0); return BSP_BUSY; }
    if (p->used>=sizeof p->buffer) { p->used=0; return BSP_ERROR; }
    p->buffer[p->used++]=byte;
    if (p->used<HEADER_SIZE) return BSP_BUSY;
    uint16_t len=(uint16_t)p->buffer[6]|((uint16_t)p->buffer[7]<<8);
    if (p->buffer[2]!=BSP_PROTOCOL_VERSION || len>BSP_PROTOCOL_MAX_PAYLOAD) { p->used=0; return BSP_ERROR; }
    if (p->used<HEADER_SIZE+len+2U) return BSP_BUSY;
    uint16_t got=(uint16_t)p->buffer[HEADER_SIZE+len]|((uint16_t)p->buffer[HEADER_SIZE+len+1U]<<8);
    uint16_t expected=BSP_Protocol_Crc16(p->buffer+2,HEADER_SIZE-2U+len);
    if (got!=expected) { p->used=0; return BSP_ERROR; }
    f->type=p->buffer[3]; f->sequence=p->buffer[4]; f->length=len;
    if (len) memcpy(f->payload,p->buffer+HEADER_SIZE,len);
    p->used=0; return BSP_OK;
}
