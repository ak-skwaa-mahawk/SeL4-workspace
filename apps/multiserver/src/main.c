#include <autoconf.h>
#include <stdio.h>
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

#define CLIENT_BADGE 0xC001
#define SRV_IMAGE_NAME "compute_srv"
#define CLIENT_IMAGE_NAME "client"

#define ALLOCATOR_STATIC_POOL_SIZE (BIT(seL4_PageBits) * 10)
static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];
#define ALLOCATOR_VIRTUAL_POOL_SIZE (BIT(seL4_PageBits) * 100)
static sel4utils_alloc_data_t data;

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

    /* Allocate communication endpoint */
    vka_object_t ep_object = {0};
    error = vka_alloc_endpoint(&vka, &ep_object);
    ZF_LOGF_IFERR(error, "Failed to allocate endpoint");

    cspacepath_t ep_path;
    vka_cspace_make_path(&vka, ep_object.cptr, &ep_path);

    /* 1. Configure and Spawn Compute Server */
    sel4utils_process_t srv_proc;
    sel4utils_process_config_t srv_conf = process_config_default_simple(&simple, SRV_IMAGE_NAME, seL4_MaxPrio - 1);
    error = sel4utils_configure_process_custom(&srv_proc, &vka, &vspace, srv_conf);
    ZF_LOGF_IFERR(error, "Failed to configure compute_srv process");

    seL4_CPtr srv_ep_cap = sel4utils_mint_cap_to_process(&srv_proc, ep_path, seL4_AllRights, 0);
    assert(srv_ep_cap != 0);

    char srv_arg_strings[1][WORD_STRING_SIZE];
    char *srv_argv[1];
    sel4utils_create_word_args(srv_arg_strings, srv_argv, 1, srv_ep_cap);
    error = sel4utils_spawn_process_v(&srv_proc, &vka, &vspace, 1, srv_argv, 1);
    ZF_LOGF_IFERR(error, "Failed to spawn compute_srv");

    /* 2. Configure and Spawn Client */
    sel4utils_process_t client_proc;
    sel4utils_process_config_t client_conf = process_config_default_simple(&simple, CLIENT_IMAGE_NAME, seL4_MaxPrio - 2);
    error = sel4utils_configure_process_custom(&client_proc, &vka, &vspace, client_conf);
    ZF_LOGF_IFERR(error, "Failed to configure client process");

    seL4_CPtr client_ep_cap = sel4utils_mint_cap_to_process(&client_proc, ep_path, seL4_AllRights, CLIENT_BADGE);
    assert(client_ep_cap != 0);

    char client_arg_strings[1][WORD_STRING_SIZE];
    char *client_argv[1];
    sel4utils_create_word_args(client_arg_strings, client_argv, 1, client_ep_cap);
    error = sel4utils_spawn_process_v(&client_proc, &vka, &vspace, 1, client_argv, 1);
    ZF_LOGF_IFERR(error, "Failed to spawn client");

    printf("rootserver: both servers dispatched and running.\n");

    while (1) {
        seL4_Yield();
    }

    return 0;
}
