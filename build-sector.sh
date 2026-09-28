#!/bin/sh
# script to build boot sector (.bin)
nasm src/boot/sector.asm -f bin -o boot.bin
