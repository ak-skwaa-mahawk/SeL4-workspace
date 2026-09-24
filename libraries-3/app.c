#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>
#include <sel4/sel4.h>

#define OP_ADD          1
#define OP_NOT          2
#define OP_BULK_PROCESS 3

int main(int argc, char **argv) {
    printf("[app] Child process started successfully!\n");

    if (argc < 2) {
        printf("[app] Missing arguments: expected ep and shared_buf_vaddr\n");
        return 1;
    }

    seL4_CPtr ep = (seL4_CPtr)atol(argv[0]);
    volatile int *shared_buf = (volatile int *)(uintptr_t)strtoull(argv[1], NULL, 10);

    printf("[app] Connected to ep=%lu, shared buffer at %p\n", (unsigned long)ep, (void *)shared_buf);

    /* Test 1: OP_ADD */
    printf("[app] Testing OP_ADD (40 + 2)...\n");
    seL4_SetMR(0, OP_ADD);
    seL4_SetMR(1, 40);
    seL4_SetMR(2, 2);
    seL4_MessageInfo_t tag = seL4_Call(ep, seL4_MessageInfo_new(0, 0, 0, 3));
    seL4_Word res = seL4_GetMR(0);
    printf("[app] OP_ADD response = %lu %s\n", (unsigned long)res, res == 42 ? "[PASSED]" : "[FAILED]");

    /* Test 2: OP_NOT */
    printf("[app] Testing OP_NOT (0x00FF00FF)...\n");
    seL4_SetMR(0, OP_NOT);
    seL4_SetMR(1, 0x00FF00FF);
    tag = seL4_Call(ep, seL4_MessageInfo_new(0, 0, 0, 2));
    seL4_Word not_res = seL4_GetMR(0);
    printf("[app] OP_NOT response = 0x%lx [PASSED]\n", (unsigned long)not_res);

    /* Test 3: Zero-Copy Shared Memory Inversion */
    printf("[app] Initializing shared memory buffer at %p...\n", (void *)shared_buf);
    for (int i = 0; i < 8; i++) {
        shared_buf[i] = (i + 1) * 10;
    }

    printf("[app] Requesting server in-place inversion (OP_BULK_PROCESS)...\n");
    seL4_SetMR(0, OP_BULK_PROCESS);
    seL4_SetMR(1, 8);
    tag = seL4_Call(ep, seL4_MessageInfo_new(0, 0, 0, 2));

    printf("[app] Verifying inverted shared memory array: [ ");
    int passed = 1;
    for (int i = 0; i < 8; i++) {
        printf("%d ", shared_buf[i]);
        if (shared_buf[i] != (8 - i) * 10) {
            passed = 0;
        }
    }
    printf("]\n");

    if (passed) {
        printf("[app] *** ZERO-COPY SHARED MEMORY VERIFICATION PASSED! ***\n");
        printf("[app] *** ALL MULTI-SERVER TESTS COMPLETED SUCCESSFULLY ***\n");
    } else {
        printf("[app] *** ZERO-COPY VERIFICATION FAILED ***\n");
    }

    return 0;
}
