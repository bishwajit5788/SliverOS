# Interrupt Handling & Callback Architecture Guidelines

## Core Philosophy

In MicroKernel OS, the Single FreeRTOS Executive thread executes all application logic, filesystem I/O, network stacks, parsing, and rendering. Hardware interrupts (ISRs) and low-level driver callbacks must strictly behave as **producers of minimal event signals**, never consumers or processors.

---

## The Golden ISR Pattern

```text
Hardware Interrupt / Driver Callback
               │
               ▼
   [Bounded Minimal Work]
   (e.g., Read Register / Copy 10-byte Metadata)
               │
               ▼
   [Post to Event Queue or Ring Buffer]
   (Non-blocking, O(1), no heap allocation)
               │
               ▼
   [Return from Interrupt Immediately]
```

---

## Strict Prohibitions Inside ISRs & Callbacks

The following actions are strictly prohibited inside any ISR or driver callback context:
1. **NO Dynamic Allocation**: Never invoke `malloc()`, `free()`, `my_malloc()`, `my_free()`, or pool allocators.
2. **NO Filesystem Operations**: Never call `vfs_open()`, `vfs_read()`, `vfs_write()`, or flash erase/program routines.
3. **NO Blocking / Delays**: Never call `vTaskDelay()`, `usleep()`, `hal_timer_delay_ms()`, or wait on mutexes/semaphores.
4. **NO Complex Parsing**: Never parse 802.11 payloads, HTTP headers, JSON, or macro DSL scripts.
5. **NO Graphical Rendering**: Never draw pixels or execute SPI/I2C display transfers.
6. **NO Protocol Engine Processing**: Never execute Wi-Fi connection logic, TCP handshakes, or BLE GATT procedures.

---

## Subsystem Rules Matrix

| Subsystem | Context Type | Permitted Work | Target Buffer |
|---|---|---|---|
| **GPIO Inputs** | Hardware ISR | Sample pin level, filter hardware glitches | Event Bus (`MK_EVENT_GPIO`) |
| **Timer** | Hardware Timer ISR | Increment monotonic counter, set wake flags | Event Bus (`MK_EVENT_TIMER`) |
| **Wi-Fi Promiscuous** | Driver RX Callback | Extract 10-byte metadata struct (channel, RSSI, length, type/subtype) | Diagnostic Queue (`hal_wifi_frame_meta_t`) |
| **BLE Radio** | GAP/GATT Callback | Update connection boolean, notify event | Event Bus (`MK_EVENT_BLE_CONNECTED`) |
| **Native USB** | USB Serial CDC Callback | Enqueue raw UART bytes into ring buffer | Input Stream Buffer |
| **SPI DMA** | Transfer Done ISR | Clear bus busy flag, trigger dirty box reset | Display Manager State |

---

## Overflow Policy

If a ring buffer or event queue is full when an ISR attempts to post:
- The event is dropped immediately.
- A static drop counter is incremented.
- The ISR does **NOT** retry or spin-wait.
- The next scheduler cycle logs the overflow fault to the Fault Manager.
