/**
 * @file kernel.c
 * @brief Core executive coordinator implementation for ESP32-S3.
 */

#include "kernel.h"
#include <string.h>
#if defined(ESP_PLATFORM)
#include "esp_attr.h"
#define MK_INTERNAL_DRAM DRAM_ATTR
#else
#define MK_INTERNAL_DRAM
#endif

#if defined(ESP_PLATFORM)
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#if defined(CONFIG_SPIRAM)
#include "esp_psram.h"
#endif
#endif

static MK_INTERNAL_DRAM mk_kernel_t s_kernel;
static bool s_booted = false;

mk_kernel_t *mk_kernel_get_instance(void) { return &s_kernel; }

mk_status_t mk_kernel_boot(void)
{
    memset(&s_kernel, 0, sizeof(s_kernel));
    s_kernel.state = MK_KERNEL_STATE_RESET;
    s_kernel.tick = 0U;
    s_kernel.scheduler_iterations = 0U;
    s_kernel.active_app = (uint8_t)MK_APP_BLE_HID;
    mk_status_t status = mk_kernel_transition(&s_kernel, MK_KERNEL_STATE_INIT);
    if (status != MK_STATUS_OK) return status;
    s_booted = true;
    return MK_STATUS_OK;
}

mk_status_t mk_kernel_init(void)
{
    if (!s_booted || s_kernel.state != MK_KERNEL_STATE_INIT) return MK_STATUS_INVALID_STATE;
    for (uint8_t i = 0; i < MK_APP_COUNT; i++) {
        s_kernel.apps[i].id = (mk_app_id_t)i;
        s_kernel.apps[i].state = MK_APP_STATE_OFF;
        s_kernel.apps[i].task_id = 0U;
        s_kernel.apps[i].name = "unregistered";
        s_kernel.apps[i].runs_completed = 0U;
        s_kernel.apps[i].error_count = 0U;
    }
    return mk_scheduler_init(&s_kernel);
}

mk_status_t mk_kernel_register_app(mk_app_id_t app_id, const char *name, uint8_t task_id)
{
    if (app_id >= MK_APP_COUNT || task_id >= MK_MAX_TASKS) return MK_STATUS_INVALID_ARG;
    mk_app_control_t *app = &s_kernel.apps[app_id];
    app->id = app_id;
    app->name = name;
    app->task_id = task_id;
    app->state = MK_APP_STATE_OFF;
    return mk_app_transition(&s_kernel, app_id, MK_APP_STATE_INIT);
}

mk_app_control_t *mk_kernel_get_app(mk_app_id_t app_id)
{
    if (app_id >= MK_APP_COUNT) return NULL;
    return &s_kernel.apps[app_id];
}

mk_status_t mk_kernel_run(void)
{
    if (s_kernel.state != MK_KERNEL_STATE_READY) return MK_STATUS_INVALID_STATE;
    mk_status_t status = mk_kernel_transition(&s_kernel, MK_KERNEL_STATE_RUNNING);
    if (status != MK_STATUS_OK) return status;
    while (s_kernel.state == MK_KERNEL_STATE_RUNNING) mk_scheduler_run_iteration(&s_kernel);
    return MK_STATUS_OK;
}

void mk_kernel_shutdown(void) { (void)mk_kernel_transition(&s_kernel, MK_KERNEL_STATE_SHUTDOWN); }
uint32_t mk_kernel_get_tick(void) { return s_kernel.tick; }

void mk_kernel_get_diag_identity(mk_diag_identity_t *out_identity)
{
    if (out_identity == NULL) return;
    memset(out_identity, 0, sizeof(*out_identity));
    strncpy(out_identity->product, "MicroKernel OS", sizeof(out_identity->product) - 1U);
    strncpy(out_identity->version, "0.1.0", sizeof(out_identity->version) - 1U);
    strncpy(out_identity->build_id, "20260904", sizeof(out_identity->build_id) - 1U);
    strncpy(out_identity->git_revision, "20260904-rev2", sizeof(out_identity->git_revision) - 1U);
#if defined(ESP_PLATFORM)
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    strncpy(out_identity->chip_family, "ESP32-S3", sizeof(out_identity->chip_family) - 1U);
    out_identity->chip_revision = chip_info.revision;
    uint32_t flash_size = 0U;
    if (esp_flash_get_size(NULL, &flash_size) == ESP_OK) out_identity->flash_size_bytes = flash_size;
    out_identity->internal_sram_total = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    out_identity->internal_sram_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
#if defined(CONFIG_SPIRAM)
    out_identity->psram_size_bytes = (uint32_t)esp_psram_get_size();
    out_identity->psram_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    out_identity->psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
#endif
#else
    strncpy(out_identity->chip_family, "HOST", sizeof(out_identity->chip_family) - 1U);
#endif
}
