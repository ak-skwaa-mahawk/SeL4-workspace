#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sel4/sel4.h>
#include <utils/zf_log.h>

#define OP_ADD          1
#define OP_NOT          2
#define OP_BULK_PROCESS 3

int main(int argc, char **argv) {
    printf("compute_srv: child process started!\n");

    if (argc < 2) {
        printf("compute_srv: missing arguments (ep, shared_vaddr)\n");
        return 1;
    }

    seL4_CPtr ep = (seL4_CPtr)atol(argv[0]);
    volatile uint32_t *shared_buf = (volatile uint32_t *)atol(argv[1]);
    seL4_Word sender_badge = 0;
    seL4_MessageInfo_t tag;

    printf("compute_srv: listening on endpoint %lu, shared buffer at %p...\n", 
           (unsigned long)ep, (void *)shared_buf);

    tag = seL4_Recv(ep, &sender_badge);

    while (1) {
        seL4_Word op = seL4_GetMR(0);
        seL4_Word resp = 0;

        if (op == OP_ADD) {
            seL4_Word a = seL4_GetMR(1);
            seL4_Word b = seL4_GetMR(2);
            resp = a + b;
            printf("compute_srv: OP_ADD(%lu, %lu) = %lu\n", (unsigned long)a, (unsigned long)b, (unsigned long)resp);
        } else if (op == OP_NOT) {
            seL4_Word a = seL4_GetMR(1);
            resp = ~a;
            printf("compute_srv: OP_NOT(0x%lx) = 0x%lx\n", (unsigned long)a, (unsigned long)resp);
        } else if (op == OP_BULK_PROCESS) {
            seL4_Word count = seL4_GetMR(1);
            printf("compute_srv: OP_BULK_PROCESS processing %lu elements in shared memory...\n", (unsigned long)count);

            uint32_t sum = 0;
            /* Perform in-place reversal and compute checksum */
            for (size_t i = 0; i < count / 2; i++) {
                uint32_t tmp = shared_buf[i];
                shared_buf[i] = shared_buf[count - 1 - i];
                shared_buf[count - 1 - i] = tmp;
            }
            for (size_t i = 0; i < count; i++) {
                sum += shared_buf[i];
            }
            resp = sum;
            printf("compute_srv: bulk processing complete, checksum = %u\n", sum);
        }

        seL4_SetMR(0, resp);
        tag = seL4_MessageInfo_new(0, 0, 0, 1);
        tag = seL4_ReplyRecv(ep, tag, &sender_badge);
    }

    return 0;
}
