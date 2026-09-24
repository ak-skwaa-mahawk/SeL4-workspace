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

static void print_hex(const uint8_t *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        printf("%02x", buf[i]);
    }
    printf("\n");
}

int main(int argc, char **argv) {
    printf("[app] Child process started! (argc=%d)\n", argc);
    assert(argc >= 3);

    int auth_idx = (argc == 3) ? 0 : 1;
    int unauth_idx = (argc == 3) ? 1 : 2;
    int vaddr_idx = (argc == 3) ? 2 : 3;

    seL4_CPtr ep_auth = (seL4_CPtr)strtoul(argv[auth_idx], NULL, 0);
    seL4_CPtr ep_unauth = (seL4_CPtr)strtoul(argv[unauth_idx], NULL, 0);
    void *shared_vaddr = (void *)strtoul(argv[vaddr_idx], NULL, 0);

    assert(ep_auth != 0);
    assert(ep_unauth != 0);
    assert(shared_vaddr != NULL);

    volatile sovereign_audit_frame_t *frame = (volatile sovereign_audit_frame_t *)shared_vaddr;

    /* --- NEGATIVE TEST 1: Unauthorized Badge --- */
    printf("[app] [NEGATIVE TEST 1] Calling OP_SOVR_EVALUATE with unauthorized badge...\n");
    seL4_SetMR(0, OP_SOVR_EVALUATE);
    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, 1);
    info = seL4_Call(ep_unauth, info);
    seL4_Word status = seL4_GetMR(0);
    assert(status == 0xE001);
    printf("[app] [PASSED] Unauthorized capability access properly denied (0x%lx).\n", (unsigned long)status);

    /* --- NEGATIVE TEST 2: Corrupt Magic --- */
    printf("[app] [NEGATIVE TEST 2] Calling with corrupted magic (0xDEADBEEF)...\n");
    memset((void *)frame, 0, sizeof(sovereign_audit_frame_t));
    frame->magic = 0xDEADBEEF;
    seL4_SetMR(0, OP_SOVR_EVALUATE);
    info = seL4_MessageInfo_new(0, 0, 0, 1);
    info = seL4_Call(ep_auth, info);
    status = seL4_GetMR(0);
    assert(status == 0xE002);
    printf("[app] [PASSED] Corrupt magic properly denied (0x%lx).\n", (unsigned long)status);

    /* --- TEST 3: Exact Binary Contract Alignment --- */
    printf("[app] [TEST 3] Initializing authentic SovereignAuditFrame...\n");
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

    /* Node 0 */
    strncpy((char *)frame->nodes[0].name, "Dahzhit (Dehjalti')", 31);
    frame->nodes[0].era_year = 1795;
    strncpy((char *)frame->nodes[0].territorial_hub, "Yukon / Porcupine River Drain", 29);
    frame->nodes[0].title_type = TITLE_ABORIGINAL_SOVEREIGN;

    /* Node 1 */
    strncpy((char *)frame->nodes[1].name, "Shahvyah", 31);
    frame->nodes[1].era_year = 1820;
    strncpy((char *)frame->nodes[1].territorial_hub, "Circle City / Interior River ", 29);
    frame->nodes[1].title_type = TITLE_ABORIGINAL_SOVEREIGN;

    /* Node 2 */
    strncpy((char *)frame->nodes[2].name, "Christopher Carroll", 31);
    frame->nodes[2].era_year = 1744;
    strncpy((char *)frame->nodes[2].territorial_hub, "Atlantic / Early Colonial Tra", 29);
    frame->nodes[2].title_type = TITLE_ABORIGINAL_SOVEREIGN;

    printf("[app] Invoking OP_SOVR_EVALUATE with authenticated badge (0xC001)...\n");
    seL4_SetMR(0, OP_SOVR_EVALUATE);
    info = seL4_MessageInfo_new(0, 0, 0, 1);
    info = seL4_Call(ep_auth, info);
    assert(seL4_GetMR(0) == 0);

    printf("[app] In-kernel SHA-256 Root Evidentiary Hash:\n      ");
    print_hex((const uint8_t *)frame->computed_root_hash, 32);

    /* Byte-for-byte digest verification against Python audit_contract.py output */
    static const uint8_t EXPECTED_DIGEST[32] = {
        0xde, 0x15, 0x48, 0x02, 0xcd, 0x1d, 0xa1, 0x9a,
        0x38, 0x62, 0x4c, 0x54, 0xab, 0x00, 0x4b, 0xfb,
        0x95, 0x65, 0x82, 0x43, 0xfe, 0xb7, 0x2d, 0x41,
        0xb2, 0x70, 0x56, 0x3e, 0x8c, 0xb5, 0xda, 0xea
    };

    assert(memcmp((const void *)frame->computed_root_hash, EXPECTED_DIGEST, 32) == 0);
    printf("[app] *** IN-KERNEL DIGEST MATCHES PYTHON CONTRACT EXACTLY! [0xDE154802...] ***\n");
    printf("[app] *** ALL POSITIVE & NEGATIVE SECURITY TESTS PASSED! ***\n");

    return 0;
}
