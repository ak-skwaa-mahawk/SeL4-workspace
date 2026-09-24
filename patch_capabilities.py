path = '/data/data/com.termux/files/home/seL4-workspace/capabilities/src/main.c'
with open(path, 'r') as f:
    code = f.read()

# 1. Calculate CNode size in bytes
old_size = 'size_t initial_cnode_object_size_bytes = 0; // TODO calculate this.'
new_size = 'size_t initial_cnode_object_size_bytes = initial_cnode_object_size * (1ULL << seL4_SlotBits);'
code = code.replace(old_size, new_size)

# 2. Copy TCB to last_slot
old_copy = '/* TODO use seL4_CNode_Copy to make another copy of the initial TCB capability to the last slot in the CSpace */'
new_copy = '''error = seL4_CNode_Copy(seL4_CapInitThreadCNode, last_slot, seL4_WordBits,
                           seL4_CapInitThreadCNode, seL4_CapInitThreadTCB, seL4_WordBits,
                           seL4_AllRights);
    ZF_LOGF_IF(error, "Failed to copy cap to last_slot!");'''
code = code.replace(old_copy, new_copy)

# 3. Delete created TCB capabilities
old_del = '// TODO delete the created TCB capabilities'
new_del = '''error = seL4_CNode_Delete(seL4_CapInitThreadCNode, first_free_slot, seL4_WordBits);
    ZF_LOGF_IF(error, "Failed to delete first_free_slot cap!");

    error = seL4_CNode_Delete(seL4_CapInitThreadCNode, last_slot, seL4_WordBits);
    ZF_LOGF_IF(error, "Failed to delete last_slot cap!");'''
code = code.replace(old_del, new_del)

# 4. Suspend root thread
old_susp = '// TODO suspend the current thread'
new_susp = 'seL4_TCB_Suspend(seL4_CapInitThreadTCB);'
code = code.replace(old_susp, new_susp)

with open(path, 'w') as f:
    f.write(code)

print("Applied capabilities tutorial solution.")
