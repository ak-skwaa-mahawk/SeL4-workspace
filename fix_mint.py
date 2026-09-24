path = '/data/data/com.termux/files/home/seL4-workspace/libraries-2/main.c'
with open(path, 'r') as f:
    code = f.read()

# Insert the vka_mint_object call right before the error check
target = '    ZF_LOGF_IFERR(error, "Failed to mint new badged copy of IPC endpoint.'
replacement = '''    error = vka_mint_object(&vka, &ep_object, &ep_cap_path, seL4_AllRights, EP_BADGE);
    ZF_LOGF_IFERR(error, "Failed to mint new badged copy of IPC endpoint.'''

code = code.replace(target, replacement)

with open(path, 'w') as f:
    f.write(code)

print("Inserted vka_mint_object call.")
