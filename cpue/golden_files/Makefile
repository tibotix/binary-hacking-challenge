# Compiler and linker commands
NASM = nasm
NASM_FLAGS = -f elf64
LD = ld
LD_FLAGS = -no-pie -static

TCC = tcc
TCC_FLAGS = -nostdlib -static

# Directories
ASM_DIR = asm
C_DIR = c
BUILD_DIR = build

# File lists
ASM_FILES = $(wildcard $(ASM_DIR)/*.asm)
ASM_OBJS = $(patsubst $(ASM_DIR)/%.asm, $(BUILD_DIR)/%.o, $(ASM_FILES))
ASM_BINS = $(patsubst $(ASM_DIR)/%.asm, $(BUILD_DIR)/%, $(ASM_FILES))

C_FILES = $(wildcard $(C_DIR)/*.c)
C_BINS = $(patsubst $(C_DIR)/%.c, $(BUILD_DIR)/%, $(C_FILES))

# Targets
.PHONY: all asm c clean

all: asm c

asm: $(ASM_BINS)

c: $(C_BINS)

# Compile and link ASM files
$(BUILD_DIR)/%.o: $(ASM_DIR)/%.asm
	@mkdir -p $(BUILD_DIR)
	$(NASM) $(NASM_FLAGS) -o $@ $<

$(BUILD_DIR)/%: $(BUILD_DIR)/%.o
	$(LD) $(LD_FLAGS) -o $@ $<

# Compile C files
$(BUILD_DIR)/%: $(C_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(TCC) $(TCC_FLAGS) -o $@ $<

# Clean up
clean:
	rm -rf $(BUILD_DIR)
