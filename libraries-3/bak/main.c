#include <autoconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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

#define SRV_IMAGE_NAME "compute_srv"

#define OP_ADD          1
#define OP_NOT          2
#define OP_BULK_PROCESS 3

#define ALLOCATOR_STATIC_POOL_SIZE (BIT(seL4_PageBits) * 10)
static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];
#define ALLOCATOR_VIRTUAL_POOL_SIZE (BIT(seL4_PageBits) * 100)
static sel4utils_alloc_data_t data;

#define SHARED_BUF_VADDR ((void *)0x50000000ULL)
#define NUM_PAGE_LEVELS 4

int main(void) {
    int error;
    seL4_BootInfo *info = platsupport_get_bootinfo();
    ZF_LOGF_IF(!info, "Failed to get bootinfo");

    simple_t simple;
    simple_default_init_bootinfo(&simple, info);

    allocman_t *allocman = bootstrap_use_current_simple(&simple, ALLOCATOR_STATIC_POOL_SIZE, allocator_mem_pool);
    ZF_LOGF_IF(!allocman, "Failed to initialize allocman");

    vka_t vka;
    allocman_make_vka(&vka, allocman);

    vspace_t vspace;
    error = sel4utils_bootstrap_vspace_with_bootinfo_leaky(&vspace, &data, seL4_CapInitThreadVSpace, &vka, info);
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

    /* Mint a dedicated call endpoint cap with Grant rights for the root task */
    cspacepath_t root_call_ep_path;
    error = vka_cspace_alloc_path(&vka, &root_call_ep_path);
    ZF_LOGF_IFERR(error, "Failed to allocate cspace slot for root call ep");

    error = seL4_CNode_Mint(root_call_ep_path.root, root_call_ep_path.capPtr, root_call_ep_path.capDepth,
                            ep_path.root, ep_path.capPtr, ep_path.capDepth,
                            seL4_AllRights, 0);
    ZF_LOGF_IFERR(error, "Failed to mint call ep for root task");

    /* 2. Allocate fault endpoint */
    vka_object_t fault_ep_obj = {0};
    error = vka_alloc_endpoint(&vka, &fault_ep_obj);
    ZF_LOGF_IFERR(error, "Failed to allocate fault endpoint");

    /* 3. Allocate 4KB physical frame */
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

    /* Map shared page into root task */
    vka_object_t root_pt_objects[NUM_PAGE_LEVELS];
    int root_num_pt_objects = 0;
    error = sel4utils_map_page(&vka, seL4_CapInitThreadVSpace, frame_path_root.capPtr, SHARED_BUF_VADDR,
                               seL4_AllRights, 1, root_pt_objects, &root_num_pt_objects);
    ZF_LOGF_IFERR(error, "Failed to map shared page into root task");

    /* 4. Configure Compute Server Process */
    sel4utils_process_t srv_proc;
    sel4utils_process_config_t srv_conf = process_config_default_simple(&simple, SRV_IMAGE_NAME, 200);
    srv_conf.fault_endpoint = fault_ep_obj;
    error = sel4utils_configure_process_custom(&srv_proc, &vka, &vspace, srv_conf);
    ZF_LOGF_IFERR(error, "Failed to configure compute_srv process");

    seL4_CPtr srv_ep_cap = sel4utils_mint_cap_to_process(&srv_proc, ep_path, seL4_AllRights, 0);
    assert(srv_ep_cap != 0);

    /* Map shared frame into compute_srv */
    vka_object_t srv_pt_objects[NUM_PAGE_LEVELS];
    int srv_num_pt_objects = 0;
    error = sel4utils_map_page(&vka, srv_proc.pd.cptr, frame_path_orig.capPtr, SHARED_BUF_VADDR,
                               seL4_AllRights, 1, srv_pt_objects, &srv_num_pt_objects);
    ZF_LOGF_IFERR(error, "Failed to map shared page into compute_srv");

    char srv_arg_strings[2][WORD_STRING_SIZE];
    char *srv_argv[2];
    sel4utils_create_word_args(srv_arg_strings, srv_argv, 2, srv_ep_cap, (seL4_Word)SHARED_BUF_VADDR);
    error = sel4utils_spawn_process_v(&srv_proc, &vka, &vspace, 2, srv_argv, 1);
    ZF_LOGF_IFERR(error, "Failed to spawn compute_srv");

    printf("rootserver: compute_srv spawned. Testing IPC...\n");

    /* Yield so compute_srv runs to Recv */
    seL4_TCB_SetPriority(seL4_CapInitThreadTCB, seL4_CapInitThreadTCB, 100);
    seL4_Yield();

    /* Test 1: OP_ADD */
    printf("rootserver: calling OP_ADD...\n");
    seL4_SetMR(0, OP_ADD);
    seL4_SetMR(1, 40);
    seL4_SetMR(2, 2);
    seL4_MessageInfo_t rep = seL4_Call(root_call_ep_path.capPtr, seL4_MessageInfo_new(0, 0, 0, 3));
    seL4_Word res = seL4_GetMR(0);
    printf("rootserver: OP_ADD(40, 2) = %lu %s\n",
           (unsigned long)res, res == 42 ? "[PASSED]" : "[FAILED]");

    /* Test 2: OP_NOT */
    seL4_SetMR(0, OP_NOT);
    seL4_SetMR(1, 0x00FF00FF);
    rep = seL4_Call(root_call_ep_path.capPtr, seL4_MessageInfo_new(0, 0, 0, 2));
    seL4_Word not_res = seL4_GetMR(0);
    printf("rootserver: OP_NOT(0x00FF00FF) = 0x%lx [PASSED]\n", (unsigned long)not_res);

    /* Test 3: Shared Memory Inversion */
    volatile int *shared_buf = (volatile int *)SHARED_BUF_VADDR;
    printf("rootserver: populating shared buffer at %p...\n", SHARED_BUF_VADDR);
    for (int i = 0; i < 8; i++) {
        shared_buf[i] = (i + 1) * 10;
    }

    printf("rootserver: calling OP_BULK_PROCESS (in-place reverse)...\n");
    seL4_SetMR(0, OP_BULK_PROCESS);
    seL4_SetMR(1, 8);
    rep = seL4_Call(root_call_ep_path.capPtr, seL4_MessageInfo_new(0, 0, 0, 2));

    printf("rootserver: reading inverted buffer:\n  [ ");
    int passed = 1;
    for (int i = 0; i < 8; i++) {
        printf("%d ", shared_buf[i]);
        if (shared_buf[i] != (8 - i) * 10) {
            passed = 0;
        }
    }
    printf("]\n");

    if (passed) {
        printf("rootserver: *** SHARED MEMORY ZERO-COPY VERIFICATION PASSED! ***\n");
    } else {
        printf("rootserver: *** SHARED MEMORY VERIFICATION FAILED ***\n");
    }

    printf("rootserver: test run completed.\n");
    while (1) {
        seL4_Yield();
    }

    return 0;
}
