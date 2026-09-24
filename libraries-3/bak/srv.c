#include <stdio.h>
#include <stdlib.h>
#include <sel4/sel4.h>
#include <utils/zf_log.h>

#define OP_ADD 1
#define OP_NOT 2

int main(int argc, char **argv) {
    printf("compute_srv: child process started!\n");

    if (argc < 1) {
        printf("compute_srv: missing endpoint capability argument\n");
        return 1;
    }

    seL4_CPtr ep = (seL4_CPtr)atol(argv[0]);
    seL4_Word sender_badge = 0;
    seL4_MessageInfo_t tag;

    printf("compute_srv: listening on endpoint %lu...\n", (unsigned long)ep);

    tag = seL4_Recv(ep, &sender_badge);

    while (1) {
        seL4_Word op = seL4_GetMR(0);
        seL4_Word resp = 0;

        printf("compute_srv: received request from badge 0x%lx, opcode %lu\n",
               (unsigned long)sender_badge, (unsigned long)op);

        if (op == OP_ADD) {
            seL4_Word a = seL4_GetMR(1);
            seL4_Word b = seL4_GetMR(2);
            resp = a + b;
            printf("compute_srv: OP_ADD(%lu, %lu) = %lu\n", (unsigned long)a, (unsigned long)b, (unsigned long)resp);
        } else if (op == OP_NOT) {
            seL4_Word a = seL4_GetMR(1);
            resp = ~a;
            printf("compute_srv: OP_NOT(0x%lx) = 0x%lx\n", (unsigned long)a, (unsigned long)resp);
        }

        seL4_SetMR(0, resp);
        tag = seL4_MessageInfo_new(0, 0, 0, 1);
        tag = seL4_ReplyRecv(ep, tag, &sender_badge);
    }

    return 0;
}
