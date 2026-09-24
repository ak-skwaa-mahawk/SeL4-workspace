#include <autoconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
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
#include "sha256.h"
#include "uart_com2.h"

#define CLIENT_IMAGE_NAME "app"
#define CLIENT_BADGE_AUTH   0xC001
#define CLIENT_BADGE_UNAUTH 0x0002

#define OP_ADD            1
#define OP_NOT            2
#define OP_BULK_PROCESS   3

#define ALLOCATOR_STATIC_POOL_SIZE (BIT(seL4_PageBits) * 10)
static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];
#define ALLOCATOR_VIRTUAL_POOL_SIZE (BIT(seL4_PageBits) * 100)
static sel4utils_alloc_data_t data;

#define SHARED_BUF_VADDR ((void *)0x50000000ULL)
#define NUM_PAGE_LEVELS 4

static void evaluate_and_hash_frame(volatile sovereign_audit_frame_t *audit_frame) {
    sha256_ctx_t ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, (const void *)audit_frame->claimant, sizeof(audit_frame->claimant));
    sha256_update(&ctx, (const void *)audit_frame->dockets, sizeof(audit_frame->dockets));
    size_t nodes_size = sizeof(c_lineage_node_t) * audit_frame->node_count;
    sha256_update(&ctx, (const void *)audit_frame->nodes, nodes_size);

    uint8_t digest[32];
    sha256_final(&ctx, digest);
    memcpy((void *)audit_frame->computed_root_hash, digest, 32);

    int has_orig_title = 0;
    for (int i = 0; i < audit_frame->node_count && i < MAX_NODES; i++) {
        if (audit_frame->nodes[i].title_type == TITLE_ABORIGINAL_SOVEREIGN) {
            has_orig_title = 1;
            break;
        }
    }
    int has_judicial_orders = (audit_frame->dockets[0][0] != '\0');

    if (has_orig_title && has_judicial_orders) {
        audit_frame->statutory_duty = 1;
        audit_frame->corporate_defense_valid = 0;
        audit_frame->can_be_administered_away = 0;
    } else {
        audit_frame->statutory_duty = 0;
        audit_frame->corporate_defense_valid = 1;
        audit_frame->can_be_administered_away = 1;
    }
}

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

    /* 1. Endpoint allocation */
    vka_object_t ep_object = {0};
    error = vka_alloc_endpoint(&vka, &ep_object);
    ZF_LOGF_IFERR(error, "Failed to allocate endpoint");

    cspacepath_t ep_path;
    vka_cspace_make_path(&vka, ep_object.cptr, &ep_path);

    /* 2. Shared physical page allocation */
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

    vka_object_t root_pt_objects[NUM_PAGE_LEVELS];
    int root_num_pt_objects = 0;
    error = sel4utils_map_page(&vka, seL4_CapInitThreadVSpace, frame_path_root.capPtr, SHARED_BUF_VADDR,
                               seL4_AllRights, 1, root_pt_objects, &root_num_pt_objects);
    ZF_LOGF_IFERR(error, "Failed to map shared page into root task");

    /* 3. COM2 Port I/O Capability */
    cspacepath_t com2_ioport_path;
    error = vka_cspace_alloc_path(&vka, &com2_ioport_path);
    ZF_LOGF_IFERR(error, "Failed to allocate cspace slot for COM2 IOPort");

    error = simple_get_IOPort_cap(&simple, COM2_PORT_BASE, COM2_PORT_TOP,
                                  com2_ioport_path.root, com2_ioport_path.capPtr, com2_ioport_path.capDepth);
    ZF_LOGF_IFERR(error, "Failed to obtain COM2 IOPort capability");

    uart_com2_init(com2_ioport_path.capPtr);
    printf("rootserver: [COM2] Continuous Structured UART listener active at 0x%x-0x%x\n",
           COM2_PORT_BASE, COM2_PORT_TOP);

    /* 4. Client Process Configuration */
    sel4utils_process_t client_proc;
    sel4utils_process_config_t client_conf = process_config_default_simple(&simple, CLIENT_IMAGE_NAME, 200);
    error = sel4utils_configure_process_custom(&client_proc, &vka, &vspace, client_conf);
    ZF_LOGF_IFERR(error, "Failed to configure client process");

    seL4_CPtr client_ep_auth = sel4utils_mint_cap_to_process(&client_proc, ep_path, seL4_AllRights, CLIENT_BADGE_AUTH);
    assert(client_ep_auth != 0);

    seL4_CPtr client_ep_unauth = sel4utils_mint_cap_to_process(&client_proc, ep_path, seL4_AllRights, CLIENT_BADGE_UNAUTH);
    assert(client_ep_unauth != 0);

    vka_object_t client_pt_objects[NUM_PAGE_LEVELS];
    int client_num_pt_objects = 0;
    error = sel4utils_map_page(&vka, client_proc.pd.cptr, frame_path_orig.capPtr, SHARED_BUF_VADDR,
                               seL4_AllRights, 1, client_pt_objects, &client_num_pt_objects);
    ZF_LOGF_IFERR(error, "Failed to map shared page into client");

    char client_arg_strings[3][WORD_STRING_SIZE];
    char *client_argv[3];
    sel4utils_create_word_args(client_arg_strings, client_argv, 3, client_ep_auth, client_ep_unauth, (seL4_Word)SHARED_BUF_VADDR);
    error = sel4utils_spawn_process_v(&client_proc, &vka, &vspace, 3, client_argv, 1);
    ZF_LOGF_IFERR(error, "Failed to spawn client");

    printf("rootserver: client dispatched. Entering unified non-blocking event loop...\n");

    volatile sovereign_audit_frame_t *audit_frame = (volatile sovereign_audit_frame_t *)SHARED_BUF_VADDR;
    volatile int *shared_buf = (volatile int *)SHARED_BUF_VADDR;

    uint8_t rx_buffer[sizeof(sovereign_audit_frame_t)];
    size_t rx_index = 0;

    while (1) {
        bool had_work = false;

        /* --- 1. COM2 Serial Ingestion --- */
        while (uart_com2_has_data(com2_ioport_path.capPtr)) {
            had_work = true;
            uint8_t byte = uart_com2_read_byte(com2_ioport_path.capPtr);
            rx_buffer[rx_index++] = byte;

            if (rx_index == sizeof(sovereign_audit_frame_t)) {
                rx_index = 0;
                sovereign_audit_frame_t *incoming = (sovereign_audit_frame_t *)rx_buffer;
                sovereign_response_frame_t resp = {0};
                resp.magic = SOVA_MAGIC;

                if (incoming->magic != SOVR_MAGIC) {
                    printf("rootserver: [COM2 REJECT] Invalid magic 0x%08x\n", (unsigned int)incoming->magic);
                    resp.status_code = SOVR_STATUS_ERR_MAGIC;
                } else if (incoming->node_count > MAX_NODES) {
                    printf("rootserver: [COM2 REJECT] Node count out of bounds (%u > %u)\n", incoming->node_count, MAX_NODES);
                    resp.status_code = SOVR_STATUS_ERR_BOUNDS;
                } else {
                    memcpy((void *)audit_frame, rx_buffer, sizeof(sovereign_audit_frame_t));
                    evaluate_and_hash_frame(audit_frame);

                    resp.status_code = SOVR_STATUS_SUCCESS;
                    if (audit_frame->statutory_duty)           resp.flags |= SOVR_FLAG_STATUTORY_DUTY;
                    if (audit_frame->corporate_defense_valid)  resp.flags |= SOVR_FLAG_CORP_DEFENSE_VALID;
                    if (audit_frame->can_be_administered_away) resp.flags |= SOVR_FLAG_CAN_BE_ADMINISTERED;
                    memcpy(resp.root_hash, (const void *)audit_frame->computed_root_hash, 32);

                    printf("rootserver: [COM2 ACK] Certified frame. Status: 0x%04x, Flags: 0x%04x\n", resp.status_code, resp.flags);
                }

                uart_com2_write_exact(com2_ioport_path.capPtr, (const uint8_t *)&resp, sizeof(resp));
            }
        }

        /* --- 2. Non-Blocking IPC Servicing --- */
        seL4_Word sender_badge = 0;
        seL4_MessageInfo_t msg_info = seL4_NBRecv(ep_object.cptr, &sender_badge);

        if (seL4_MessageInfo_get_length(msg_info) > 0 || sender_badge != 0) {
            had_work = true;
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
            } else if (op == OP_SOVR_EVALUATE) {
                if (sender_badge != ROLE_FIDUCIARY_PR) {
                    seL4_SetMR(0, SOVR_STATUS_ERR_UNAUTH);
                    reply_tag = seL4_MessageInfo_new(0, 0, 0, 1);
                } else if (audit_frame->magic != SOVR_MAGIC) {
                    seL4_SetMR(0, SOVR_STATUS_ERR_MAGIC);
                    reply_tag = seL4_MessageInfo_new(0, 0, 0, 1);
                } else {
                    evaluate_and_hash_frame(audit_frame);
                    seL4_SetMR(0, SOVR_STATUS_SUCCESS);
                    reply_tag = seL4_MessageInfo_new(0, 0, 0, 1);
                }
            } else {
                reply_tag = seL4_MessageInfo_new(0, 0, 0, 0);
            }

            seL4_Reply(reply_tag);
        }

        if (!had_work) {
            seL4_Yield();
        }
    }

    return 0;
}
