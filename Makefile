ASM = nasm
SRC = src
BUILD = build

.PHONY: all FloppyImage Bootloader Kernel ToolFat Clean Always 

all: FloppyImage ToolFat

# FLOPPY IMAGE BUILD
FloppyImage: $(BUILD)/main_floppy.img
$(BUILD)/main_floppy.img: Bootloader Kernel
	dd if=/dev/zero of=$(BUILD)/main_floppy.img bs=512 count=2880
	mkfs.fat -F 12 -n "TTOS" $(BUILD)/main_floppy.img
	dd if=$(BUILD)/stage1.bin of=$(BUILD)/main_floppy.img conv=notrunc 
	mcopy -i $(BUILD)/main_floppy.img $(BUILD)/stage2.bin "::stage2.bin"
	mcopy -i $(BUILD)/main_floppy.img $(BUILD)/kernel.bin "::kernel.bin"
	mcopy -i $(BUILD)/main_floppy.img test.txt "::test.txt"
# -----

# BOOTLOADER BUILD
Bootloader: Stage1 Stage2

Stage1: $(BUILD)/stage1.bin

$(BUILD)/stage1.bin: Always
	$(MAKE) -C $(SRC)/bootloader/stage1 BUILD_DIR=$(abspath $(BUILD))

Stage2: $(BUILD)/stage2.bin

$(BUILD)/stage2.bin: Always
	$(MAKE) -C $(SRC)/bootloader/stage2 BUILD_DIR=$(abspath $(BUILD))
# -----

# KERNEL BUILD
Kernel: $(BUILD)/kernel.bin
$(BUILD)/kernel.bin: Always
	$(MAKE) -C $(SRC)/kernel BUILD_DIR=$(abspath $(BUILD))
# -----

# TOOL FAT
ToolFat: $(BUILD)/tools/fat
$(BUILD)/tools/fat: Always $(SRC)/tools/fat.c 
	mkdir -p $(BUILD)/tools
	gcc -g $(SRC)/tools/fat.c -o $(BUILD)/tools/fat
# -----

# ALWAYS
Always: 
	mkdir -p $(BUILD)
# -----

# ALWAYS
Clean: 
	$(MAKE) -C $(SRC)/kernel BUILD_DIR=$(abspath $(BUILD)) clean
	$(MAKE) -C $(SRC)/bootloader/stage1 BUILD_DIR=$(abspath $(BUILD)) clean
	$(MAKE) -C $(SRC)/bootloader/stage2 BUILD_DIR=$(abspath $(BUILD)) clean
	rm -rf $(BUILD)
# -----
