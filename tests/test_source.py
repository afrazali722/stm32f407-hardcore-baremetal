from pathlib import Path
root = Path(__file__).resolve().parents[1]
hw = (root / 'include/hw.h').read_text()
spi = (root / 'src/spi.c').read_text()
uart = (root / 'src/uart.c').read_text()
assert '#define CPU_HZ 16000000u' in hw
assert 'DMA1_S5_BASE (DMA1_BASE + 0x88u)' in hw
assert '(4u<<25)' in uart
assert 'BIT(10)|BIT(8)' in uart
assert 'SPI_DR8' in spi
assert 6432 / 168 == 38.285714285714285
assert 6432 / 16 == 402.0
print('Static checks passed (these are NOT on-device tests)')
