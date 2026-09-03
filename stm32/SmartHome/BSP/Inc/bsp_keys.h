#ifndef BSP_KEYS_H
#define BSP_KEYS_H

#include <stdint.h>

/* Call every 10 ms. Returned bits are one-shot press events. */
uint32_t BSP_Keys_Scan10ms(void);

#endif
