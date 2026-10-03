# almond makefile
# Toolchain definition (Gentoo cross-compiler paths)
CC       = i686-elf-gcc
LD       = i686-elf-ld
OBJCOPY  = i686-elf-objcopy
ASM      = nasm

# Flags
CFLAGS   = -ffreestanding -nostdlib -Os -Wall -Wextra -Isrc/boot -march=pentium -ffunction-sections -fdata-sections -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-ident -fshort-enums -fno-pie -fno-pic
ASMFLAGS = -f elf32

# Directories & Objects
OBJDIR   = obj/boot
C_SRCS   = src/boot/bl_vga.c src/boot/main.c src/boot/drv/keyboard.c src/boot/drv/disk.c src/boot/drv/ide.c src/boot/drv/ext2.c src/boot/shell.c
C_OBJS   = $(patsubst src/boot/%.c,$(OBJDIR)/%.o,$(C_SRCS))
ASM_OBJ  = $(OBJDIR)/stub.o
LDSCRIPT = src/boot/linker.ld

OBJDIRKERN = obj/kernel
C_SRCSKERN = src/kernel/main.c src/kernel/messaging/messaging.c src/kernel/messaging/drv/com.c src/kernel/mmu/pmm.c src/kernel/mmu/vmm.c src/kernel/mmu/kmalloc.c src/kernel/idt/panic.c src/kernel/idt/pic.c src/kernel/framebuffer/fb.c src/kernel/framebuffer/drv/vbe.c src/kernel/bootsplash.c src/kernel/helpers/terminus.c src/kernel/framebuffer/bsman.c src/kernel/panic.c src/kernel/disk/disk.c src/kernel/disk/drv/ide.c src/kernel/cmdline.c src/kernel/pci/pci.c src/kernel/mtrr.c src/kernel/cpuid_c.c src/kernel/disk/mbr.c
C_OBJSKERN = $(patsubst src/kernel/%.c,$(OBJDIRKERN)/%.o,$(C_SRCSKERN))
ASM_OBJKERN = $(OBJDIRKERN)/stub.o $(OBJDIRKERN)/idt/isr.o
LDSCRIPTKERN = src/kernel/linker.ld
ELF_OUTPUTKERN = almond.elf
OUTPUTKERN = almond.bin

ELF_OUTPUT = stage2.elf
OUTPUT     = stage2.bin

.PHONY: all clean

all: $(OBJDIR) $(OUTPUT) $(OBJDIRKERN) $(OUTPUTKERN)

# Ensure object directory exists
$(OBJDIR):
	mkdir -p $(OBJDIR)
	mkdir -p $(OBJDIR)/drv

$(OBJDIRKERN):
	mkdir -p $(OBJDIRKERN)
	mkdir -p $(OBJDIRKERN)/messaging
	mkdir -p $(OBJDIRKERN)/messaging/drv
	mkdir -p $(OBJDIRKERN)/framebuffer
	mkdir -p $(OBJDIRKERN)/framebuffer/drv
	mkdir -p $(OBJDIRKERN)/mmu
	mkdir -p $(OBJDIRKERN)/idt
	mkdir -p $(OBJDIRKERN)/helpers
	mkdir -p $(OBJDIRKERN)/disk
	mkdir -p $(OBJDIRKERN)/disk/drv
	mkdir -p $(OBJDIRKERN)/pci

# Compile C files into obj/boot/
$(OBJDIR)/%.o: src/boot/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble NASM stub into obj/boot/
$(OBJDIR)/stub.o: src/boot/stub.asm
	$(ASM) $(ASMFLAGS) $< -o $@

# Link everything into an intermediate ELF file
$(ELF_OUTPUT): $(ASM_OBJ) $(C_OBJS) $(LDSCRIPT)
	$(LD)  -T $(LDSCRIPT) -o $@ $(ASM_OBJ) $(C_OBJS) --entry=_start

# Extract flat binary and pad to exactly 32256 bytes (31.5 KiB)
$(OUTPUT): $(ELF_OUTPUT)
	$(OBJCOPY) -O binary --pad-to 32256 $(ELF_OUTPUT) $(OUTPUT)

$(OBJDIRKERN)/%.o: src/kernel/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIRKERN)/stub.o: src/kernel/stub.asm
	$(ASM) $(ASMFLAGS) $< -o $@

$(OBJDIRKERN)/idt/isr.o: src/kernel/idt/isr.asm
	$(ASM) $(ASMFLAGS) $< -o $@

$(ELF_OUTPUTKERN): $(ASM_OBJKERN) $(C_OBJSKERN) $(LDSCRIPTKERN)
	$(LD) -T $(LDSCRIPTKERN) -o $@ $(ASM_OBJKERN) $(C_OBJSKERN) --entry=_start

$(OUTPUTKERN): $(ELF_OUTPUTKERN)
	$(OBJCOPY) -O binary $(ELF_OUTPUTKERN) $(OUTPUTKERN)

clean:
	rm -rf obj $(ELF_OUTPUT) $(OUTPUT) $(ELF_OUTPUTKERN) $(OUTPUTKERN) $(OBJDIR) $(OBJDIRKERN)
