PROJECT = stm32f407_baremetal
BUILD = build
CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size
CPU = -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
CFLAGS = $(CPU) -std=gnu11 -Os -g3 -Wall -Wextra -Wno-override-init \
         -ffreestanding -fno-builtin -ffunction-sections -fdata-sections -Iinclude
LDFLAGS = $(CPU) -nostdlib -Wl,--gc-sections,-Map=$(BUILD)/$(PROJECT).map \
          -Tlinker/stm32f407vg.ld
SOURCES = $(wildcard src/*.c)
OBJECTS = $(patsubst src/%.c,$(BUILD)/%.o,$(SOURCES))
.PHONY: all clean flash
all: $(BUILD)/$(PROJECT).bin
$(BUILD):
	mkdir -p $@
$(BUILD)/%.o: src/%.c include/hw.h include/driver.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/$(PROJECT).elf: $(OBJECTS) linker/stm32f407vg.ld
	$(CC) $(OBJECTS) $(LDFLAGS) -lgcc -o $@
	$(SIZE) $@
$(BUILD)/$(PROJECT).bin: $(BUILD)/$(PROJECT).elf
	$(OBJCOPY) -O binary $< $@
flash: $(BUILD)/$(PROJECT).elf
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program $(BUILD)/$(PROJECT).elf verify reset exit"
clean:
	rm -rf $(BUILD)
