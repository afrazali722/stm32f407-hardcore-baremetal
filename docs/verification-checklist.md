# Bench verification checklist (fill in after testing)

- [ ] STM32F407VGT6, board revision: _______
- [ ] Firmware Git commit SHA: _______
- [ ] CPU clock verified as HSI 16 MHz (or code/clock updated)
- [ ] ST-LINK detects board, firmware ELF flash verification PASS
- [ ] Serial RX/TX works at 115200, 8N1, 3.3V
- [ ] `led on`, `led off`, `led t` all control PD12
- [ ] `mode poll` accepts commands and records polling reads
- [ ] `mode dma` accepts commands, `stats` shows IDLE/HT/TC activity
- [ ] No UART framing/overrun errors at controlled test rate
- [ ] PA7-PA6 jumper installed for SPI loopback: 4/4 bytes match
- [ ] PA7-PA6 jumper removed: `accel` reports valid 0x3B or 0x3F (variant-dependent)
- [ ] DWT cycle readings stable and nonzero
- [ ] PD13 oscilloscope duration comparable to software cycle count / CPU clock (expected overhead differs)
- [ ] Logic analyzer capture includes PA5 (SCK), PA6, PA7, PD13 and GND
- [ ] Stress test different UART sizes and document buffer-overwrite risk

## Results to record (do NOT invent values)

| Measurement | Real measured value | Method |
|---|---|---|
| UART baud | ___ | Scope / decoder |
| SPI SCK | ___ | Logic analyzer |
| SPI loopback DWT cycles | ___ | `spi` output |
| SPI pulse width | ___ | PD13 external probe |
| DMA HT events | ___ | `stats` output |
| DMA TC events | ___ | `stats` output |
| RX dropped | ___ | `stats` output |
| RX overrun / errors | ___ | `stats` output |

## Caution

This minimal demo does not provide an RTOS, independent CPU-load measurement,
interrupt entry-latency profiling, DMA cache-coherency primitives (F407 is
non-cached on these SRAM accesses), or a lossless sustained high-rate UART
receiver. Real continuous streaming requires overrun detection, flow control,
error recovery and validated worst-case interrupt service time.
