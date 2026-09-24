#!/bin/bash
set -e

IMG="${1:-images/libraries-3-image-x86_64-pc99}"
KERNEL="images/kernel-x86_64-pc99"

if [ ! -f "$KERNEL" ]; then
    echo "[-] Error: kernel binary not found at $KERNEL. Execute from multiserver_build."
    exit 1
fi

if [ ! -f "$IMG" ]; then
    echo "[-] Error: userland image not found at $IMG"
    exit 1
fi

# Clear any lingering listener on 9998
fuser -k 9998/tcp 2>/dev/null || true

echo "[*] Booting seL4 with dual UART interfaces:"
echo "    -> COM1 (0x3f8): mon:stdio"
echo "    -> COM2 (0x2f8): tcp:127.0.0.1:9998,server,nowait"

exec qemu-system-x86_64 \
    -machine pc \
    -cpu Nehalem,-vme,+pdpe1gb,-xsave,-xsaveopt,-xsavec,-fsgsbase,-invpcid,+syscall,+lm \
    -m 512M \
    -display none \
    -serial mon:stdio \
    -serial tcp:127.0.0.1:9998,server,nowait \
    -no-reboot \
    -kernel "$KERNEL" \
    -initrd "$IMG"
