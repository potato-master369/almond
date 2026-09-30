#!/bin/sh

qemu-system-i386 -cpu pentium3 --drive format=raw,file=disk.img,if=ide --no-reboot -serial stdio -m 128M -d int,cpu_reset -vga none -device VGA,vgamem_mb=2

