# ESP32-S3 Memory Architecture & Allocator Specification

## 1. Physical Memory Architecture: Internal SRAM vs External PSRAM

The ESP32-S3-DevKitC-1 hardware integrates both internal SRAM and external SPI PSRAM:

| Memory Domain | Address Range | Total Size | Access Speed | Role in MicroKernel OS |
|---|---|---|---|---|
| **Internal SRAM** | `0x3FC80000` – `0x3FD00000` | 512 KB | Zero wait-states (~240 MHz) | **CRITICAL KERNEL STRUCTURES ONLY**<br>- 128KB Static Arena (`s_arena_buffer`)<br>- Fixed Pools (`s_pool_buf_0..4`)<br>- Task Control Blocks (`s_kernel.tasks`)<br>- Fault Ring Buffer (`s_fault_ring`)<br>- Kernel Event Bus (`s_queue`)<br>- FreeRTOS Executive Stack |
| **External PSRAM** | `0x3C000000` – `0x3C800000` | 8 MB | SPI/OPI Bus (~80 MHz) | **NON-CRITICAL BULK DATA ONLY**<br>- High-Resolution Framebuffers (ST7789)<br>- Game Sprite Asset Sheets<br>- Audio Waveform Buffers<br>- Non-Critical Application Caches |

---

## 2. Linker & Symbol Address Verification Method

> [!IMPORTANT]
> **Capability Flags Alone Are Not Sufficient Proof**
> 
> Capability flags (`MALLOC_CAP_INTERNAL`) can be bypassed or misconfigured. MicroKernel OS guarantees internal SRAM placement by allocating all critical data structures statically in the **`.bss` segment**, which the ESP-IDF linker script maps directly to Internal Data RAM:

```text
Section .bss / .sbss:
  Mapped to: DRAM_SEG (Internal Data RAM, address range 0x3FC80000 - 0x3FD00000)
  Proof:
    s_arena_buffer:  &s_arena_buffer = 0x3FC8xxxx (Inside Internal Data RAM)
    s_kernel:        &s_kernel       = 0x3FC9xxxx (Inside Internal Data RAM)
    s_queue:         &s_queue        = 0x3FC9xxxx (Inside Internal Data RAM)
    s_fault_ring:    &s_fault_ring   = 0x3FC9xxxx (Inside Internal Data RAM)
    s_pool_buf_0..4: &s_pool_buf_x   = 0x3FC9xxxx (Inside Internal Data RAM)
```

At boot time, `print_diagnostic_banner()` in `main.c` explicitly queries:
- `heap_caps_get_total_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)`
- `heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)`
- `esp_psram_get_size()`

---

## 3. Internal SRAM Budget Breakdown

```text
512 KB Total Internal SRAM
├── 64 KB : IRAM (Interrupt Service Routines, CPU Vectors, Flash Cache)
├── 64 KB : Hardware Cache Tag Memory & Silicon ROM Reservoirs
├── 160 KB: ESP-IDF Runtime (FreeRTOS Executive, Wi-Fi MAC/PHY, NimBLE Stack, TWDT)
├── 128 KB: MicroKernel OS Static Arena (my_malloc)
├──  12 KB: Fixed Memory Pools (16B, 32B, 64B, 128B, 256B)
├──   8 KB: Kernel Control Block (8 TCBs, Event Bus Queue, Fault Ring Buffer)
└──  76 KB: System Heap Safety Margin & DMA Descriptors
```

---

## 4. The 128KB Static Arena Allocator (`memory_manager.c`)

- **Buffer**: Statically allocated `uint8_t s_arena_buffer[MK_ARENA_SIZE]` aligned to 8 bytes.
- **Canaries**: Header magic `0x55AA55AA` (Allocated) and `0xAA55AA55` (Free).
- **Search Complexity**: First-Fit traversal: **O(N)** worst-case latency. Variable-time search; does **not** claim hard-real-time sub-microsecond bounds.
- **Coalescing**: Physical boundary pointers enable **O(1)** constant-time coalescing on `my_free()`.
- **Double-Free & Corrupted Header Traps**: Immediate fault generation (`MK_FAULT_MEMORY_DOUBLE_FREE`, `MK_FAULT_MEMORY_CORRUPTION`).
- **ISR Rule**: Dynamic allocation from hardware interrupt context is strictly prohibited.

---

## 5. Fixed-Size Memory Pools (`memory_pool.c`)

Provides guaranteed **O(1)** constant-time allocation and deallocation for small, high-frequency objects:

| Pool Class | Block Size | Count | Dedicated Storage | Intended Purpose |
|:---:|:---:|:---:|:---:|---|
| **Class 0** | 16 bytes | 32 blocks | 512 B | Small timer events, status flags |
| **Class 1** | 32 bytes | 32 blocks | 1,024 B | GPIO inputs, fault snapshots |
| **Class 2** | 64 bytes | 32 blocks | 2,048 B | Kernel Event Bus records (`mk_event_t`) |
| **Class 3** | 128 bytes | 16 blocks | 2,048 B | VFS directory entries, BLE HID reports |
| **Class 4** | 256 bytes | 8 blocks | 2,048 B | Log message lines, network diagnostic states |

- **Mechanism**: Singly-linked free-list embedded inside static internal SRAM headers (`mk_pool_node_t`).
- **Observable Metrics**: `peak_allocated`, `allocated_blocks`, `allocation_failures`.
