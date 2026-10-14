# firmware.elf, firmware.hex, firmware.bin in build/
TARGET      := firmware
BUILD_DIR   := build

# C compiler, objcopy and size commands
CC          := arm-none-eabi-gcc
OBJCOPY     := arm-none-eabi-objcopy
SIZE        := arm-none-eabi-size

# Programmer
PROGRAMMER       := openocd
PROGRAMMER_FLAGS := -f board/st_nucleo_c0.cfg

# Linker script and startup files
LINKER_SCRIPT := STM32C031C6TX_FLASH.ld
STARTUP     := Core/Startup/startup_stm32c031c6tx.s

# e.g. main.c, adc.c, lcd.c, etc.
C_SOURCES  := $(wildcard Core/Src/*.c)

# Core/Startup/startup_stm32c031c6tx.s
ASM_SOURCES := $(STARTUP)

# Directory where .h header files are stored
INCLUDES := -ICore/Inc

# Architecture flags
CPU         := cortex-m0plus
ARCH_FLAGS := -mcpu=$(CPU) -mthumb

CPPFLAGS := \
    -DSTM32C031xx \
    $(INCLUDES)

CFLAGS := \
    $(ARCH_FLAGS) \
    -std=c11 \
    -Wall \
    -Wextra \
    -Wshadow \
    -Wundef \
    -ffunction-sections \
    -fdata-sections \
    -MMD \
    -MP \
    -Og \
    -g3

ASFLAGS := \
    $(ARCH_FLAGS) \
    -g3

LDFLAGS := \
    $(ARCH_FLAGS) \
    -T$(LINKER_SCRIPT) \
    -Wl,--gc-sections \
    -Wl,-Map=$(BUILD_DIR)/$(TARGET).map \
    -Wl,--print-memory-usage \
    --specs=nano.specs \
    --specs=nosys.specs

LDLIBS := -lc -lm

# Convert list of .c file names to .o file names
C_OBJECTS := $(patsubst Core/Src/%.c,$(BUILD_DIR)/%.o,$(C_SOURCES))

# Convert list of .s file names to .o file names
ASM_OBJECTS := $(patsubst Core/Startup/%.s,$(BUILD_DIR)/%.o,$(ASM_SOURCES))

OBJECTS := $(C_OBJECTS) $(ASM_OBJECTS)
DEPS := $(C_OBJECTS:.o=.d)

# build/firmware.elf, build/firmware.hex, build/firmware.bin
ELF := $(BUILD_DIR)/$(TARGET).elf
HEX := $(BUILD_DIR)/$(TARGET).hex
BIN := $(BUILD_DIR)/$(TARGET).bin

.PHONY: all clean flash size

all: $(ELF) $(HEX) $(BIN) size

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o: Core/Src/%.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: Core/Startup/%.s | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c $< -o $@

$(ELF): $(OBJECTS) $(LINKER_SCRIPT)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o $@

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex $< $@

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

size: $(ELF)
	$(SIZE) $<

clean:
	rm -rf $(BUILD_DIR)

# Include automatically generated header dependencies.
-include $(DEPS)

flash: $(ELF)
	$(PROGRAMMER) $(PROGRAMMER_FLAGS) -c "program $(ELF) verify reset exit"
