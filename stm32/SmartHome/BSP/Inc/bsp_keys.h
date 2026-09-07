#ifndef BSP_KEYS_H
#define BSP_KEYS_H

#include <stdint.h>

/* Call every 10 ms. Returned bits are one-shot press events. */
uint32_t BSP_Keys_Scan10ms(void);

/* Simple application interface: 0=no key, 1=KEY1, 2=KEY2, 3=KEY0, 4=WK_UP. */
uint8_t BSP_Key_Scan(void);

#endif
