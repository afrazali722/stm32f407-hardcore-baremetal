#pragma once
#include <stdint.h>
#include <stddef.h>

/* STM32F407VG, reference manual RM0090: intentionally no HAL dependency. */
#define CPU_HZ 16000000u /* reset/default HSI, no PLL setup */
#define REG32(a) (*(volatile uint32_t *)(uintptr_t)(a))
#define REG8(a)  (*(volatile uint8_t  *)(uintptr_t)(a))
#define BIT(n)  (1u << (n))
#define GPIOA_BASE 0x40020000u
#define GPIOB_BASE 0x40020400u
#define GPIOD_BASE 0x40020C00u
#define GPIOE_BASE 0x40021000u
#define RCC_BASE   0x40023800u
#define USART2_BASE 0x40004400u
#define SPI1_BASE  0x40013000u
#define DMA1_BASE  0x40026000u
#define DMA1_S5_BASE (DMA1_BASE + 0x88u)

#define RCC_AHB1ENR REG32(RCC_BASE + 0x30u)
#define RCC_APB1ENR REG32(RCC_BASE + 0x40u)
#define RCC_APB2ENR REG32(RCC_BASE + 0x44u)
#define GPIO_MODER(p) REG32((p) + 0x00u)
#define GPIO_OTYPER(p) REG32((p) + 0x04u)
#define GPIO_OSPEEDR(p) REG32((p) + 0x08u)
#define GPIO_PUPDR(p) REG32((p) + 0x0Cu)
#define GPIO_BSRR(p) REG32((p) + 0x18u)
#define GPIO_AFR(p,n) REG32((p) + 0x20u + 4u*((n)/8u))
#define USART_SR REG32(USART2_BASE + 0x00u)
#define USART_DR REG32(USART2_BASE + 0x04u)
#define USART_BRR REG32(USART2_BASE + 0x08u)
#define USART_CR1 REG32(USART2_BASE + 0x0Cu)
#define USART_CR3 REG32(USART2_BASE + 0x14u)
#define DMA_HISR REG32(DMA1_BASE + 0x04u)
#define DMA_HIFCR REG32(DMA1_BASE + 0x0Cu)
#define DMA_S5CR REG32(DMA1_S5_BASE + 0x00u)
#define DMA_S5NDTR REG32(DMA1_S5_BASE + 0x04u)
#define DMA_S5PAR REG32(DMA1_S5_BASE + 0x08u)
#define DMA_S5M0AR REG32(DMA1_S5_BASE + 0x0Cu)
#define DMA_S5FCR REG32(DMA1_S5_BASE + 0x14u)
#define SPI_CR1 REG32(SPI1_BASE + 0x00u)
#define SPI_CR2 REG32(SPI1_BASE + 0x04u)
#define SPI_SR REG32(SPI1_BASE + 0x08u)
#define SPI_DR8 REG8(SPI1_BASE + 0x0Cu)

#define NVIC_ISER0 REG32(0xE000E100u)
#define NVIC_ISER1 REG32(0xE000E104u)
#define NVIC_ICER0 REG32(0xE000E180u)
#define NVIC_ICER1 REG32(0xE000E184u)
#define NVIC_IPR(irq) REG8(0xE000E400u + (uint32_t)(irq))
#define SCB_SHPR3 REG32(0xE000ED20u)
#define DEMCR REG32(0xE000EDFCu)
#define DWT_CTRL REG32(0xE0001000u)
#define DWT_CYCCNT REG32(0xE0001004u)
#define STK_CTRL REG32(0xE000E010u)
#define STK_LOAD REG32(0xE000E014u)
#define STK_VAL REG32(0xE000E018u)

#define IRQ_DMA1_STREAM5 16u
#define IRQ_USART2      38u
#define USART_SR_PE BIT(0)
#define USART_SR_FE BIT(1)
#define USART_SR_NE BIT(2)
#define USART_SR_ORE BIT(3)
#define USART_SR_IDLE BIT(4)
#define USART_SR_RXNE BIT(5)
#define USART_SR_TXE BIT(7)
#define USART_CR1_RE BIT(2)
#define USART_CR1_TE BIT(3)
#define USART_CR1_IDLEIE BIT(4)
#define USART_CR1_TXEIE BIT(7)
#define USART_CR1_UE BIT(13)
#define USART_CR3_DMAR BIT(6)
#define DMA_S5_ALL_FLAGS (BIT(6)|BIT(8)|BIT(9)|BIT(10)|BIT(11))

static inline void compiler_barrier(void) { __asm__ volatile ("" ::: "memory"); }
static inline uint32_t irq_save(void) {
    uint32_t p;
    __asm__ volatile ("mrs %0, primask\ncpsid i" : "=r"(p) :: "memory");
    return p;
}
static inline void irq_restore(uint32_t p) {
    __asm__ volatile ("msr primask, %0" :: "r"(p) : "memory");
}
static inline void nvic_enable(uint32_t irq, uint8_t priority) {
    NVIC_IPR(irq) = (uint8_t)(priority << 4u); /* F407: 4 priority bits */
    if (irq < 32u) NVIC_ISER0 = BIT(irq);
    else NVIC_ISER1 = BIT(irq - 32u);
}
static inline void gpio_mode(uint32_t port, uint32_t pin, uint32_t mode) {
    GPIO_MODER(port) = (GPIO_MODER(port) & ~(3u << (pin*2u))) | ((mode & 3u) << (pin*2u));
}
static inline void gpio_af(uint32_t port, uint32_t pin, uint32_t af) {
    uint32_t shift=(pin%8u)*4u;
    GPIO_AFR(port,pin) = (GPIO_AFR(port,pin) & ~(15u << shift)) | ((af & 15u) << shift);
}
static inline void gpio_write(uint32_t port,uint32_t pin,int high) {
    GPIO_BSRR(port) = BIT(pin + (high ? 0u : 16u));
}
