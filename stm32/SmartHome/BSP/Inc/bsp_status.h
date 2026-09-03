#ifndef BSP_STATUS_H
#define BSP_STATUS_H

typedef enum {
    BSP_OK = 0,
    BSP_ERROR = -1,
    BSP_TIMEOUT = -2,
    BSP_BUSY = -3,
    BSP_INVALID_ARG = -4
} bsp_status_t;

#endif
