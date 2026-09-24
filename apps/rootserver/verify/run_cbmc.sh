#!/usr/bin/env bash
set -euo pipefail

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HARNESS="$DIR/verify_harness.c"

echo "=== [CBMC] Bounded Model Checking: Multi-Node Batch Sweep ==="
echo "Target Harness : $HARNESS"
echo "Unwind Bound   : 16"

cbmc "$HARNESS" \
    --unwind 16 \
    --unwinding-assertions \
    --bounds-check \
    --pointer-check \
    --memory-leak-check \
    --div-by-zero-check \
    --signed-overflow-check \
    --unsigned-overflow-check \
    --trace

echo "=== [CBMC] SUCCESS: All invariants and bounds verified (0 errors) ==="
