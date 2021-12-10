BASENAME  = gauntletlegends

ROOT_DIR  = $(PWD)
BUILD_DIR = build
ASM_DIR   = asm
ASM_DIRS  = $(ASM_DIR) src/asm $(ASM_DIR)/data $(ASM_DIR)/libultra
BIN_DIR   = assets
BIN_DIRS  = $(BIN_DIR)
SRC_DIR   = src
SRC_DIRS  = $(SRC_DIR)
TOOLS_DIR := tools

S_FILES   = $(foreach dir,$(ASM_DIRS),$(wildcard $(dir)/*.s))
C_FILES   = $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.c))
BIN_FILES = $(foreach dir,$(BIN_DIRS),$(wildcard $(dir)/*.bin))
O_FILES := $(foreach file,$(S_FILES),$(BUILD_DIR)/$(file).o) \
           $(foreach file,$(C_FILES),$(BUILD_DIR)/$(file).o) \
           $(foreach file,$(BIN_FILES),$(BUILD_DIR)/$(file).o)


GLOBAL_ASM_C_FILES != grep -rl 'GLOBAL_ASM(' $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/*/*.c)
GLOBAL_ASM_O_FILES = $(foreach file,$(GLOBAL_ASM_C_FILES),$(BUILD_DIR)/$(file:.c=.c.o))

TARGET = $(BUILD_DIR)/$(BASENAME)
LD_SCRIPT = $(BASENAME).ld

CROSS = mips-linux-gnu-
CC = $(TOOLS_DIR)/mips-gcc/kmc/gcc
OLD_CC = $(TOOLS_DIR)/mips-gcc/kmc/gcc
AS = $(CROSS)as
OLD_AS = $(TOOLS_DIR)/mips-gcc/kmc/as
CPP = cpp
LD = $(CROSS)ld
OBJDUMP = $(CROSS)objdump
OBJCOPY = $(CROSS)objcopy
STRIP = $(CROSS)strip
PYTHON = python3
N64CRC = $(TOOLS_DIR)/n64crc

# Flags
OPT_FLAGS := -O2
INCLUDE_CC_FLAGS := -I. -Iinclude -Ilibreultra/include/2.0I
AS_FLAGS := -EB -mtune=vr4300 -march=vr4300 -mips3 -O1 -I include
OLD_AS_FLAGS := -EB -mips3 -I include
D_FLAGS := -D_LANGUAGE_C -DF3DEX_GBI_2 -D__GNUC__=2

# Additional defines
ifeq ($(NON_MATCHING),1)
D_FLAGS += -DNON_MATCHING
endif

CC_FLAGS := -c -G0 -mips3 -mgp32 -mfp32 $(OPT_FLAGS) $(D_FLAGS) $(INCLUDE_CC_FLAGS) -nostdinc
LD_FLAGS := -T $(LD_SCRIPT) -Map $(TARGET).map -T undefined_syms_auto.txt -T undefined_funcs_auto.txt -T undefined_funcs.txt -T undefined_syms.txt --no-check-sections
OBJCOPY_FLAGS = -O binary

POSTPROCESS = $(PYTHON) $(TOOLS_DIR)/postprocess_asm.py
$(GLOBAL_ASM_O_FILES): CC = $(PYTHON) $(TOOLS_DIR)/asm-processor/build.py $(OLD_CC) -- $(AS) $(AS_FLAGS) --

### Optimisation Overrides

### Targets

default: all

all: dirs $(TARGET).z64 check

check: $(TARGET).z64 $(N64CRC)
	$(N64CRC) $<
	@shasum $<
	@shasum -c checksum.sha1

dirs:
	$(foreach dir,$(SRC_DIRS) $(ASM_DIRS) $(BIN_DIRS),$(shell mkdir -p $(BUILD_DIR)/$(dir)))

nuke:
	rm -rf asm
	rm -rf assets
	rm -rf build
	rm -f *auto.txt

clean:
	rm -rf build

setup:
	$(PYTHON) tools/splat/split.py $(BASENAME).yaml

context:
	rm -f ctx.c ctx_includes.c
	find include/ src/ -type f -name "*.h" | sed -e 's/.*/#include "\0"/' > ctx_includes.c
	$(PYTHON) tools/m2ctx.py ctx_includes.c
	sed -i 's/sizeof(long)/4/g' ctx.c

compare:
	$(PYTHON) first_diff.py

### Recipes

$(TARGET).elf: $(O_FILES)
	@$(LD) $(LD_FLAGS) -o $@

$(BUILD_DIR)/%.c.o: %.c
	$(CC) $(CC_FLAGS) -o $@ $<
	$(STRIP) $@ -N $<

$(BUILD_DIR)/%.s.o: %.s
	$(AS) $(AS_FLAGS) -o $@ $<

$(BUILD_DIR)/%.bin.o: %.bin
	$(LD) -r -b binary -o $@ $<

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) $(OBJCOPY_FLAGS) $< $@

$(TARGET).z64: $(TARGET).bin
	@mv $< $@

$(N64CRC): tools/n64crc.c
	make -C tools

### Settings
.SECONDARY:
.PHONY: all clean default
SHELL = /bin/bash -e -o pipefail
