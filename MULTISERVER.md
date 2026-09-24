# Native seL4 Multi-Server Architecture (Non-CapDL)

Built and executed natively inside Android Termux (ARM64) targeting `x86_64` (pc99) in QEMU.

## Architecture

+--------------------------------+
|    Root Server (Init/Broker)   |
|  - Bootstraps allocman & vka   |
|  - Bootstraps root vspace      |
|  - Spawns compute_srv & client |
|  - Distributes Badged Caps     |
+---------------+----------------+
|
+---------------------+---------------------+
|                                           |
v                                           v
+-----------------------+                   +-----------------------+
|     compute_srv       |                   |        client         |
|  - Isolated CSpace    | <=== IPC Chan === |  - Isolated CSpace    |
|  - Raw Endpoint       |  (Badge 0xC001)   |  - Badged Endpoint    |
|  - OP_ADD / OP_NOT    |                   |  - RPC Invoker        |
+-----------------------+                   +-----------------------+


## Primitives Used
- **IPC Mechanism**: Synchronous Microkernel IPC via `seL4_Recv` / `seL4_ReplyRecv` (server) and `seL4_Call` (client).
- **Access Control**: Capability minting via `sel4utils_mint_cap_to_process` providing badge `0xC001` to the client.
- **Packaging**: Dual ELF embedding into a unified CPIO archive via `MakeCPIO`.
- **Process Spawning**: `libsel4utils` process configuration and VSpace creation.
- **Hardware Simulation**: QEMU pc99 with ELF program header patching hook.
