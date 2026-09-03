# ESP32-S3 Memory Architecture & Allocator Specification

## 1. Physical Memory Reality: Internal SRAM vs External PSRAM

The ESP32-S3-DevKitC-1 hardware integrates both internal SRAM and external SPI PSRAM:

| Memory Domain | Total Size | Access Speed | Bus Interface | Role in MicroKernel OS |
|---|---|---|---|---|
| **Internal SRAM** | 512 KB | Zero wait-states (~240 MHz) | Internal Direct Bus | **CRITICAL KERNEL STRUCTURES ONLY**<br>- 128KB Static Arena (`my_malloc`)<br>- Fixed-Size Memory Pools (16B–256B)<br>- Task Control Blocks (TCBs)<br>- Fault Ring Buffer<br>- Kernel Event Bus Queue<br>- Interrupt Stacks |
| **External PSRAM** | 8 MB | SPI/OPI Bus (~80 MHz) | Octal SPI with cache | **NON-CRITICAL BULK DATA ONLY**<br>- Graphical Display Framebuffers<br>- Game Sprites & Tile Maps<br>- Temporary Audio Buffers<br>- Large Non-Critical Caches |

> [!CAUTION]
> **Strict Architectural Separation Rule**:
> Critical scheduler state, task control blocks, interrupt infrastructure, fault logs, event queues, and timing-critical kernel structures **MUST NEVER** be placed in PSRAM. PSRAM access is cached and subject to bus arbitration latency; accessing PSRAM during flash writes or cache-miss states causes timing jitter that violates real-time deadlines.

---

## 2. Internal SRAM Linker Breakdown

Although ESP32-S3 silicon has 512 KB of physical SRAM, not all of it is freely available to user applications:
- **~64 KB**: Dedicated to IRAM (Interrupt handlers, timing-critical routines, cache management).
- **~64 KB**: Reserved for ROM bootloader and hardware cache tag memory.
- **~160 KB**: Consumed by ESP-IDF runtime services, Wi-Fi MAC/PHY driver buffers, Bluetooth NimBLE stack, and FreeRTOS idle task stacks.
- **128 KB**: Allocated to the MicroKernel Static Arena (`#define MK_ARENA_SIZE (128U * 1024U)`).
- **~8 KB**: Allocated to Fixed-Size Kernel Memory Pools (`os/kernel/memory_pool.c`).
- **Remaining ~88 KB**: Reserved for system heap margin and driver DMA descriptors.

---

## 3. The 128KB Static Arena Allocator (`memory_manager.c`)

The general-purpose allocator operates entirely within a static BSS buffer in internal SRAM:
```c
static uint8_t s_arena_buffer[MK_ARENA_SIZE] __attribute__((aligned(8)));
```

### Key Properties:
- **Zero Libc Calls**: Does not call libc `malloc()`, `free()`, `realloc()`, or `calloc()`.
- **Search Complexity**: First-Fit linear traversal with **O(N)** worst-case latency where $N$ is the number of blocks. It is variable-time and does **not** guarantee hard-real-time sub-microsecond latency.
- **Coalescing**: Bidirectional boundary-tag coalescing in **O(1)** constant time.
- **Safety Canaries**: Header magic tags (`0x55AA55AA` for allocated, `0xAA55AA55` for free).
- **Defenses**: Rejects double-free attempts, out-of-bounds pointers, and misaligned addresses.
- **Prohibition**: Allocation from ISR or hardware interrupt context is strictly barred.

---

## 4. Fixed-Size Memory Pools (`memory_pool.c`)

For critical short-lived objects requiring deterministic, constant-time allocation, MicroKernel OS provides a dedicated pool allocator:

| Class | Block Size | Block Count | Primary Use |
|:---:|:---:|:---:|---|
| **Class 0** | 16 bytes | 32 blocks | Small timer signals, state flags |
| **Class 1** | 32 bytes | 32 blocks | GPIO events, fault context snapshots |
| **Class 2** | 64 bytes | 32 blocks | Kernel Event Bus records (`mk_event_t`) |
| **Class 3** | 128 bytes | 16 blocks | Network audit logs, BLE HID report sequences |
| **Class 4** | 256 bytes | 8 blocks | VFS file descriptor buffers, diagnostics |

- **Latency Guarantee**: Strictly **O(1)** constant time for both `mk_pool_alloc()` and `mk_pool_free()`.
- **Mechanism**: Singly-linked free-list within static internal SRAM arrays.
