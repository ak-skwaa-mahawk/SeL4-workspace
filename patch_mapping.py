path = '/data/data/com.termux/files/home/seL4-workspace/mapping/src/main.c'
with open(path, 'r') as f:
    code = f.read()

# Map PD
code = code.replace(
    '    // TODO map a page directory object',
    '    error = seL4_X86_PageDirectory_Map(pd, seL4_CapInitThreadVSpace, TEST_VADDR, seL4_X86_Default_VMAttributes);\n    ZF_LOGF_IF(error != seL4_NoError, "Failed to map page directory");'
)

# Map PT
code = code.replace(
    '    // TODO map a page table object',
    '    error = seL4_X86_PageTable_Map(pt, seL4_CapInitThreadVSpace, TEST_VADDR, seL4_X86_Default_VMAttributes);\n    ZF_LOGF_IF(error != seL4_NoError, "Failed to map page table");'
)

# Remap page with read/write rights
code = code.replace(
    '    // TODO remap the page',
    '    error = seL4_X86_Page_Map(frame, seL4_CapInitThreadVSpace, TEST_VADDR, seL4_AllRights, seL4_X86_Default_VMAttributes);\n    ZF_LOGF_IF(error != seL4_NoError, "Failed to remap page");'
)

with open(path, 'w') as f:
    f.write(code)

print("Patch applied to mapping/src/main.c")
