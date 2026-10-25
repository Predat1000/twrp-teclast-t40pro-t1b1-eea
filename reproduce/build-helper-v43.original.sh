#!/bin/sh
set -eu
C=/home/twrp/twrp-11-clean/prebuilts/clang/host/linux-x86/clang-r383902b/bin
"$C/clang.real" --target=aarch64-linux-gnu -nostdlib -static -ffreestanding -fno-stack-protector -fno-builtin -O2 -Wall -Wextra -fuse-ld=lld -Wl,--build-id=none -Wl,-e,_start /home/twrp/t40wm_init_v43.c -o /home/twrp/t40wm_init_v43
sha256sum /home/twrp/t40wm_init_v43
file /home/twrp/t40wm_init_v43
readelf -l /home/twrp/t40wm_init_v43
