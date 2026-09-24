path = '/data/data/com.termux/files/home/seL4-workspace/libraries-1/main.c'
with open(path, 'r') as f:
    code = f.read()

# Task 15: thread_2 body
code = code.replace(
    '    /* TASK 15: print something */\n    /* hint: printf() */',
    '    /* TASK 15: print something */\n    printf("thread_2: hello world\\n");'
)

# Task 1: bootinfo
code = code.replace(
    '    /* TASK 1: get boot info */',
    '    /* TASK 1: get boot info */\n    info = platsupport_get_bootinfo();'
)

# Task 2: simple init
code = code.replace(
    '   /* TASK 2: initialise simple object */',
    '   /* TASK 2: initialise simple object */\n    simple_default_init_bootinfo(&simple, info);'
)

# Task 3: simple print
code = code.replace(
    '    /* TASK 3: print out bootinfo and other info about simple */',
    '    /* TASK 3: print out bootinfo and other info about simple */\n    simple_print(&simple);'
)

# Task 4: allocman
code = code.replace(
    '    /* TASK 4: create an allocator */',
    '    /* TASK 4: create an allocator */\n    allocman = bootstrap_use_current_simple(&simple, ALLOCATOR_STATIC_POOL_SIZE, allocator_mem_pool);'
)

# Task 5: make vka
code = code.replace(
    '    /* TASK 5: create a vka (interface for interacting with the underlying allocator) */',
    '    /* TASK 5: create a vka (interface for interacting with the underlying allocator) */\n    allocman_make_vka(&vka, allocman);'
)

# Task 6: cspace root
code = code.replace(
    '    seL4_CPtr cspace_cap;',
    '    seL4_CPtr cspace_cap = simple_get_cnode(&simple);'
)

# Task 7: vspace root
code = code.replace(
    '    seL4_CPtr pd_cap;',
    '    seL4_CPtr pd_cap = simple_get_pd(&simple);'
)

# Task 8: alloc tcb
code = code.replace(
    '    vka_object_t tcb_object = {0};',
    '    vka_object_t tcb_object = {0};\n    error = vka_alloc_tcb(&vka, &tcb_object);'
)

# Task 9: configure tcb
code = code.replace(
    '    /* TASK 9: initialise the new TCB */',
    '    /* TASK 9: initialise the new TCB */\n    error = seL4_TCB_Configure(tcb_object.cptr, seL4_CapNull, cspace_cap, seL4_NilData, pd_cap, seL4_NilData, 0, seL4_CapNull);'
)

# Task 10: name thread
code = code.replace(
    '    /* TASK 10: give the new thread a name */',
    '    /* TASK 10: give the new thread a name */\n    NAME_THREAD(tcb_object.cptr, "thread_2");'
)

# Task 11: instruction pointer
code = code.replace(
    '    /* TASK 11: set instruction pointer where the thread shoud start running */',
    '    /* TASK 11: set instruction pointer where the thread shoud start running */\n    sel4utils_set_instruction_pointer(&regs, (seL4_Word)thread_2);'
)

# Task 12: stack pointer
code = code.replace(
    '    /* TASK 12: set stack pointer for the new thread */',
    '    /* TASK 12: set stack pointer for the new thread */\n    sel4utils_set_stack_pointer(&regs, (seL4_Word)thread_2_stack_top);'
)

# Task 13: write registers
code = code.replace(
    '    /* TASK 13: actually write the TCB registers.  We write 2 registers:\n     * instruction pointer is first, stack pointer is second. */',
    '    /* TASK 13: actually write the TCB registers. */\n    error = seL4_TCB_WriteRegisters(tcb_object.cptr, 0, 0, sizeof(regs) / sizeof(seL4_Word), &regs);'
)

# Task 14: resume thread
code = code.replace(
    '    /* TASK 14: start the new thread running */',
    '    /* TASK 14: start the new thread running */\n    error = seL4_TCB_Resume(tcb_object.cptr);'
)

with open(path, 'w') as f:
    f.write(code)

print("Applied libraries-1 tutorial solutions.")
