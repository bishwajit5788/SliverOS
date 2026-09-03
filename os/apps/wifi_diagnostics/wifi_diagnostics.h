/**
 * @file wifi_diagnostics.h
 * @brief Passive Wi-Fi diagnostic application.
 * Collects safe frame statistics and writes audit records to VFS.
 */

#ifndef MK_APP_WIFI_DIAGNOSTICS_H
#define MK_APP_WIFI_DIAGNOSTICS_H

#include "kernel_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t total_frames_analyzed;
    uint32_t mgmt_frames;
    uint32_t ctrl_frames;
    uint32_t data_frames;
    uint32_t beacon_frames;
    uint32_t probe_frames;
    int32_t avg_rssi;
    uint8_t active_channel;
} wifi_diag_stats_t;

mk_status_t wifi_diagnostics_init(void);
mk_status_t wifi_diagnostics_start(void);
mk_status_t wifi_diagnostics_stop(void);
void wifi_diagnostics_task(void *context);
void wifi_diagnostics_get_stats(wifi_diag_stats_t *out_stats);

#ifdef __cplusplus
}
#endif

#endif /* MK_APP_WIFI_DIAGNOSTICS_H */
