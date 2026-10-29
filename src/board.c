#include "hw.h"
#include "driver.h"
static volatile uint32_t millis_counter;

void board_init(void) {
    RCC_AHB1ENR |= BIT(0)|BIT(1)|BIT(3)|BIT(4); /* GPIO A, B, D, E */
    (void)RCC_AHB1ENR; /* allow peripheral clock to become active */
    gpio_mode(GPIOD_BASE,12,1); /* LD4 green */
    gpio_mode(GPIOD_BASE,13,1); /* LD3 orange = timing probe */
    GPIO_OSPEEDR(GPIOD_BASE) |= (3u<<24)|(3u<<26);
    gpio_write(GPIOD_BASE,12,0);
    gpio_write(GPIOD_BASE,13,0);
    gpio_mode(GPIOE_BASE,3,1); /* onboard MEMS CS: idle high */
    gpio_write(GPIOE_BASE,3,1);
    /* 1 ms SysTick at the reset 16MHz HSI frequency. */
    SCB_SHPR3 = (SCB_SHPR3 & ~(0xFFu<<24)) | (15u<<28);
    STK_LOAD = CPU_HZ/1000u-1u;
    STK_VAL=0;
    STK_CTRL=7u;
}
void SysTick_Handler(void) { ++millis_counter; }
uint32_t board_millis(void) { return millis_counter; }
void led_set(int on) { gpio_write(GPIOD_BASE,12,on); }
void led_toggle(void) { REG32(GPIOD_BASE + 0x14u) ^= BIT(12); }
void probe_set(int on) { gpio_write(GPIOD_BASE,13,on); }
