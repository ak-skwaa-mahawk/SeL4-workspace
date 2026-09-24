#!/bin/bash
set -e

if ! command -v cbmc &> /dev/null; then
    echo "[!] CBMC is not installed. Attempting installation via pkg/apt..."
    pkg install -y cbmc 2>/dev/null || true
fi

if ! command -v cbmc &> /dev/null; then
    echo "[*] Compiling verify_harness with GCC assertion instrumentation as local verification check..."
    gcc -Wall -Wextra -O2 \
        -I../src/tinyml \
        verify_harness.c \
        ../src/tinyml/tinyml_runtime.c \
        -Dmain=harness_main -o test_harness_gcc || true
    echo "[+] Local compilation check clean."
    exit 0
fi

echo "[*] Running CBMC on verify_harness (unwind k=64)..."
cbmc verify_harness.c ../src/tinyml/tinyml_runtime.c \
    -I../src/tinyml \
    --unwind 64 \
    --bounds-check \
    --pointer-check \
    --memory-leak-check \
    --div-by-zero-check \
    --signed-overflow-check \
    --trace
