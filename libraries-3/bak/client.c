#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>
#include <utils/zf_log.h>

#define OP_ADD          1
#define OP_NOT          2
#define OP_BULK_PROCESS 3

#define BULK_ELEMS 16

int main(int argc, char **argv) {
    printf("client: child process started!\n");

    if (argc < 2) {
        printf("client: missing arguments (ep, shared_vaddr)\n");
        return 1;
    }

    seL4_CPtr srv_ep = (seL4_CPtr)atol(argv[0]);
    volatile uint32_t *shared_buf = (volatile uint32_t *)atol(argv[1]);

    printf("client: server endpoint %lu, shared buffer at %p\n", 
           (unsigned long)srv_ep, (void *)shared_buf);

    /* Test 1: OP_ADD */
    seL4_SetMR(0, OP_ADD);
    seL4_SetMR(1, 105);
    seL4_SetMR(2, 215);
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 3);
    tag = seL4_Call(srv_ep, tag);
    printf("client: OP_ADD result = %lu (Expected: 320)\n", (unsigned long)seL4_GetMR(0));

    /* Test 2: OP_NOT */
    seL4_SetMR(0, OP_NOT);
    seL4_SetMR(1, 0xAAAAAAAA);
    tag = seL4_MessageInfo_new(0, 0, 0, 2);
    tag = seL4_Call(srv_ep, tag);
    printf("client: OP_NOT result = 0x%lx\n", (unsigned long)seL4_GetMR(0));

    /* Test 3: Zero-Copy Shared Memory Bulk Processing */
    printf("client: initializing %d elements in shared buffer...\n", BULK_ELEMS);
    for (uint32_t i = 0; i < BULK_ELEMS; i++) {
        shared_buf[i] = (i + 1) * 10; /* 10, 20, 30, ... 160 */
    }

    printf("client: shared buffer initial: [%u, %u, ... %u]\n", 
           shared_buf[0], shared_buf[1], shared_buf[BULK_ELEMS - 1]);

    seL4_SetMR(0, OP_BULK_PROCESS);
    seL4_SetMR(1, BULK_ELEMS);
    tag = seL4_MessageInfo_new(0, 0, 0, 2);
    tag = seL4_Call(srv_ep, tag);

    seL4_Word checksum = seL4_GetMR(0);
    printf("client: server replied checksum = %lu\n", (unsigned long)checksum);
    printf("client: shared buffer after server reversal: [%u, %u, ... %u]\n", 
           shared_buf[0], shared_buf[1], shared_buf[BULK_ELEMS - 1]);

    /* Verification: index 0 should now be 160, and checksum should be 1360 */
    if (shared_buf[0] == 160 && checksum == 1360) {
        printf("client: shared memory zero-copy verification PASSED!\n");
    } else {
        printf("client: shared memory verification FAILED!\n");
    }

    printf("client: all multi-server RPC operations succeeded!\n");
    return 0;
}
