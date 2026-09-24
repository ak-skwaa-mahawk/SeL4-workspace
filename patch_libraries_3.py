import re

# 1. Patch main.c
main_path = '/data/data/com.termux/files/home/seL4-workspace/libraries-3/main.c'
with open(main_path, 'r') as f:
    code = f.read()

# Fix VSpace root for x86_64
code = code.replace(
    'simple_get_pd(&simple)',
    'seL4_CapInitThreadVSpace'
)

# Task 1: Bootstrap VSpace
code = code.replace(
    '    /* TASK 1: create a vspace object to manage our vspace */',
    '    /* TASK 1: create a vspace object to manage our vspace */\n    error = sel4utils_bootstrap_vspace_with_bootinfo_leaky(&vspace, &data, seL4_CapInitThreadVSpace, &vka, info);'
)

# Task 2: Configure process
code = code.replace(
    '    sel4utils_process_t new_process;',
    '''    sel4utils_process_t new_process;
    sel4utils_process_config_t config = process_config_default_simple(&simple, APP_IMAGE_NAME, APP_PRIORITY);
    error = sel4utils_configure_process_custom(&new_process, &vka, &vspace, config);'''
)

# Task 3: Make cspace path
code = code.replace(
    '    cspacepath_t ep_cap_path;\n    seL4_CPtr new_ep_cap = 0;',
    '''    cspacepath_t ep_cap_path;
    vka_cspace_make_path(&vka, ep_object.cptr, &ep_cap_path);
    seL4_CPtr new_ep_cap = 0;'''
)

# Task 4: Mint badged cap to process
code = code.replace(
    '    /* TASK 4: copy the endpont cap and add a badge to the new cap */',
    '    /* TASK 4: copy the endpont cap and add a badge to the new cap */\n    new_ep_cap = sel4utils_mint_cap_to_process(&new_process, ep_cap_path, seL4_AllRights, EP_BADGE);'
)

# Task 5: Spawn process with arguments
code = code.replace(
    '    /* TASK 5: spawn the process */',
    '''    /* TASK 5: spawn the process */
    char strings[1][WORD_STRING_SIZE];
    char *argv[1];
    sel4utils_create_word_args(strings, argv, 1, new_ep_cap);
    error = sel4utils_spawn_process_v(&new_process, &vka, &vspace, 1, argv, 1);'''
)

# Task 6: Wait for message
code = code.replace(
    '    /* TASK 6: wait for a message */',
    '    /* TASK 6: wait for a message */\n    tag = seL4_Recv(ep_object.cptr, &sender_badge);'
)

# Task 7: Send reply
code = code.replace(
    '    /* TASK 7: send the modified message back */',
    '    /* TASK 7: send the modified message back */\n    tag = seL4_ReplyRecv(ep_object.cptr, seL4_MessageInfo_new(0, 0, 0, 1), &sender_badge);'
)

with open(main_path, 'w') as f:
    f.write(code)

# 2. Patch app.c
app_path = '/data/data/com.termux/files/home/seL4-workspace/libraries-3/app.c'
with open(app_path, 'r') as f:
    app_code = f.read()

# Task 8: Call endpoint
app_code = app_code.replace(
    '    seL4_CPtr ep = (seL4_CPtr) atol(argv[0]);',
    '    seL4_CPtr ep = (seL4_CPtr) atol(argv[0]);\n    tag = seL4_Call(ep, tag);'
)

with open(app_path, 'w') as f:
    f.write(app_code)

print("Applied libraries-3 patches to main.c and app.c.")
