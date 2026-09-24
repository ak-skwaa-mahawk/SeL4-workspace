#include <stdio.h>
#include <stdlib.h>
#include <sel4/sel4.h>
#include <utils/zf_log.h>

#define OP_ADD 1
#define OP_NOT 2

int main(int argc, char **argv) {
    if (argc < 1) {
        printf("client: error, missing endpoint capability argument\n");
        return 1;
    }

    seL4_CPtr srv_ep = (seL4_CPtr)atol(argv[0]);
    printf("client: starting tests against server endpoint %lu...\n", (unsigned long)srv_ep);

    /* Test 1: OP_ADD (105 + 215) */
    seL4_SetMR(0, OP_ADD);
    seL4_SetMR(1, 105);
    seL4_SetMR(2, 215);
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 3);
    tag = seL4_Call(srv_ep, tag);

    seL4_Word add_res = seL4_GetMR(0);
    printf("client: OP_ADD result = %lu (Expected: 320)\n", (unsigned long)add_res);

    /* Test 2: OP_NOT (0xAAAAAAAA) */
    seL4_SetMR(0, OP_NOT);
    seL4_SetMR(1, 0xAAAAAAAA);
    tag = seL4_MessageInfo_new(0, 0, 0, 2);
    tag = seL4_Call(srv_ep, tag);

    seL4_Word not_res = seL4_GetMR(0);
    printf("client: OP_NOT result = 0x%lx\n", (unsigned long)not_res);

    printf("client: all multi-server RPC operations succeeded!\n");
    return 0;
}
