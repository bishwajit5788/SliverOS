/**
 * @file wifi_diagnostics.c
 * @brief Passive Wi-Fi diagnostic application implementation.
 */

#include "wifi_diagnostics.h"
#include "hal_wifi.h"
#include "vfs_log.h"
#include "kernel.h"
#include <stdio.h>
#include <string.h>

static wifi_diag_stats_t s_stats;
static uint32_t s_last_hop_tick = 0U;
static uint32_t s_last_log_tick = 0U;
static bool s_active = false;

mk_status_t wifi_diagnostics_init(void)
{
    memset(&s_stats, 0, sizeof(s_stats));
    s_stats.active_channel = 1U;
    s_last_hop_tick = 0U;
    s_last_log_tick = 0U;
    s_active = false;
    return MK_STATUS_OK;
}

mk_status_t wifi_diagnostics_start(void)
{
    s_active = true;
    return hal_wifi_start_promiscuous(s_stats.active_channel);
}

mk_status_t wifi_diagnostics_stop(void)
{
    s_active = false;
    return hal_wifi_stop_promiscuous();
}

void wifi_diagnostics_task(void *context)
{
    (void)context;
    if (!s_active) {
        return;
    }

    uint32_t current_tick = mk_kernel_get_tick();

    /* Drain up to 8 frames per cooperative task slice */
    hal_wifi_frame_meta_t meta;
    uint32_t processed = 0U;
    int32_t rssi_sum = 0;

    while (processed < 8U && hal_wifi_queue_pop(&meta)) {
        s_stats.total_frames_analyzed++;
        rssi_sum += meta.rssi;

        switch (meta.frame_type) {
            case WIFI_FRAME_TYPE_MGMT:
                s_stats.mgmt_frames++;
                if (meta.frame_subtype == 8U) {
                    s_stats.beacon_frames++;
                } else if (meta.frame_subtype == 4U || meta.frame_subtype == 5U) {
                    s_stats.probe_frames++;
                }
                break;
            case WIFI_FRAME_TYPE_CTRL:
                s_stats.ctrl_frames++;
                break;
            case WIFI_FRAME_TYPE_DATA:
                s_stats.data_frames++;
                break;
            default:
                break;
        }
        processed++;
    }

    if (processed > 0) {
        s_stats.avg_rssi = (s_stats.avg_rssi + (rssi_sum / (int32_t)processed)) / 2;
    }

    /* Channel hopping every 50 ticks */
    if ((current_tick - s_last_hop_tick) >= 50U) {
        s_stats.active_channel = (s_stats.active_channel % 11U) + 1U;
        (void)hal_wifi_set_channel(s_stats.active_channel);
        s_last_hop_tick = current_tick;
    }

    /* Periodic VFS Audit log every 100 ticks */
    if ((current_tick - s_last_log_tick) >= 100U) {
        char log_buf[128];
        snprintf(log_buf, sizeof(log_buf),
                 "[WIFI_AUDIT] Ch:%u Tot:%u Mgmt:%u (Bcn:%u,Prb:%u) Ctrl:%u Data:%u RSSI:%d\n",
                 s_stats.active_channel, s_stats.total_frames_analyzed,
                 s_stats.mgmt_frames, s_stats.beacon_frames, s_stats.probe_frames,
                 s_stats.ctrl_frames, s_stats.data_frames, s_stats.avg_rssi);
        (void)vfs_log_append("wifi_audit.log", log_buf);
        s_last_log_tick = current_tick;
    }
}

void wifi_diagnostics_get_stats(wifi_diag_stats_t *out_stats)
{
    if (out_stats != NULL) {
        *out_stats = s_stats;
    }
}
