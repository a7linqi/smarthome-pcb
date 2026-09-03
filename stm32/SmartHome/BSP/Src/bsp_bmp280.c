#include "bsp_bmp280.h"
#include "stm32f1xx_hal.h"

#define BMP_ADDR (0x76U << 1)
extern I2C_HandleTypeDef hi2c1;
typedef struct { uint16_t t1; int16_t t2,t3; uint16_t p1; int16_t p2,p3,p4,p5,p6,p7,p8,p9; int32_t fine; } calib_t;
static calib_t cal;

static bsp_status_t read_reg(uint8_t reg, uint8_t *buf, uint16_t len)
{ return HAL_I2C_Mem_Read(&hi2c1, BMP_ADDR, reg, I2C_MEMADD_SIZE_8BIT, buf, len, 100) == HAL_OK ? BSP_OK : BSP_ERROR; }
static bsp_status_t write_reg(uint8_t reg, uint8_t value)
{ return HAL_I2C_Mem_Write(&hi2c1, BMP_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &value, 1, 100) == HAL_OK ? BSP_OK : BSP_ERROR; }
static uint16_t u16le(const uint8_t *p) { return (uint16_t)p[0] | ((uint16_t)p[1] << 8); }

bsp_status_t BSP_BMP280_Init(void)
{
    uint8_t id, b[24];
    if (read_reg(0xD0, &id, 1) != BSP_OK || id != 0x58) return BSP_ERROR;
    if (read_reg(0x88, b, sizeof b) != BSP_OK) return BSP_ERROR;
    cal.t1=u16le(b); cal.t2=(int16_t)u16le(b+2); cal.t3=(int16_t)u16le(b+4); cal.p1=u16le(b+6);
    cal.p2=(int16_t)u16le(b+8); cal.p3=(int16_t)u16le(b+10); cal.p4=(int16_t)u16le(b+12); cal.p5=(int16_t)u16le(b+14);
    cal.p6=(int16_t)u16le(b+16); cal.p7=(int16_t)u16le(b+18); cal.p8=(int16_t)u16le(b+20); cal.p9=(int16_t)u16le(b+22);
    if (write_reg(0xF5, 5U << 2) != BSP_OK) return BSP_ERROR;
    return write_reg(0xF4, (5U << 5) | (4U << 2) | 3U);
}

bsp_status_t BSP_BMP280_Read(bsp_bmp280_data_t *data)
{
    uint8_t b[6]; int32_t at, ap, v1, v2, temp; int64_t p1, p2, pressure;
    if (!data) return BSP_INVALID_ARG;
    if (read_reg(0xF7, b, sizeof b) != BSP_OK) return BSP_ERROR;
    ap=((int32_t)b[0]<<12)|((int32_t)b[1]<<4)|(b[2]>>4); at=((int32_t)b[3]<<12)|((int32_t)b[4]<<4)|(b[5]>>4);
    v1=((((at>>3)-((int32_t)cal.t1<<1)))*cal.t2)>>11;
    v2=(((((at>>4)-(int32_t)cal.t1)*((at>>4)-(int32_t)cal.t1))>>12)*cal.t3)>>14;
    cal.fine=v1+v2; temp=(cal.fine*5+128)>>8;
    p1=(int64_t)cal.fine-128000; p2=p1*p1*cal.p6; p2+=((p1*cal.p5)<<17); p2+=((int64_t)cal.p4<<35);
    p1=((p1*p1*cal.p3)>>8)+((p1*cal.p2)<<12); p1=(((((int64_t)1)<<47)+p1)*cal.p1)>>33;
    if (!p1) return BSP_ERROR;
    pressure=1048576-ap; pressure=(((pressure<<31)-p2)*3125)/p1;
    p1=((int64_t)cal.p9*(pressure>>13)*(pressure>>13))>>25; p2=((int64_t)cal.p8*pressure)>>19;
    pressure=((pressure+p1+p2)>>8)+((int64_t)cal.p7<<4);
    data->temperature_centi_c=temp; data->pressure_pa=(uint32_t)(pressure>>8); return BSP_OK;
}
