#include <stdint.h>
extern uint32_t _estack, _sidata, _sdata, _edata, _sbss, _ebss;
extern int main(void);
extern void SysTick_Handler(void), USART2_IRQHandler(void), DMA1_Stream5_IRQHandler(void);
void Reset_Handler(void);
void Default_Handler(void);
void HardFault_Handler(void);

/* Cortex-M vector 0=initial SP, 1=reset, 16..=external IRQs. */
typedef void (*handler_t)(void);
__attribute__((used,section(".isr_vector")))
const handler_t vectors[16 + 82] = {
    [0] = (handler_t)&_estack,
    [1] = Reset_Handler,
    [2 ... 97] = Default_Handler,
    [3] = HardFault_Handler,
    [15] = SysTick_Handler,
    [16 + 16] = DMA1_Stream5_IRQHandler,
    [16 + 38] = USART2_IRQHandler,
};

void Reset_Handler(void) {
    uint32_t *to=&_sdata, *from=&_sidata;
    while(to<&_edata) *to++=*from++;
    for(to=&_sbss;to<&_ebss;) *to++=0u;
    (void)main();
    for(;;) { }
}
void Default_Handler(void) { for(;;) { __asm__ volatile("wfi"); } }
void HardFault_Handler(void) { for(;;) { __asm__ volatile("wfi"); } }
