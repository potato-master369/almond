# Almond

Hobby Os. Idk. Im terrible at using git.

## How 2 use

```
make
./genminimg.sh
sudo ./maketestfile.sh
```

tooling is AI slop. I dont really care cos yk no one can write tooling.

## How 2 run

Theres 2 VM scripts.

* `86test.sh` -- run in 86Box. You will need to set up your own VM and modify it.
* `vm.sh` -- QEMU KVM vm. Works without ridiculous config.

Or write `disk.img` to le HDD. You need a reasonably ancient computer to use this (Must support booting Legacy BIOS (non-CSM), have an IDE controller, have a VBE-compatible GPU (that ISNT max 256-color), support the newer PCI IO config mechanism (so late Pentium I and later) and the rest is trial and error.

## Requirements

* `i686-elf-gcc`


## How 2 read

If you want know how use OS better, read `doc/*`. There is stuff there. Some stuff. I will write more docs when i feel like it. Source code has more docs.

## How 2 contribute

Do whatever you want add a driver whatever idc just make sure it doesnt look out of place and open PR. Will look at it myself.

## Issue

* need IDE controller (has no timeout which is an easy fix but im lazy)
