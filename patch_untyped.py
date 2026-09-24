import re

path = '/data/data/com.termux/files/home/seL4-workspace/untyped/src/main.c'
with open(path, 'r') as f:
    code = f.read()

# Replace the untyped size calculation and selection
pattern1 = r'// TODO work out what size object we need to create.*?break;\s*\}\s*\}'
replacement1 = """seL4_Word total_size = 0;
    for (int i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        total_size += BIT(sizes[i]);
    }
    seL4_Word untyped_size_bits = seL4_TCBBits + 1;
    while (BIT(untyped_size_bits) < total_size) {
        untyped_size_bits++;
    }

    seL4_CPtr parent_untyped = 0;
    seL4_CPtr child_untyped = info->empty.start;

    for (int i = 0; i < (info->untyped.end - info->untyped.start); i++) {
        if (!info->untypedList[i].isDevice && info->untypedList[i].sizeBits >= untyped_size_bits) {
            parent_untyped = info->untyped.start + i;
            break;
        }
    }"""

code = re.sub(pattern1, replacement1, code, flags=re.DOTALL)

# Replace TODO tasks
code = code.replace(
    '/* TODO create a TCB in CSlot child_tcb */',
    'error = seL4_Untyped_Retype(child_untyped, seL4_TCBObject, seL4_TCBBits, seL4_CapInitThreadCNode, 0, 0, child_tcb, 1);\n    ZF_LOGF_IF(error != seL4_NoError, "Failed to create TCB");'
)

code = code.replace(
    '/* TODO create an endpoint in CSlot child_ep */',
    'error = seL4_Untyped_Retype(child_untyped, seL4_EndpointObject, seL4_EndpointBits, seL4_CapInitThreadCNode, 0, 0, child_ep, 1);\n    ZF_LOGF_IF(error != seL4_NoError, "Failed to create Endpoint");'
)

code = code.replace(
    '// TODO create a notification object in CSlot child_ntfn',
    'error = seL4_Untyped_Retype(child_untyped, seL4_NotificationObject, seL4_NotificationBits, seL4_CapInitThreadCNode, 0, 0, child_ntfn, 1);\n    ZF_LOGF_IF(error != seL4_NoError, "Failed to create Notification");'
)

code = code.replace(
    '// TODO revoke the child untyped',
    'error = seL4_CNode_Revoke(seL4_CapInitThreadCNode, child_untyped, 64);\n    ZF_LOGF_IF(error != seL4_NoError, "Failed to revoke child untyped");'
)

with open(path, 'w') as f:
    f.write(code)

print("Patch applied successfully.")
