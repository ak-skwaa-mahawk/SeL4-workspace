path = '/data/data/com.termux/files/home/seL4-workspace/libraries-1/main.c'
with open(path, 'r') as f:
    code = f.read()

# Fix stack pointer for x86_64 ABI
code = code.replace(
    'sel4utils_set_stack_pointer(&regs, (seL4_Word)thread_2_stack_top);',
    'sel4utils_set_stack_pointer(&regs, (seL4_Word)thread_2_stack_top - 8);'
)

# Prevent root thread from exiting immediately
code = code.replace(
    '    /* we are done, say hello */\n    printf("main: hello world\\n");\n    return 0;',
    '    /* we are done, say hello */\n    printf("main: hello world\\n");\n    while (1) { seL4_Yield(); }\n    return 0;'
)

with open(path, 'w') as f:
    f.write(code)

print("Applied thread_2 ABI and yield patch.")
