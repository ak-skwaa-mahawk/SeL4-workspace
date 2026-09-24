path = '/data/data/com.termux/files/home/seL4-workspace/libraries-1/main.c'
with open(path, 'r') as f:
    code = f.read()

# Replace any definition of pd_cap with seL4_CapInitThreadVSpace
import re
code = re.sub(r'seL4_CPtr pd_cap\s*=\s*[^;]+;', 'seL4_CPtr pd_cap = seL4_CapInitThreadVSpace;', code)

# Ensure seL4_TCB_Configure uses seL4_CapInitThreadVSpace
code = re.sub(
    r'seL4_TCB_Configure\([^)]+\)',
    'seL4_TCB_Configure(tcb_object.cptr, seL4_CapNull, cspace_cap, seL4_NilData, seL4_CapInitThreadVSpace, seL4_NilData, 0, seL4_CapNull)',
    code
)

with open(path, 'w') as f:
    f.write(code)

print("VSpace root updated to seL4_CapInitThreadVSpace.")
