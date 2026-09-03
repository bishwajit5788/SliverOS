/**
 * @file main.c
 * @brief Primary boot orchestrator for MicroKernel OS on ESP32-S3-DevKitC-1.
 * Pure startup sequence orchestrator; zero application logic.
 */

#include <stdio.h>
#include "kernel.h"
#include "memory_pool.h"
#include "event_bus.h"
#include "hal.h"
#include "vfs.h"
#include "ui/ui_runtime.h"
#include "ble_hid.h"
#include "wifi_diagnostics.h"
#include "network_diagnostics.h"
#include "retro_games.h"

#define TASK_ID_UI_RUNTIME  0U
#define TASK_ID_BLE_HID     1U
#define TASK_ID_WIFI_DIAG   2U
#define TASK_ID_NET_DIAG    3U
#define TASK_ID_GAMES       4U

static void print_diagnostic_banner(void)
{
    mk_diag_identity_t diag;
    mk_kernel_get_diag_identity(&diag);

    printf("\n");
    printf("====================================================\n");
    printf("        MicroKernel OS - ESP32-S3 Executive         \n");
    printf("====================================================\n");
    printf("  Target Platform:     %s (DevKitC-1 Primary)\n", diag.chip_family);
    printf("  Silicon Revision:    %u\n", diag.chip_revision);
    printf("  SPI Flash Capacity:  %u MB\n", diag.flash_size_bytes / (1024U * 1024U));
    printf("  External PSRAM:      %u MB\n", diag.psram_size_bytes / (1024U * 1024U));
    printf("  Internal SRAM Total: %u KB (Free: %u KB)\n",
           (unsigned)(diag.internal_sram_total / 1024U), (unsigned)(diag.internal_sram_free / 1024U));
    printf("  Static Arena Heap:   %u KB (Internal SRAM Only)\n", MK_ARENA_SIZE / 1024U);
    printf("  Fixed Memory Pools:  16B, 32B, 64B, 128B, 256B (Internal SRAM)\n");
    printf("  Firmware Version:    %s\n", diag.version);
    printf("  Build Identifier:    %s (%s)\n", diag.build_id, diag.git_revision);
    printf("  Cooperative Tasks:   5 / %u\n", MK_MAX_TASKS);
    printf("====================================================\n\n");
}

void app_main(void)
{
    mk_status_t status;
    mk_kernel_t *kernel;

    /* 1. ESP-IDF Startup -> Kernel Boot */
    status = mk_kernel_boot();
    if (status != MK_STATUS_OK) {
        printf("[FATAL] Kernel boot failed: %d\n", status);
        return;
    }

    /* 2. Initialize 128KB Static Arena Allocator in Internal SRAM */
    status = mk_memory_init();
    if (status != MK_STATUS_OK) {
        printf("[FATAL] Static arena memory manager init failed: %d\n", status);
        return;
    }

    /* 3. Initialize Fixed-Size Memory Pools (O(1) allocation for events & messages) */
    status = mk_pool_init();
    if (status != MK_STATUS_OK) {
        printf("[FATAL] Fixed memory pool init failed: %d\n", status);
        return;
    }

    /* 4. Initialize Ring-Buffer Fault Manager */
    status = mk_fault_manager_init();
    if (status != MK_STATUS_OK) {
        printf("[FATAL] Fault manager initialization failed: %d\n", status);
        return;
    }

    /* 5. Initialize Kernel Event Bus */
    status = mk_event_bus_init();
    if (status != MK_STATUS_OK) {
        printf("[FATAL] Event bus initialization failed: %d\n", status);
        return;
    }

    /* 6. Executive Subsystem Initialization */
    status = mk_kernel_init();
    if (status != MK_STATUS_OK) {
        printf("[FATAL] Kernel init failed: %d\n", status);
        return;
    }
    kernel = mk_kernel_get_instance();

    /* 7. Initialize Hardware Abstraction Layer */
    status = hal_init();
    if (status != MK_STATUS_OK) {
        printf("[WARN] HAL initialized with non-critical warnings: %d\n", status);
    }

    /* 8. Mount OS VFS over 'osfs' flash partition (with power-loss recovery) */
    status = vfs_init();
    if (status != MK_STATUS_OK) {
        printf("[WARN] VFS mount failed (%d); operating with in-memory fallback\n", status);
    }

    /* Print diagnostic system banner and memory map */
    print_diagnostic_banner();

    /* 9. Initialize Graphical OS Runtime & Display Manager */
    status = ui_runtime_init();
    if (status != MK_STATUS_OK) {
        printf("[WARN] UI Runtime initialized with fallback: %d\n", status);
    }

    /* 10. Initialize Application Modules */
    (void)ble_hid_init();
    (void)wifi_diagnostics_init();
    (void)network_diagnostics_init();
    (void)retro_games_init();

    /* 11. Register the 4 Applications into Kernel Control Block */
    (void)mk_kernel_register_app(MK_APP_BLE_HID, "BLE_HID", TASK_ID_BLE_HID);
    (void)mk_kernel_register_app(MK_APP_WIFI_DIAGNOSTICS, "WIFI_DIAG", TASK_ID_WIFI_DIAG);
    (void)mk_kernel_register_app(MK_APP_NETWORK_DIAGNOSTICS, "NET_DIAG", TASK_ID_NET_DIAG);
    (void)mk_kernel_register_app(MK_APP_RETRO_GAMES, "RETRO_GAMES", TASK_ID_GAMES);

    /* 12. Register Cooperative Tasks into Scheduler */
    (void)mk_task_register(kernel, TASK_ID_UI_RUNTIME, "ui_runtime",
                           ui_runtime_task, NULL, MK_TASK_PRIO_HIGH, 3U);

    (void)mk_task_register(kernel, TASK_ID_BLE_HID, "ble_hid",
                           ble_hid_task, NULL, MK_TASK_PRIO_HIGH, 1U);

    (void)mk_task_register(kernel, TASK_ID_WIFI_DIAG, "wifi_diag",
                           wifi_diagnostics_task, NULL, MK_TASK_PRIO_MEDIUM, 5U);

    (void)mk_task_register(kernel, TASK_ID_NET_DIAG, "net_diag",
                           network_diagnostics_task, NULL, MK_TASK_PRIO_LOW, 10U);

    (void)mk_task_register(kernel, TASK_ID_GAMES, "retro_games",
                           retro_games_task, NULL, MK_TASK_PRIO_LOWEST, 2U);

    /* Start Applications */
    (void)ble_hid_start();
    (void)wifi_diagnostics_start();
    (void)retro_games_start();

    (void)mk_app_transition(kernel, MK_APP_BLE_HID, MK_APP_STATE_READY);
    (void)mk_app_transition(kernel, MK_APP_WIFI_DIAGNOSTICS, MK_APP_STATE_READY);
    (void)mk_app_transition(kernel, MK_APP_NETWORK_DIAGNOSTICS, MK_APP_STATE_READY);
    (void)mk_app_transition(kernel, MK_APP_RETRO_GAMES, MK_APP_STATE_READY);

    /* 13. Transition Kernel State to READY */
    status = mk_kernel_transition(kernel, MK_KERNEL_STATE_READY);
    if (status != MK_STATUS_OK) {
        printf("[FATAL] Unable to transition kernel to READY: %d\n", status);
        return;
    }

    printf("[INFO] Kernel state is READY. Commencing cooperative executive execution...\n");

    /* 14. Run Kernel Cooperative Loop */
    (void)mk_kernel_run();
}
