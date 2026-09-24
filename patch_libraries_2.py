path = '/data/data/com.termux/files/home/seL4-workspace/libraries-2/main.c'
with open(path, 'r') as f:
    code = f.read()

# Fix VSpace root for x86_64
code = code.replace(
    'pd_cap = simple_get_pd(&simple);',
    'pd_cap = seL4_CapInitThreadVSpace;'
)

# Task 11: wait for message
code = code.replace(
    '    /* TASK 11: wait for a message to come in over the endpoint */',
    '    /* TASK 11: wait for a message to come in over the endpoint */\n    tag = seL4_Recv(ep_object.cptr, &sender_badge);'
)

# Task 12: verify badge and length
code = code.replace(
    '    /* TASK 12: make sure it is what we expected */',
    '    /* TASK 12: make sure it is what we expected */\n    ZF_LOGF_IF(sender_badge != EP_BADGE, "Badge mismatch");\n    ZF_LOGF_IF(seL4_MessageInfo_get_length(tag) != 1, "Length mismatch");'
)

# Task 13: get incoming MR
code = code.replace(
    '    /* TASK 13: get the message stored in the first message register */',
    '    /* TASK 13: get the message stored in the first message register */\n    msg = seL4_GetMR(0);'
)

# Task 14: copy modified MR back
code = code.replace(
    '    /* TASK 14: copy the modified message back into the message register */',
    '    /* TASK 14: copy the modified message back into the message register */\n    seL4_SetMR(0, msg);'
)

# Task 15: send reply
code = code.replace(
    '    /* TASK 15: send the message back */',
    '    /* TASK 15: send the message back */\n    seL4_ReplyRecv(ep_object.cptr, seL4_MessageInfo_new(0, 0, 0, 1), &sender_badge);'
)

# Task 1: alloc frame
code = code.replace(
    '    vka_object_t ipc_frame_object;',
    '    vka_object_t ipc_frame_object = {0};\n    error = vka_alloc_frame(&vka, IPCBUF_FRAME_SIZE_BITS, &ipc_frame_object);\n    ZF_LOGF_IFERR(error, "Failed to allocate frame for IPC buffer");'
)

# Task 2: first map attempt
code = code.replace(
    '    /* TASK 2: try to map the frame the first time  */',
    '    /* TASK 2: try to map the frame the first time  */\n    error = seL4_ARCH_Page_Map(ipc_frame_object.cptr, pd_cap, ipc_buffer_vaddr, seL4_AllRights, seL4_ARCH_Default_VMAttributes);'
)

# Task 3: alloc page table
code = code.replace(
    '        /* TASK 3: create a page table */',
    '        /* TASK 3: create a page table */\n        vka_object_t pt_object = {0};\n        error = vka_alloc_page_table(&vka, &pt_object);'
)

# Task 4: map page table
code = code.replace(
    '        /* TASK 4: map the page table */',
    '        /* TASK 4: map the page table */\n        error = seL4_ARCH_PageTable_Map(pt_object.cptr, pd_cap, ipc_buffer_vaddr, seL4_ARCH_Default_VMAttributes);'
)

# Task 5: retry mapping frame
code = code.replace(
    '        /* TASK 5: then map the frame in */',
    '        /* TASK 5: then map the frame in */\n        error = seL4_ARCH_Page_Map(ipc_frame_object.cptr, pd_cap, ipc_buffer_vaddr, seL4_AllRights, seL4_ARCH_Default_VMAttributes);'
)

# Task 6: alloc endpoint
code = code.replace(
    '    /* TASK 6: create an endpoint */',
    '    /* TASK 6: create an endpoint */\n    error = vka_alloc_endpoint(&vka, &ep_object);'
)

# Task 7: mint badged copy
code = code.replace(
    '     * hint 3: for the badge use EP_BADGE                   */',
    '     * hint 3: for the badge use EP_BADGE                   */\n    error = vka_mint_object(&vka, &ep_object, &ep_cap_path, seL4_AllRights, EP_BADGE);'
)

# Fix SysV ABI stack pointer
code = code.replace(
    'sel4utils_set_stack_pointer(&regs, thread_2_stack_top);',
    'sel4utils_set_stack_pointer(&regs, thread_2_stack_top - 8);'
)

# Task 8: set send data
code = code.replace(
    '    /* TASK 8: set the data to send. We send it in the first message register */',
    '    /* TASK 8: set the data to send. We send it in the first message register */\n    seL4_SetMR(0, MSG_DATA);\n    tag = seL4_MessageInfo_new(0, 0, 0, 1);'
)

# Task 9: call endpoint
code = code.replace(
    '    /* TASK 9: send and wait for a reply. */',
    '    /* TASK 9: send and wait for a reply. */\n    tag = seL4_Call(ep_cap_path.capPtr, tag);'
)

# Task 10: get reply
code = code.replace(
    '    /* TASK 10: get the reply message */',
    '    /* TASK 10: get the reply message */\n    msg = seL4_GetMR(0);'
)

with open(path, 'w') as f:
    f.write(code)

print("Applied libraries-2 patch.")
