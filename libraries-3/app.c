#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <sel4/sel4.h>

#include "sovereign_contract.h"

#define OP_ADD            1
#define OP_NOT            2
#define OP_BULK_PROCESS   3

int main(int argc, char **argv) {
    printf("[app] Child process started successfully! (argc=%d)\n", argc);
    assert(argc >= 2);

    /* Handle both formats: [ep, vaddr] when argc==2, or [prog, ep, vaddr] when argc==3 */
    int ep_idx = (argc == 2) ? 0 : 1;
    int vaddr_idx = (argc == 2) ? 1 : 2;

    seL4_CPtr ep = (seL4_CPtr)strtoul(argv[ep_idx], NULL, 0);
    void *shared_vaddr = (void *)strtoul(argv[vaddr_idx], NULL, 0);
    assert(ep != 0);
    assert(shared_vaddr != NULL);

    printf("[app] Connected to ep=%lu, shared buffer at %p\n", (unsigned long)ep, shared_vaddr);

    /* Test 1: OP_ADD */
    seL4_SetMR(0, OP_ADD);
    seL4_SetMR(1, 40);
    seL4_SetMR(2, 2);
    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, 3);
    info = seL4_Call(ep, info);
    assert(seL4_MessageInfo_get_length(info) == 1);
    seL4_Word sum = seL4_GetMR(0);
    printf("[app] OP_ADD response = %lu [PASSED]\n", (unsigned long)sum);
    assert(sum == 42);

    /* Test 2: OP_NOT */
    seL4_SetMR(0, OP_NOT);
    seL4_SetMR(1, 0x00FF00FF);
    info = seL4_MessageInfo_new(0, 0, 0, 2);
    info = seL4_Call(ep, info);
    assert(seL4_MessageInfo_get_length(info) == 1);
    seL4_Word inverted = seL4_GetMR(0);
    printf("[app] OP_NOT response = 0x%lx [PASSED]\n", (unsigned long)inverted);
    assert(inverted == ~((seL4_Word)0x00FF00FF));

    /* Test 3: Sovereign Audit Frame Evaluation */
    printf("[app] Initializing SovereignAuditFrame at %p...\n", shared_vaddr);
    volatile sovereign_audit_frame_t *frame = (volatile sovereign_audit_frame_t *)shared_vaddr;
    memset((void *)frame, 0, sizeof(sovereign_audit_frame_t));

    frame->magic = SOVR_MAGIC;
    frame->version = SOVR_VERSION;
    frame->fiduciary_role = ROLE_FIDUCIARY_PR;
    frame->veteran_verified = 1;
    frame->node_count = 3;
    strncpy((char *)frame->claimant, "John B. J. Carroll", sizeof(frame->claimant) - 1);
    strncpy((char *)frame->dockets[0], "STATE-PROBATE-ORDER-ALASKA", MAX_DOCKET_STR_LEN - 1);
    strncpy((char *)frame->dockets[1], "DOI-BIA-HEIRSHIP-1", MAX_DOCKET_STR_LEN - 1);
    strncpy((char *)frame->dockets[2], "DOI-BIA-HEIRSHIP-2", MAX_DOCKET_STR_LEN - 1);

    /* Node 0: Dahzhit */
    strncpy((char *)frame->nodes[0].name, "Dahzhit (Dehjalti')", sizeof(frame->nodes[0].name) - 1);
    frame->nodes[0].era_year = 1795;
    strncpy((char *)frame->nodes[0].territorial_hub, "Yukon / Porcupine River", sizeof(frame->nodes[0].territorial_hub) - 1);
    frame->nodes[0].title_type = TITLE_ABORIGINAL_SOVEREIGN;

    /* Node 1: Shahvyah */
    strncpy((char *)frame->nodes[1].name, "Shahvyah", sizeof(frame->nodes[1].name) - 1);
    frame->nodes[1].era_year = 1820;
    strncpy((char *)frame->nodes[1].territorial_hub, "Circle City Corridor", sizeof(frame->nodes[1].territorial_hub) - 1);
    frame->nodes[1].title_type = TITLE_ABORIGINAL_SOVEREIGN;

    /* Node 2: Christopher Carroll */
    strncpy((char *)frame->nodes[2].name, "Christopher Carroll", sizeof(frame->nodes[2].name) - 1);
    frame->nodes[2].era_year = 1744;
    strncpy((char *)frame->nodes[2].territorial_hub, "Atlantic Frontier", sizeof(frame->nodes[2].territorial_hub) - 1);
    frame->nodes[2].title_type = TITLE_ABORIGINAL_SOVEREIGN;

    printf("[app] Invoking OP_SOVR_EVALUATE via badged endpoint...\n");
    seL4_SetMR(0, OP_SOVR_EVALUATE);
    info = seL4_MessageInfo_new(0, 0, 0, 1);
    info = seL4_Call(ep, info);
    assert(seL4_MessageInfo_get_length(info) == 1);
    assert(seL4_GetMR(0) == 0);

    printf("[app] Verifying evaluation results in shared memory:\n");
    printf("      -> statutory_duty: %u (%s)\n",
           frame->statutory_duty,
           frame->statutory_duty == 1 ? "MANDATORY_ACCOUNTING_REQUIRED" : "DEFERRED");
    printf("      -> corporate_defense_valid: %u\n", frame->corporate_defense_valid);
    printf("      -> can_be_administered_away: %u\n", frame->can_be_administered_away);

    assert(frame->statutory_duty == 1);
    assert(frame->corporate_defense_valid == 0);
    assert(frame->can_be_administered_away == 0);

    printf("[app] *** SOVEREIGN AUDIT CONTRACT VERIFICATION PASSED! ***\n");
    printf("[app] *** ALL MULTI-SERVER TESTS COMPLETED SUCCESSFULLY ***\n");

    return 0;
}
