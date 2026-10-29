#include "hw.h"
#include "driver.h"
void spi_init(void) {
    RCC_APB2ENR |= BIT(12); /* SPI1 clock */
    (void)RCC_APB2ENR;
    /* SPI1 pins; the accelerometer shares SPI1 on Discovery. */
    gpio_mode(GPIOA_BASE,5,2);
    gpio_mode(GPIOA_BASE,6,2);
    gpio_mode(GPIOA_BASE,7,2);
    gpio_af(GPIOA_BASE,5,5);
    gpio_af(GPIOA_BASE,6,5);
    gpio_af(GPIOA_BASE,7,5);
    GPIO_OSPEEDR(GPIOA_BASE) |= (3u<<10)|(3u<<12)|(3u<<14);
    /* SPI master, Mode 0, 8-bit, fPCLK/8=2MHz, software NSS. */
    SPI_CR1=0;
    SPI_CR2=0;
    SPI_CR1 = BIT(2) | (2u<<3) | BIT(8) | BIT(9); /* MSTR BR=010 SSI SSM */
    SPI_CR1 |= BIT(6); /* SPE */
}
static int wait_sr(uint32_t mask, int set) {
    uint32_t start=timing_cycles();
    const uint32_t limit=CPU_HZ/100u; /* ~10ms timeout */
    while ((((SPI_SR & mask)!=0u) ? 1:0) != set) {
        if ((uint32_t)(timing_cycles()-start)>limit) return 0;
    }
    return 1;
}
int spi_exchange(uint8_t out,uint8_t *in) {
    if (!wait_sr(BIT(1),1)) return 0; /* TXE */
    SPI_DR8=out;
    if (!wait_sr(BIT(0),1)) return 0; /* RXNE */
    uint8_t r=SPI_DR8;
    if (!wait_sr(BIT(7),0)) return 0; /* BSY */
    if (in) *in=r;
    return 1;
}
int spi_loopback(uint32_t *cycles,uint8_t *received) {
    /* Physically connect PA7 MOSI to PA6 MISO. Onboard sensor CS stays high. */
    static const uint8_t out[4]={0x9Fu,0xEFu,0x40u,0x19u};
    uint32_t start=timing_cycles();
    probe_set(1);
    int valid=1;
    for (unsigned i=0;i<4;i++) {
        if (!spi_exchange(out[i],&received[i])) { valid=0; break; }
        if (out[i]!=received[i]) valid=0;
    }
    probe_set(0);
    *cycles=timing_cycles()-start;
    return valid;
}
int accel_whoami(uint8_t *id,uint32_t *cycles) {
    /* REMOVE PA7-PA6 loopback jumper before using onboard accelerometer. */
    uint8_t ignored=0;
    uint32_t start=timing_cycles();
    probe_set(1);
    gpio_write(GPIOE_BASE,3,0); /* MEMS CS assert */
    int ok=spi_exchange(0x8Fu,&ignored); /* read WHO_AM_I at 0x0F */
    if (ok) ok=spi_exchange(0x00u,id);
    gpio_write(GPIOE_BASE,3,1);
    probe_set(0);
    *cycles=timing_cycles()-start;
    return ok;
}
