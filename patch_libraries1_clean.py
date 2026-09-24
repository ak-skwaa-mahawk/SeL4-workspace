path = '/data/data/com.termux/files/home/seL4-workspace/libraries-1/main.c'
with open(path, 'r') as f:
    code = f.read()

# 1. thread_2 body
code = code.replace(
    '/* TASK 15: print something */\n    /* hint: printf() */',
    '/* TASK 15: print something */\n    printf("thread_2: hello world\\n");'
)

# 2. BootInfo
code = code.replace(
    '/* TASK 1: get boot info */',
    '/* TASK 1: get boot info */\n    info = platsupport_get_bootinfo();'
)

# 3. simple init
code = code.replace(
    '/* TASK 2: initialise simple object */',
    '/* TASK 2: initialise simple object */\n    simple_default_init_bootinfo(&simple, info);'
)

# 4. simple print
code = code.replace(
    '/* TASK 3: print out bootinfo and other info about simple */',
    '/* TASK 3: print out bootinfo and other info about simple */\n    simple_print(&simple);'
)

# 5. allocman
code = code.replace(
    '/* TASK 4: create an allocator */',
    '/* TASK 4: create an allocator */\n    allocman = bootstrap_use_current_simple(&simple, ALLOCATOR_STATIC_POOL_SIZE, allocator_mem_pool);'
)

# 6. make vka
code = code.replace(
    '/* TASK 5: create a vka (interface for interacting with the underlying allocator) */',
    '/* TASK 5: create a vka (interface for interacting with the underlying allocator) */\n    allocman_make_vka(&vka, allocman);'
)

# 7. cspace root
code = code.replace(
    'seL4_CPtr cspace_cap;',
    'seL4_CPtr cspace_cap = simple_get_cnode(&simple);'
)

# 8. vspace root - explicitly use root thread VSpace (PML4) on x86_64
code = code.replace(
    'seL4_CPtr pd_cap;',
    'seL4_CPtr pd_cap = seL4_CapInitThreadVSpace;'
)

# 9. allocate TCB
code = code.replace(
    'vka_object_t tcb_object = {0};',
    'vka_object_t tcb_object = {0};\n    error = vka_alloc_tcb(&vka, &tcb_object);'
)

# 10. configure TCB call after the comment block
tcb_conf_target = '    /* TASK 9: initialise the new TCB */'
# Insert right before the error check
code = code.replace(
    '    ZF_LOGF_IFERR(error, "Failed to configure the new TCB object.',
    '    error = seL4_TCB_Configure(tcb_object.cptr, seL4_CapNull, cspace_cap, seL4_NilData, pd_cap, seL4_NilData, 0, seL4_CapNull);\n    ZF_LOGF_IFERR(error, "Failed to configure the new TCB object.'
)

# 11. name thread
code = code.replace(
    '/* TASK 10: give the new thread a name */',
    '/* TASK 10: give the new thread a name */\n    NAME_THREAD(tcb_object.cptr, "thread_2");'
)

# 12. instruction pointer
code = code.replace(
    '/* TASK 11: set instruction pointer where the thread shoud start running */',
    '/* TASK 11: set instruction pointer where the thread shoud start running */\n    sel4utils_set_instruction_pointer(&regs, (seL4_Word)thread_2);'
)

# 13. stack pointer aligned for SysV ABI
code = code.replace(
    '/* TASK 12: set stack pointer for the new thread */',
    '/* TASK 12: set stack pointer for the new thread */\n    sel4utils_set_stack_pointer(&regs, (seL4_Word)thread_2_stack_top - 8);'
)

# 14. write registers
code = code.replace(
    '    ZF_LOGF_IFERR(error, "Failed to write the new thread\'s register set.',
    '    error = seL4_TCB_WriteRegisters(tcb_object.cptr, 0, 0, sizeof(regs) / sizeof(seL4_Word), &regs);\n    ZF_LOGF_IFERR(error, "Failed to write the new thread\'s register set.'
)

# 15. resume thread
code = code.replace(
    '    ZF_LOGF_IFERR(error, "Failed to start new thread.',
    '    error = seL4_TCB_Resume(tcb_object.cptr);\n    ZF_LOGF_IFERR(error, "Failed to start new thread.'
)

# 16. prevent main from terminating before thread_2 executes
code = code.replace(
    'printf("main: hello world\\n");\n    return 0;',
    'printf("main: hello world\\n");\n    while (1) { seL4_Yield(); }\n    return 0;'
)

with open(path, 'w') as f:
    f.write(code)

print("Applied clean libraries-1 implementation.")
