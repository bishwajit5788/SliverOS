# Hardware Troubleshooting Guide

This guide addresses common hardware, serial, and flashing issues when developing with MicroKernel OS.

---

## 1. Web Serial Port Issues

### "Web Serial API is not supported in this browser"
- **Cause**: Browser does not support Web Serial (e.g. Safari, Firefox).
- **Resolution**: Use Google Chrome, Microsoft Edge, Opera, or Brave (v89+).

### No devices appear in the connection dialog
- **Check Cable**: Ensure the USB cable supports data transfer. Many micro-USB and USB-C cables are charge-only.
- **Check USB Drivers**:
  - Silicon Labs CP210x (ESP32 DevKit v1): Install CP210x VCP driver.
  - WCH CH340 / CH9102 (NodeMCU / clones): Install CH34x driver.
- **USB Hubs**: Unplug from unpowered USB hubs and plug directly into the computer.

---

## 2. Bootloader Synchronization Failures

### "Failed to synchronize with ESP ROM bootloader"
- **Cause**: The auto-reset circuit failed to pull GPIO 0 low while pulsing EN.
- **Resolution**:
  1. Hold down the **BOOT** (or **IO0**) button on the ESP32 board.
  2. Momentarily click **EN** (or **RST**).
  3. Release the **BOOT** button.
  4. Click **CONNECT DEVICE** in the browser flasher.

### Connection drops during transfer
- **Cause**: Insufficient USB power supply or high baudrate over poor wiring.
- **Resolution**: Lower the baudrate in the selector dropdown from `460,800` to `115,200`.

---

## 3. Runtime Kernel Faults

### `MK_FAULT_SCHEDULER_OVERRUN` in Developer Console
- **Cause**: A task executed longer than `MK_TASK_EXEC_BUDGET_TICKS` (50 ticks).
- **Resolution**: Ensure tasks execute small bounded steps and return control. Do not insert long delays or loops inside cooperative tasks.

### `MK_FAULT_MEMORY_EXHAUSTED`
- **Cause**: Total static arena allocations exceeded 128KB.
- **Resolution**: Inspect active buffer allocations using `mk_memory_get_stats()`. Ensure all allocated buffers are released with `my_free()`.

### `MK_FAULT_MEMORY_DOUBLE_FREE`
- **Cause**: An application called `my_free()` on an address that was already freed.
- **Resolution**: Trace pointer lifecycle in the offending application and set freed pointers to `NULL`.
