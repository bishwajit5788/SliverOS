/**
 * @file network_diagnostics.h
 * @brief Authorized local-network diagnostic state machine application.
 * Non-blocking cooperative auditing of ICMP and ports 22, 80, 443.
 */

#ifndef MK_APP_NETWORK_DIAGNOSTICS_H
#define MK_APP_NETWORK_DIAGNOSTICS_H

#include "kernel_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NET_DIAG_IDLE = 0,
    NET_DIAG_TARGET_CONFIGURED,
    NET_DIAG_ICMP_PENDING,
    NET_DIAG_ICMP_RESULT,
    NET_DIAG_TCP_22_PENDING,
    NET_DIAG_TCP_22_RESULT,
    NET_DIAG_TCP_80_PENDING,
    NET_DIAG_TCP_80_RESULT,
    NET_DIAG_TCP_443_PENDING,
    NET_DIAG_TCP_443_RESULT,
    NET_DIAG_COMPLETE
} net_diag_state_t;

typedef struct {
    char target_ip[48];
    bool target_configured;
    bool icmp_reachable;
    uint32_t icmp_rtt_ms;
    bool tcp_22_open;
    bool tcp_80_open;
    bool tcp_443_open;
    net_diag_state_t current_state;
} net_diag_report_t;

mk_status_t network_diagnostics_init(void);
mk_status_t network_diagnostics_start(void);
mk_status_t network_diagnostics_stop(void);
mk_status_t network_diagnostics_reset(void);
mk_status_t network_diagnostics_set_target(const char *target_ip);
void network_diagnostics_task(void *context);
void network_diagnostics_get_report(net_diag_report_t *out_report);
const mk_app_interface_t *network_diagnostics_get_interface(void);

#ifdef __cplusplus
}
#endif

#endif /* MK_APP_NETWORK_DIAGNOSTICS_H */
