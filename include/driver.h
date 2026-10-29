#pragma once
#include <stdint.h>

typedef enum { UART_MODE_DMA = 0, UART_MODE_POLLING = 1 } uart_mode_t;
typedef struct {
    uint32_t rx_bytes, rx_dropped, uart_errors;
    uint32_t dma_ht, dma_tc, dma_errors, uart_idle_irqs;
    uint32_t uart_irq_count, dma_irq_count, uart_irq_max_cycles, dma_irq_max_cycles;
    uint32_t poll_iterations;
} uart_stats_t;

void board_init(void);
void led_set(int on);
void led_toggle(void);
void probe_set(int on);
uint32_t board_millis(void);
void timing_init(void);
uint32_t timing_cycles(void);
uint32_t cycles_to_us100(uint32_t cycles);

void uart_init(uint32_t baud);
void uart_set_mode(uart_mode_t mode);
uart_mode_t uart_get_mode(void);
int uart_try_read(void);
void uart_write_char(char c);
void uart_write_str(const char *text);
void uart_write_u32(uint32_t value);
void uart_write_hex8(uint8_t value);
void uart_stats_read(uart_stats_t *dst);
void uart_stats_reset(void);

void spi_init(void);
int spi_exchange(uint8_t out, uint8_t *in);
int spi_loopback(uint32_t *cycles, uint8_t *received);
int accel_whoami(uint8_t *id, uint32_t *cycles);
