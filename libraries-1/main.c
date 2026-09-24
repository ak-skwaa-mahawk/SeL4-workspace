/*
 * Copyright 2018, Data61, CSIRO (ABN 41 687 119 230).
 * SPDX-License-Identifier: BSD-2-Clause
 */

/*
 * seL4 tutorial part 2: create and run a new thread
 */

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
#include <utils/arith.h>
#include <utils/zf_log.h>
#include <sel4utils/sel4_zf_logif.h>
#include <sel4utils/thread.h>
#include <sel4platsupport/bootinfo.h>

seL4_BootInfo *info;
simple_t simple;
vka_t vka;
allocman_t *allocman;

#define ALLOCATOR_STATIC_POOL_SIZE (BIT(seL4_PageBits) * 10)
UNUSED static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];

#define THREAD_2_STACK_SIZE 512
static uint64_t thread_2_stack[THREAD_2_STACK_SIZE];

void thread_2(void) {
    /* TASK 15: print something */
    printf("thread_2: hello world\n");
    while (1) {
        seL4_Yield();
    }
}

int main(void) {
    UNUSED int error = 0;

    /* TASK 1: get boot info */
    info = platsupport_get_bootinfo();
    ZF_LOGF_IF(info == NULL, "Failed to get bootinfo.");

    zf_log_set_tag_prefix("libraries-1:");
    NAME_THREAD(seL4_CapInitThreadTCB, "libraries-1");

    /* TASK 2: initialise simple object */
    simple_default_init_bootinfo(&simple, info);

    /* TASK 3: print out bootinfo and other info about simple */
    simple_print(&simple);

    /* TASK 4: create an allocator */
    allocman = bootstrap_use_current_simple(&simple, ALLOCATOR_STATIC_POOL_SIZE, allocator_mem_pool);
    ZF_LOGF_IF(allocman == NULL, "Failed to initialize alloc manager.\n");

    /* TASK 5: create a vka */
    allocman_make_vka(&vka, allocman);

    /* TASK 6: get our cspace root cnode */
    seL4_CPtr cspace_cap = simple_get_cnode(&simple);

    /* TASK 7: get our vspace root */
    seL4_CPtr pd_cap = seL4_CapInitThreadVSpace;

    /* TASK 8: create a new TCB */
    vka_object_t tcb_object = {0};
    error = vka_alloc_tcb(&vka, &tcb_object);
    ZF_LOGF_IFERR(error, "Failed to allocate new TCB.\n");

    /* TASK 9: initialise the new TCB */
    error = seL4_TCB_Configure(tcb_object.cptr, seL4_CapNull, cspace_cap, seL4_NilData, pd_cap, seL4_NilData, 0, seL4_CapNull);
    ZF_LOGF_IFERR(error, "Failed to configure the new TCB object.\n");

    /* Set the priority of the new thread to equal our priority */
    error = seL4_TCB_SetPriority(tcb_object.cptr, simple_get_tcb(&simple), 255);
    ZF_LOGF_IFERR(error, "Failed to set the priority for the new TCB object.\n");

    /* TASK 10: give the new thread a name */
    NAME_THREAD(tcb_object.cptr, "thread_2");

    /* Registers setup */
    UNUSED seL4_UserContext regs = {0};

    /* TASK 11: set instruction pointer */
    sel4utils_set_instruction_pointer(&regs, (seL4_Word)thread_2);

    /* check that stack is aligned correctly */
    const int stack_alignment_requirement = sizeof(seL4_Word) * 2;
    uintptr_t thread_2_stack_top = (uintptr_t)thread_2_stack + sizeof(thread_2_stack);
    ZF_LOGF_IF(thread_2_stack_top % (stack_alignment_requirement) != 0,
               "Stack top isn't aligned correctly.");

    /* TASK 12: set stack pointer for x86_64 ABI */
    sel4utils_set_stack_pointer(&regs, (seL4_Word)thread_2_stack_top - 8);

    /* TASK 13: actually write the TCB registers */
    error = seL4_TCB_WriteRegisters(tcb_object.cptr, 0, 0, sizeof(regs) / sizeof(seL4_Word), &regs);
    ZF_LOGF_IFERR(error, "Failed to write the new thread's register set.\n");

    /* TASK 14: start the new thread running */
    error = seL4_TCB_Resume(tcb_object.cptr);
    ZF_LOGF_IFERR(error, "Failed to start new thread.\n");

    printf("main: hello world\n");
    while (1) {
        seL4_Yield();
    }

    return 0;
}
