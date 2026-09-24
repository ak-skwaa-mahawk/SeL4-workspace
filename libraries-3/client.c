#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <sel4/sel4.h>
#include <sel4debug/debug.h>

#define OP_ADD          1
#define OP_NOT          2
#define OP_BULK_PROCESS 3

static void dbg_puts(const char *s) {
    while (*s) {
        seL4_DebugPutChar(*s++);
    }
}

int main(int argc, char **argv) {
    dbg_puts("[client] started!\n");

    seL4_CPtr ep = (seL4_CPtr)strtoul(argv[0], NULL, 10);
    volatile int *shared_buf = (volatile int *)(uintptr_t)strtoull(argv[1], NULL, 10);

    /* 1. OP_ADD test */
    dbg_puts("[client] testing OP_ADD (40 + 2)...\n");
    seL4_SetMR(0, OP_ADD);
    seL4_SetMR(1, 40);
    seL4_SetMR(2, 2);
    seL4_Call(ep, seL4_MessageInfo_new(0, 0, 0, 3));
    seL4_Word res = seL4_GetMR(0);

    if (res == 42) {
        dbg_puts("[client] OP_ADD test PASSED: 42\n");
    } else {
        dbg_puts("[client] OP_ADD test FAILED\n");
    }

    /* 2. Zero-copy shared memory buffer test */
    dbg_puts("[client] populating shared buffer at 0x50000000...\n");
    for (int i = 0; i < 8; i++) {
        shared_buf[i] = (i + 1) * 10;
    }

    dbg_puts("[client] sending OP_BULK_PROCESS...\n");
    seL4_SetMR(0, OP_BULK_PROCESS);
    seL4_SetMR(1, 8);
    seL4_Call(ep, seL4_MessageInfo_new(0, 0, 0, 2));

    dbg_puts("[client] verifying shared memory values after inversion...\n");
    int passed = 1;
    for (int i = 0; i < 8; i++) {
        if (shared_buf[i] != (8 - i) * 10) {
            passed = 0;
            break;
        }
    }

    if (passed) {
        dbg_puts("[client] *** SHARED MEMORY ZERO-COPY VERIFICATION PASSED! ***\n");
        dbg_puts("[client] *** ALL MULTI-SERVER TESTS COMPLETED SUCCESSFULLY ***\n");
    } else {
        dbg_puts("[client] *** SHARED MEMORY VERIFICATION FAILED ***\n");
    }

    while (1) {
        seL4_Yield();
    }
    return 0;
}
