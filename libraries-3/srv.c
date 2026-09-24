#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>

#define OP_ADD          1
#define OP_NOT          2
#define OP_BULK_PROCESS 3

int main(int argc, char **argv) {
    int ep_idx = (argc >= 3) ? 1 : 0;
    int buf_idx = (argc >= 3) ? 2 : 1;

    seL4_CPtr ep = (seL4_CPtr)strtoul(argv[ep_idx], NULL, 10);
    volatile int *shared_buf = (volatile int *)(uintptr_t)strtoull(argv[buf_idx], NULL, 10);

    seL4_Word sender_badge = 0;
    seL4_MessageInfo_t info = seL4_Recv(ep, &sender_badge);

    while (1) {
        seL4_Word op = seL4_GetMR(0);
        seL4_MessageInfo_t reply_tag;

        if (op == OP_ADD) {
            seL4_Word a = seL4_GetMR(1);
            seL4_Word b = seL4_GetMR(2);
            seL4_SetMR(0, a + b);
            reply_tag = seL4_MessageInfo_new(0, 0, 0, 1);
        } else if (op == OP_NOT) {
            seL4_Word val = seL4_GetMR(1);
            seL4_SetMR(0, ~val);
            reply_tag = seL4_MessageInfo_new(0, 0, 0, 1);
        } else if (op == OP_BULK_PROCESS) {
            seL4_Word len = seL4_GetMR(1);
            for (seL4_Word i = 0; i < len / 2; i++) {
                int tmp = shared_buf[i];
                shared_buf[i] = shared_buf[len - 1 - i];
                shared_buf[len - 1 - i] = tmp;
            }
            seL4_SetMR(0, 0);
            reply_tag = seL4_MessageInfo_new(0, 0, 0, 1);
        } else {
            reply_tag = seL4_MessageInfo_new(0, 0, 0, 0);
        }

        info = seL4_ReplyRecv(ep, reply_tag, &sender_badge);
    }
    return 0;
}
