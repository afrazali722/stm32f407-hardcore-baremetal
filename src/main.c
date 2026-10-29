#include "hw.h"
#include "driver.h"
#define LINE_SIZE 80u

static void nl(void) { uart_write_str("\r\n"); }
static void field(const char *name,uint32_t value) {
    uart_write_str(name);
    uart_write_u32(value);
    nl();
}
static void timing_field(const char *name,uint32_t cycles) {
    uart_write_str(name);
    uart_write_u32(cycles);
    uart_write_str(" cycles (");
    uint32_t us100=cycles_to_us100(cycles);
    uart_write_u32(us100/100u);
    uart_write_char('.');
    uart_write_char((char)('0'+(us100/10u)%10u));
    uart_write_char((char)('0'+(us100%10u)));
    uart_write_str(" us @16 MHz)");
    nl();
}
static int equals(const char *a,const char *b) {
    while(*a && *b && *a==*b) { ++a; ++b; }
    return *a==*b;
}
static void help(void) {
    uart_write_str("\r\nCOMMANDS\r\n"
        "  help          list commands\r\n"
        "  led on/off/t  onboard green LED\r\n"
        "  mode dma      circular DMA receive\r\n"
        "  mode poll     software polling receive\r\n"
        "  stats         measured interrupt counters\r\n"
        "  resetstats    reset diagnostic counters\r\n"
        "  spi           SPI1 physical loopback: PA7 -> PA6\r\n"
        "  accel         onboard accelerometer ID (remove loopback)\r\n"
        "  clock         CPU clock and SysTick uptime\r\n"
        "  about         description and limitations\r\n");
}
static void stats(void) {
    uart_stats_t s;
    uart_stats_read(&s);
    uart_write_str("\r\n=== LIVE HARDWARE COUNTERS ===\r\n");
    uart_write_str("receive mode: ");
    uart_write_str(uart_get_mode()==UART_MODE_DMA ? "DMA circular\r\n" : "CPU polling\r\n");
    field("RX bytes: ",s.rx_bytes);
    field("RX dropped (SW ring full): ",s.rx_dropped);
    field("USART errors: ",s.uart_errors);
    field("DMA half IRQ events: ",s.dma_ht);
    field("DMA full IRQ events: ",s.dma_tc);
    field("DMA error flags: ",s.dma_errors);
    field("USART IDLE events: ",s.uart_idle_irqs);
    field("USART IRQ count (includes TX): ",s.uart_irq_count);
    field("DMA IRQ count: ",s.dma_irq_count);
    field("Polling iterations: ",s.poll_iterations);
    timing_field("Max USART IRQ duration: ",s.uart_irq_max_cycles);
    timing_field("Max DMA IRQ duration: ",s.dma_irq_max_cycles);
    uart_write_str("NOTE: IRQ duration != interrupt entry latency.\r\n");
}
static void run(const char *cmd) {
    if(equals(cmd,"help") || equals(cmd,"?")) help();
    else if(equals(cmd,"led on")) { led_set(1); uart_write_str("LED ON\r\n"); }
    else if(equals(cmd,"led off")) { led_set(0); uart_write_str("LED OFF\r\n"); }
    else if(equals(cmd,"led t")) { led_toggle(); uart_write_str("LED TOGGLED\r\n"); }
    else if(equals(cmd,"mode dma")) { uart_set_mode(UART_MODE_DMA); uart_write_str("RX: DMA circular + HT/TC/IDLE\r\n"); }
    else if(equals(cmd,"mode poll")) { uart_set_mode(UART_MODE_POLLING); uart_write_str("RX: foreground polling, no DMA\r\n"); }
    else if(equals(cmd,"resetstats")) { uart_stats_reset(); uart_write_str("COUNTERS RESET\r\n"); }
    else if(equals(cmd,"stats")) stats();
    else if(equals(cmd,"clock")) {
        field("Clock HSI (Hz): ",CPU_HZ);
        field("Uptime ms: ",board_millis());
    }
    else if(equals(cmd,"spi")) {
        uint8_t r[4]={0}; uint32_t cycles=0;
        uart_write_str("SPI1 loopback PA7->PA6: ");
        int ok=spi_loopback(&cycles,r);
        uart_write_str(ok ? "PASS\r\n" : "FAIL (check jumper, wiring and mode)\r\n");
        uart_write_str("RX: ");
        for(unsigned i=0;i<4u;i++) { uart_write_hex8(r[i]); uart_write_char(' '); }
        nl();
        timing_field("DWT SPI transfer: ",cycles);
    }
    else if(equals(cmd,"accel")) {
        uint32_t cycles=0; uint8_t id=0;
        int ok=accel_whoami(&id,&cycles);
        if(!ok) uart_write_str("Accelerometer SPI TIMEOUT\r\n");
        else {
            uart_write_str("MEMS WHO_AM_I: "); uart_write_hex8(id); nl();
            if(id==0x3Bu) uart_write_str("Detected LIS302DL\r\n");
            else if(id==0x3Fu) uart_write_str("Detected LIS3DSH\r\n");
            else uart_write_str("Unexpected ID: check board revision and loopback jumper\r\n");
        }
        timing_field("DWT MEMS SPI transaction: ",cycles);
    }
    else if(equals(cmd,"about")) {
        uart_write_str("Cortex-M4 bare-metal, STM32F407 Discovery, HSI 16MHz.\r\n"
            "DMA RX: DMA1 Stream5 Channel4. TX: interrupt-driven queue.\r\n"
            "NVIC: DMA priority 5, UART priority 6, SysTick 15.\r\n"
            "Demo: single main loop, no RTOS; no verified throughput claim.\r\n");
    }
    else if(cmd[0]) uart_write_str("Unknown command; type help\r\n");
}
int main(void) {
    board_init();
    timing_init();
    spi_init();
    uart_init(115200u);
    uart_write_str("\r\nSTM32F407 / CORTEX-M4 / REAL REGISTER DRIVERS\r\n"
        "16 MHz HSI | UART2 DMA RX / TX IRQ | SPI1 | NVIC | DWT\r\n"
        "These are hardware readings, not generated sample numbers.\r\n"
        "Type help then Enter.\r\n> ");
    led_set(1);
    char line[LINE_SIZE];
    unsigned len=0;
    while(1) {
        int b=uart_try_read();
        if(b<0) continue;
        if(b=='\r' || b=='\n') {
            if(len==0) continue; /* CRLF terminals */
            uart_write_str("\r\n");
            line[len]='\0';
            run(line);
            len=0;
            uart_write_str("> ");
        } else if(b==8 || b==127) {
            if(len) { --len; uart_write_str("\b \b"); }
        } else if(b>=32 && b<=126) {
            if(len < LINE_SIZE-1u) {
                line[len++]=(char)b;
                uart_write_char((char)b);
            } else uart_write_char('\a');
        }
    }
}
