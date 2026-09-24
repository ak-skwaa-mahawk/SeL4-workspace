#include <autoconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <sel4/sel4.h>
#include <simple/simple.h>
#include <simple-default/simple-default.h>
#include <vka/object.h>
#include <allocman/allocman.h>
#include <allocman/bootstrap.h>
#include <allocman/vka.h>
#include <vspace/vspace.h>
#include <sel4utils/vspace.h>
#include <sel4utils/mapping.h>
#include <sel4utils/process.h>
#include <utils/arith.h>
#include <utils/zf_log.h>
#include <sel4platsupport/bootinfo.h>

#include "sovereign_contract.h"

#define CLIENT_IMAGE_NAME "app"
#define CLIENT_BADGE      0xC001

#define OP_ADD            1
#define OP_NOT            2
#define OP_BULK_PROCESS   3

#define ALLOCATOR_STATIC_POOL_SIZE (BIT(seL4_PageBits) * 10)
static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];
#define ALLOCATOR_VIRTUAL_POOL_SIZE (BIT(seL4_PageBits) * 100)
static sel4utils_alloc_data_t data;

#define SHARED_BUF_VADDR ((void *)0x50000000ULL)
#define NUM_PAGE_LEVELS 4

int main(void) {
    int error;
    seL4_BootInfo *bootinfo = platsupport_get_bootinfo();
    ZF_LOGF_IF(!bootinfo, "Failed to get bootinfo");

    simple_t simple;
    simple_default_init_bootinfo(&simple, bootinfo);

    allocman_t *allocman = bootstrap_use_current_simple(&simple, ALLOCATOR_STATIC_POOL_SIZE, allocator_mem_pool);
    ZF_LOGF_IF(!allocman, "Failed to initialize allocman");

    vka_t vka;
    allocman_make_vka(&vka, allocman);

    vspace_t vspace;
    error = sel4utils_bootstrap_vspace_with_bootinfo_leaky(&vspace, &data, seL4_CapInitThreadVSpace, &vka, bootinfo);
    ZF_LOGF_IFERR(error, "Failed to bootstrap vspace");

    void *vaddr;
    reservation_t virtual_reservation = vspace_reserve_range(&vspace, ALLOCATOR_VIRTUAL_POOL_SIZE, seL4_AllRights, 1, &vaddr);
    assert(virtual_reservation.res);
    bootstrap_configure_virtual_pool(allocman, vaddr, ALLOCATOR_VIRTUAL_POOL_SIZE, seL4_CapInitThreadVSpace);

    /* 1. Allocate communication endpoint */
    vka_object_t ep_object = {0};
    error = vka_alloc_endpoint(&vka, &ep_object);
    ZF_LOGF_IFERR(error, "Failed to allocate endpoint");

    cspacepath_t ep_path;
    vka_cspace_make_path(&vka, ep_object.cptr, &ep_path);

    /* 2. Allocate 4KB physical frame */
    vka_object_t shared_frame_obj = {0};
    error = vka_alloc_frame(&vka, seL4_PageBits, &shared_frame_obj);
    ZF_LOGF_IFERR(error, "Failed to allocate shared frame object");

    cspacepath_t frame_path_orig;
    vka_cspace_make_path(&vka, shared_frame_obj.cptr, &frame_path_orig);

    cspacepath_t frame_path_root;
    error = vka_cspace_alloc_path(&vka, &frame_path_root);
    ZF_LOGF_IFERR(error, "Failed to allocate cspace slot for frame copy");

    error = seL4_CNode_Copy(frame_path_root.root, frame_path_root.capPtr, frame_path_root.capDepth,
                            frame_path_orig.root, frame_path_orig.capPtr, frame_path_orig.capDepth,
                            seL4_AllRights);
    ZF_LOGF_IFERR(error, "Failed to copy frame capability");

    /* Map shared frame into root task */
    vka_object_t root_pt_objects[NUM_PAGE_LEVELS];
    int root_num_pt_objects = 0;
    error = sel4utils_map_page(&vka, seL4_CapInitThreadVSpace, frame_path_root.capPtr, SHARED_BUF_VADDR,
                               seL4_AllRights, 1, root_pt_objects, &root_num_pt_objects);
    ZF_LOGF_IFERR(error, "Failed to map shared page into root task");

    /* 3. Configure Client Process ("app") */
    sel4utils_process_t client_proc;
    sel4utils_process_config_t client_conf = process_config_default_simple(&simple, CLIENT_IMAGE_NAME, 200);
    error = sel4utils_configure_process_custom(&client_proc, &vka, &vspace, client_conf);
    ZF_LOGF_IFERR(error, "Failed to configure client process");

    /* Mint endpoint capability into client CSpace with badge and Grant rights */
    seL4_CPtr client_ep_cap = sel4utils_mint_cap_to_process(&client_proc, ep_path, seL4_AllRights, CLIENT_BADGE);
    assert(client_ep_cap != 0);

    /* Map shared frame into client address space at SHARED_BUF_VADDR */
    vka_object_t client_pt_objects[NUM_PAGE_LEVELS];
    int client_num_pt_objects = 0;
    error = sel4utils_map_page(&vka, client_proc.pd.cptr, frame_path_orig.capPtr, SHARED_BUF_VADDR,
                               seL4_AllRights, 1, client_pt_objects, &client_num_pt_objects);
    ZF_LOGF_IFERR(error, "Failed to map shared page into client");

    /* Pass arguments (ep CPtr and shared buffer address) */
    char client_arg_strings[2][WORD_STRING_SIZE];
    char *client_argv[2];
    sel4utils_create_word_args(client_arg_strings, client_argv, 2, client_ep_cap, (seL4_Word)SHARED_BUF_VADDR);
    error = sel4utils_spawn_process_v(&client_proc, &vka, &vspace, 2, client_argv, 1);
    ZF_LOGF_IFERR(error, "Failed to spawn client");

    printf("rootserver: client dispatched. Entering RPC server loop...\n");

    volatile sovereign_audit_frame_t *audit_frame = (volatile sovereign_audit_frame_t *)SHARED_BUF_VADDR;
    volatile int *shared_buf = (volatile int *)SHARED_BUF_VADDR;

    /* Initial receive */
    seL4_Word sender_badge = 0;
    seL4_MessageInfo_t msg_info = seL4_Recv(ep_object.cptr, &sender_badge);

    while (1) {
        seL4_Word op = seL4_GetMR(0);
        seL4_MessageInfo_t reply_tag;

        if (op == OP_ADD) {
            seL4_Word a = seL4_GetMR(1);
            seL4_Word b = seL4_GetMR(2);
            printf("rootserver: [RPC] OP_ADD(%lu, %lu) -> replying %lu\n",
                   (unsigned long)a, (unsigned long)b, (unsigned long)(a + b));
            seL4_SetMR(0, a + b);
            reply_tag = seL4_MessageInfo_new(0, 0, 0, 1);
        } else if (op == OP_NOT) {
            seL4_Word val = seL4_GetMR(1);
            printf("rootserver: [RPC] OP_NOT(0x%lx) -> replying 0x%lx\n",
                   (unsigned long)val, (unsigned long)(~val));
            seL4_SetMR(0, ~val);
            reply_tag = seL4_MessageInfo_new(0, 0, 0, 1);
        } else if (op == OP_BULK_PROCESS) {
            seL4_Word len = seL4_GetMR(1);
            printf("rootserver: [RPC] OP_BULK_PROCESS in-place reversing %lu elements in shared memory\n",
                   (unsigned long)len);
            for (seL4_Word i = 0; i < len / 2; i++) {
                int tmp = shared_buf[i];
                shared_buf[i] = shared_buf[len - 1 - i];
                shared_buf[len - 1 - i] = tmp;
            }
            seL4_SetMR(0, 0);
            reply_tag = seL4_MessageInfo_new(0, 0, 0, 1);
        } else if (op == OP_SOVR_EVALUATE) {
            printf("rootserver: [RPC] OP_SOVR_EVALUATE requested by badge 0x%lx\n", (unsigned long)sender_badge);

            /* 1. Hardware capability verification */
            int auth_ok = (sender_badge == ROLE_FIDUCIARY_PR);
            int magic_ok = (audit_frame->magic == SOVR_MAGIC);

            if (!magic_ok) {
                printf("rootserver: [AUTH FAIL] Invalid magic 0x%08x\n", (unsigned int)audit_frame->magic);
                seL4_SetMR(0, 0xFFFFFFFF);
                reply_tag = seL4_MessageInfo_new(0, 0, 0, 1);
            } else {
                /* 2. Deterministic rules evaluation */
                int has_orig_title = 0;
                for (int i = 0; i < audit_frame->node_count && i < MAX_NODES; i++) {
                    if (audit_frame->nodes[i].title_type == TITLE_ABORIGINAL_SOVEREIGN) {
                        has_orig_title = 1;
                        break;
                    }
                }
                int has_judicial_orders = (audit_frame->dockets[0][0] != '\0');

                if (auth_ok && has_orig_title && has_judicial_orders) {
                    audit_frame->statutory_duty = 1; /* MANDATORY_ACCOUNTING_REQUIRED */
                    audit_frame->corporate_defense_valid = 0;
                    audit_frame->can_be_administered_away = 0;
                    printf("rootserver: [EVAL PASSED] Claimant: %s -> MANDATORY_ACCOUNTING\n",
                           (char *)audit_frame->claimant);
                } else {
                    audit_frame->statutory_duty = 0; /* DEFER_TO_ADMINISTRATIVE_PROXY */
                    audit_frame->corporate_defense_valid = 1;
                    audit_frame->can_be_administered_away = 1;
                    printf("rootserver: [EVAL DEFERRED] Insufficient standing or dockets\n");
                }

                seL4_SetMR(0, 0); /* Success status */
                reply_tag = seL4_MessageInfo_new(0, 0, 0, 1);
            }
        } else {
            reply_tag = seL4_MessageInfo_new(0, 0, 0, 0);
        }

        msg_info = seL4_ReplyRecv(ep_object.cptr, reply_tag, &sender_badge);
    }

    return 0;
}
