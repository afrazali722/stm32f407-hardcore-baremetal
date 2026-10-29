#include "hw.h"
#include "driver.h"
#define DMA_SIZE 256u
#define QUEUE_SIZE 1024u
/* Power-of-two ring sizes, producer/consumer indices monotonic u32. */
static volatile uint8_t dma_ring[DMA_SIZE] __attribute__((aligned(4)));
static uint8_t rx_ring[QUEUE_SIZE], tx_ring[QUEUE_SIZE];
static volatile uint32_t rx_head,rx_tail,tx_head,tx_tail;
static volatile uint32_t dma_cursor;
static volatile uart_mode_t rx_mode=UART_MODE_DMA;
static volatile uart_stats_t metrics;

static void dma_collect(void) {
    uint32_t pos=DMA_SIZE-DMA_S5NDTR;
    if (pos>=DMA_SIZE) pos=0;
    while (dma_cursor!=pos) {
        uint8_t b=dma_ring[dma_cursor];
        ++dma_cursor;
        dma_cursor &= DMA_SIZE-1u;
        ++metrics.rx_bytes;
        if ((rx_head-rx_tail)<QUEUE_SIZE) {
            rx_ring[rx_head & (QUEUE_SIZE-1u)]=b;
            compiler_barrier();
            ++rx_head;
        } else ++metrics.rx_dropped;
    }
}
static void disable_dma(void) {
    DMA_S5CR &= ~BIT(0);
    while (DMA_S5CR & BIT(0)) { }
}
static void enable_dma(void) {
    disable_dma();
    DMA_HIFCR=DMA_S5_ALL_FLAGS;
    dma_cursor=0;
    DMA_S5NDTR=DMA_SIZE;
    DMA_S5PAR=USART2_BASE+0x04u;
    DMA_S5M0AR=(uint32_t)(uintptr_t)dma_ring;
    DMA_S5FCR=0; /* Direct mode; FIFO disabled */
    /* Channel 4, PL=medium, MINC, circular, HTIE, TCIE, TEIE, DMEIE, P->M */
    DMA_S5CR=(4u<<25)|(1u<<16)|BIT(10)|BIT(8)|BIT(4)|BIT(3)|BIT(2)|BIT(1);
    DMA_S5CR |= BIT(0);
}
void uart_set_mode(uart_mode_t mode) {
    uint32_t saved=irq_save();
    /* Disable receive DMA requests and IDLE interrupts first. */
    USART_CR1 &= ~USART_CR1_IDLEIE;
    USART_CR3 &= ~USART_CR3_DMAR;
    disable_dma();
    DMA_HIFCR=DMA_S5_ALL_FLAGS;
    /* Read SR and DR to clear pending IDLE/ORE; discards partial receive. */
    volatile uint32_t sr=USART_SR, dr=USART_DR;
    (void)sr; (void)dr;
    rx_head=rx_tail=0;
    if (mode==UART_MODE_DMA) {
        enable_dma();
        rx_mode=UART_MODE_DMA;
        USART_CR3 |= USART_CR3_DMAR;
        USART_CR1 |= USART_CR1_IDLEIE;
    } else rx_mode=UART_MODE_POLLING;
    irq_restore(saved);
}
void uart_init(uint32_t baud) {
    RCC_APB1ENR |= BIT(17); /* USART2 */
    RCC_AHB1ENR |= BIT(21); /* DMA1 */
    (void)RCC_APB1ENR; (void)RCC_AHB1ENR;
    gpio_mode(GPIOA_BASE,2,2);
    gpio_mode(GPIOA_BASE,3,2);
    gpio_af(GPIOA_BASE,2,7);
    gpio_af(GPIOA_BASE,3,7);
    GPIO_OSPEEDR(GPIOA_BASE) |= (2u<<4)|(2u<<6);
    GPIO_PUPDR(GPIOA_BASE) = (GPIO_PUPDR(GPIOA_BASE) & ~((3u<<4)|(3u<<6))) | (1u<<6); /* RX pull-up */
    USART_CR1=0;
    USART_BRR=(CPU_HZ + baud/2u)/baud; /* OVER8=0, 16x oversampling */
    USART_CR3=0;
    USART_CR1=USART_CR1_UE|USART_CR1_TE|USART_CR1_RE;
    nvic_enable(IRQ_DMA1_STREAM5,5u);
    nvic_enable(IRQ_USART2,6u);
    uart_set_mode(UART_MODE_DMA);
}
uart_mode_t uart_get_mode(void) { return rx_mode; }
int uart_try_read(void) {
    if (rx_mode==UART_MODE_POLLING) {
        ++metrics.poll_iterations;
        uint32_t status=USART_SR;
        if (status & (USART_SR_ORE|USART_SR_FE|USART_SR_NE|USART_SR_PE)) {
            (void)USART_DR; ++metrics.uart_errors;
            return -1;
        }
        if (!(status & USART_SR_RXNE)) return -1;
        ++metrics.rx_bytes;
        return (int)(USART_DR & 0xFFu);
    }
    if (rx_tail==rx_head) return -1;
    compiler_barrier();
    uint8_t b=rx_ring[rx_tail & (QUEUE_SIZE-1u)];
    compiler_barrier();
    ++rx_tail;
    return b;
}
void uart_write_char(char c) {
    /* Only call in thread/main context; waits if TX queue is full. */
    while ((tx_head-tx_tail)>=QUEUE_SIZE) { }
    uint32_t saved=irq_save();
    tx_ring[tx_head & (QUEUE_SIZE-1u)]=(uint8_t)c;
    compiler_barrier();
    ++tx_head;
    USART_CR1 |= USART_CR1_TXEIE;
    irq_restore(saved);
}
void uart_write_str(const char *text) { while (*text) uart_write_char(*text++); }
void uart_write_u32(uint32_t value) {
    char d[10]; unsigned n=0;
    do { d[n++]=(char)('0'+value%10u); value/=10u; } while(value);
    while(n) uart_write_char(d[--n]);
}
void uart_write_hex8(uint8_t value) {
    const char *hex="0123456789ABCDEF";
    uart_write_str("0x");
    uart_write_char(hex[value>>4]);
    uart_write_char(hex[value&15u]);
}
void uart_stats_read(uart_stats_t *dst) {
    uint32_t saved=irq_save();
    *dst=(uart_stats_t){
        metrics.rx_bytes, metrics.rx_dropped, metrics.uart_errors,
        metrics.dma_ht,metrics.dma_tc,metrics.dma_errors,metrics.uart_idle_irqs,
        metrics.uart_irq_count,metrics.dma_irq_count,
        metrics.uart_irq_max_cycles,metrics.dma_irq_max_cycles,
        metrics.poll_iterations
    };
    irq_restore(saved);
}
void uart_stats_reset(void) {
    uint32_t saved=irq_save();
    volatile uint32_t *p=(volatile uint32_t *)&metrics;
    for (unsigned i=0;i<sizeof(metrics)/sizeof(uint32_t);++i) p[i]=0;
    irq_restore(saved);
}
void DMA1_Stream5_IRQHandler(void) {
    uint32_t start=timing_cycles();
    uint32_t flags=DMA_HISR & DMA_S5_ALL_FLAGS;
    DMA_HIFCR=flags; /* Write 1 to clear */
    if(flags & BIT(10)) ++metrics.dma_ht;
    if(flags & BIT(11)) ++metrics.dma_tc;
    if(flags & (BIT(6)|BIT(8)|BIT(9))) ++metrics.dma_errors;
    if(rx_mode==UART_MODE_DMA) dma_collect();
    ++metrics.dma_irq_count;
    uint32_t elapsed=timing_cycles()-start;
    if(elapsed>metrics.dma_irq_max_cycles) metrics.dma_irq_max_cycles=elapsed;
}
void USART2_IRQHandler(void) {
    uint32_t start=timing_cycles();
    uint32_t status=USART_SR;
    if(status & (USART_SR_IDLE|USART_SR_ORE|USART_SR_NE|USART_SR_FE|USART_SR_PE)) {
        /* RM0090: SR followed by DR read clears IDLE/error state. */
        (void)USART_DR;
        if(status & USART_SR_IDLE) {
            ++metrics.uart_idle_irqs;
            if(rx_mode==UART_MODE_DMA) dma_collect();
        }
        if(status & (USART_SR_ORE|USART_SR_NE|USART_SR_FE|USART_SR_PE)) ++metrics.uart_errors;
    }
    if((USART_CR1 & USART_CR1_TXEIE) && (USART_SR & USART_SR_TXE)) {
        if(tx_tail!=tx_head) {
            USART_DR=tx_ring[tx_tail & (QUEUE_SIZE-1u)];
            ++tx_tail;
        } else USART_CR1 &= ~USART_CR1_TXEIE;
    }
    ++metrics.uart_irq_count;
    uint32_t elapsed=timing_cycles()-start;
    if(elapsed>metrics.uart_irq_max_cycles) metrics.uart_irq_max_cycles=elapsed;
}
