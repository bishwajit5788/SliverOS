# Cooperative Scheduler Specification

## 1. Architectural Authority & The Single-Context Invariant

> [!IMPORTANT]
> **Strict Prohibition of Per-Application FreeRTOS Tasks**
> 
> The MicroKernel OS cooperative scheduler is the **sole application execution authority**:
> - There is **NO** FreeRTOS task per application.
> - There is **NO** FreeRTOS task per kernel component.
> - There is **NO** hidden RTOS scheduling replacing the custom scheduler.
> 
> **Execution Hierarchy**:
> ```text
> ESP-IDF Startup (ROM Bootloader)
>             │
>             ▼
>   Single FreeRTOS Thread Context (`app_main`)
>             │
>             ▼
>   `mk_kernel_boot()` ──► `mk_kernel_init()` ──► `mk_scheduler_init()`
>             │
>             ▼
>   `mk_kernel_run()`
>             │
>             ▼
>   Custom Cooperative Scheduler Loop (Owns all 4 applications)
>   ├── Task 0: UI Runtime / App Launcher (Prio 2, Period 3)
>   ├── Task 1: BLE-HID Macro Parser      (Prio 3, Period 1)
>   ├── Task 2: Wi-Fi Diagnostics         (Prio 4, Period 5)
>   ├── Task 3: Network Diagnostics       (Prio 6, Period 10)
>   └── Task 4: Retro Games Micro-Lander  (Prio 7, Period 2)
> ```
> FreeRTOS remains underneath ESP-IDF exclusively to host core platform drivers (Wi-Fi PHY, NimBLE controller, TWDT). Applications live and execute entirely inside the custom cooperative scheduler.

---

## 2. 3-Tier Scheduling Arbitration Algorithm

```text
               ┌────────────────────────┐
               │    Advance Timebase    │
               │      kernel->tick++    │
               └───────────┬────────────┘
                           │
                           ▼
          Tier 1: Priority Selection
          (Scan tasks for highest priority level, 0=highest)
                           │
                           ▼
          Tier 2: Period Eligibility Filter
          (Task must satisfy kernel->tick >= next_run_tick)
                           │
                           ▼
          Tier 3: Equal-Priority Round-Robin
          (Arbitrate amongst equal-priority candidates via rotating cursor)
                           │
                           ▼
             Dispatch Task Entry Routine
```

---

## 3. Task Control Block (TCB) Structure & Metrics

Every task is allocated in Internal SRAM with microsecond-level telemetry:

```c
typedef struct {
    uint8_t id;
    mk_task_state_t state;
    uint8_t priority;
    uint8_t padding;

    uint32_t period_ticks;
    uint32_t next_run_tick;

    mk_task_entry_t entry;
    void *context;

    /* Telemetry & Health Metrics */
    uint32_t execution_count;
    uint32_t fault_count;
    uint32_t max_execution_us;        /* Budget limit (default 25ms) */
    uint32_t last_execution_us;       /* Most recent runtime */
    uint32_t worst_execution_us;      /* High-water mark runtime */
    uint32_t deadline_miss_count;     /* Incremented when eligible before tick */
    uint32_t overrun_count;           /* Incremented when runtime > max_execution_us */

    char name[MK_TASK_NAME_LEN];
} mk_tcb_t;
```

---

## 4. Critical Distinction: Detection vs Preemption

> [!CAUTION]
> **Cooperative Reality: Detection $\ne$ Preemption**
> 
> Because this is a cooperative scheduler, the kernel cannot forcibly preempt a task while it is executing inside its C function call.
> - **Execution Timing**: The scheduler measures elapsed time between function entry and exit using `hal_timer_get_us()`.
> - **Detection**: If `last_execution_us > max_execution_us`, the scheduler records a `MK_FAULT_SCHEDULER_OVERRUN` fault and increments `overrun_count`.
> - **Enforcement & Recovery**: If a buggy task enters an infinite loop or blocks indefinitely, the cooperative loop is stalled. In this scenario, the hardware **Task Watchdog Timer (TWDT)** expires (configured to 5.0 seconds in `sdkconfig.defaults`) and triggers an automatic hardware reset to restore system availability.

---

## 5. Idle Behavior

When no tasks are eligible to run (e.g. all tasks are sleeping awaiting their next period):
1. The scheduler invokes `hal_timer_delay_ms(1)`.
2. Automatically kicks the hardware watchdog (`esp_task_wdt_reset()`).
3. Advances the tick counter and re-evaluates sleeping tasks.
