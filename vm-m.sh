#!/bin/sh

qemu-system-i386 -cpu Conroe,vendor=GenuineIntel,family=6,model=15,stepping=6,model-id="Intel(R) Core(TM)2 CPU  X6800  @ 2.93GHz" -smp 2 --drive format=raw,file=disk.img,if=ide --no-reboot -serial stdio -m 128M -d cpu_reset -vga none -device VGA

