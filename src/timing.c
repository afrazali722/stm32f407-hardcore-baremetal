#include "hw.h"
#include "driver.h"
void timing_init(void) {
    DEMCR |= BIT(24); /* TRCENA trace + DWT access */
    DWT_CYCCNT=0;
    DWT_CTRL |= 1u;
}
uint32_t timing_cycles(void) { return DWT_CYCCNT; }
/* At 16MHz, 16 CPU cycles = 1us; result is hundredths of 1us.
 * Intended for short intervals; 32-bit output can overflow for huge intervals. */
uint32_t cycles_to_us100(uint32_t cycles) {
    return (cycles / 16u) * 100u + ((cycles % 16u) * 25u) / 4u;
}
