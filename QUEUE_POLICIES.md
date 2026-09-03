# Queue Overflow & Buffering Policies

## Overview

In a memory-constrained embedded system with no dynamic heap allocation during runtime, all queues, rings, and message buffers must have deterministic capacities, explicit ownership, copy semantics, and predefined overflow policies.

---

## Bounded Buffer Matrix

| Buffer Subsystem | Physical Location | Capacity | Record Size | Copy Semantics | Consumption Model | Overflow Policy | Observable Telemetry |
|---|---|:---:|:---:|---|---|---|---|
| **Kernel Event Bus** (`event_bus.c`) | Internal SRAM BSS | 64 events | 24 bytes (`mk_event_t`) | Full by-value copy | Single consumer FIFO pop | **Drop-Newest + Fault Escalation** | `queue_overflows`, `current_depth` |
| **Wi-Fi Promiscuous RX** (`hal_wifi.c`) | Internal SRAM BSS | 64 frames | 12 bytes (`hal_wifi_frame_meta_t`) | Shallow copy of 6 header fields | Cooperative batch drain (8/tick) | **Drop-Oldest (Circular FIFO)** | `s_dropped_frames` counter |
| **Fault Manager Ring** (`fault_manager.c`) | Internal SRAM BSS | 32 records | 28 bytes (`mk_fault_record_t`) | By-value copy | Circular review / dev screen | **Circular Overwrite Oldest** | `s_total_faults` monotonic count |

---

## Detailed Policy Specifications

### 1. Kernel Event Bus: Drop-Newest + Fault Escalation
- **Rationale**: The event bus carries inter-subsystem control signals (timer ticks, button presses, network completion). Overwriting historical unconsumed events could corrupt active state machines.
- **Behavior**:
  1. When `s_count == MK_EVENT_QUEUE_SIZE`, incoming `mk_event_bus_post()` immediately returns `MK_STATUS_QUEUE_FULL`.
  2. The new event is rejected and dropped.
  3. `s_stats.queue_overflows` is incremented.
  4. A fault of severity `MK_FAULT_SEV_WARNING` is logged to the Fault Manager (`source = MK_FAULT_SRC_SYSTEM`).

### 2. Wi-Fi Frame Metadata: Drop-Oldest (Circular Buffer)
- **Rationale**: In dense 2.4 GHz environments, hundreds of beacon and data packets arrive per second. Stale packets are of low diagnostic value compared to current channel conditions.
- **Behavior**:
  1. If the RX ring buffer is full when the ESP32 promiscuous callback fires, the tail pointer is advanced, dropping the oldest unread frame.
  2. The new frame is written at the head.
  3. `s_dropped_frames` counter is incremented.
  4. Zero dynamic allocation and zero blocking in the ISR.

### 3. Fault Manager: Circular Ring Overwrite
- **Rationale**: Diagnostic records must always capture the most recent crash or fault sequence leading up to a failure.
- **Behavior**:
  1. The 32-slot ring buffer writes continuously via `s_write_index % 32`.
  2. Total lifetime faults are tracked monotonically in `s_total_faults`.
  3. Developer diagnostics screen retrieves records in reverse chronological order via `mk_fault_get_at(offset)`.
