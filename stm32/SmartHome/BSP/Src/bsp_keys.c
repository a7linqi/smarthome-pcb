#include "bsp_keys.h"
#include "bsp_board.h"

#define DEBOUNCE_TICKS 3U
static uint8_t count[BSP_KEY_COUNT];
static uint32_t stable_mask;

uint32_t BSP_Keys_Scan10ms(void)
{
    uint32_t events = 0;
    for (uint32_t i = 0; i < BSP_KEY_COUNT; ++i) {
        const uint32_t bit = 1UL << i;
        const uint8_t pressed = BSP_Key_IsPressed((bsp_key_t)i) ? 1U : 0U;
        if (pressed == ((stable_mask & bit) != 0U)) {
            count[i] = 0U;
        } else if (++count[i] >= DEBOUNCE_TICKS) {
            count[i] = 0U;
            if (pressed) { stable_mask |= bit; events |= bit; }
            else stable_mask &= ~bit;
        }
    }
    return events;
}
