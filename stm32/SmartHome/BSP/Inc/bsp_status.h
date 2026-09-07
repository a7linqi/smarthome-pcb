#ifndef BSP_STATUS_H
#define BSP_STATUS_H

typedef enum {
    BSP_OK = 0,           /* 操作成功 */
    BSP_ERROR = -1,       /* 一般性错误（读写失败、校验错等） */
    BSP_TIMEOUT = -2,     /* 等待超时（如 DHT11 应答） */
    BSP_BUSY = -3,        /* 外设忙，暂时无法操作（如 I2C 总线占用） */
    BSP_INVALID_ARG = -4  /* 参数非法（如传了 NULL 指针） */
} bsp_status_t;

#endif
