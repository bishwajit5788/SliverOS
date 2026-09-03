/**
 * @file network_diagnostics.c
 * @brief Non-blocking network diagnostics state machine implementation.
 */
#include "network_diagnostics.h"
#include "vfs_log.h"
#include "kernel.h"
#include <stdio.h>
#include <string.h>

#if defined(ESP_PLATFORM)
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/inet.h"
#include "lwip/ip_addr.h"
#include "ping/ping_sock.h"
#endif

static net_diag_report_t s_report;
static uint32_t s_step_timer = 0U;
#if defined(ESP_PLATFORM)
static esp_ping_handle_t s_ping_handle = NULL;
static bool s_ping_active = false;
static bool s_ping_success = false;
static void ping_on_success(esp_ping_handle_t hdl, void *args)
{
    (void)args;
    uint32_t elapsed_ms = 0U;
    (void)esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_ms, sizeof(elapsed_ms));
    s_report.icmp_reachable = true;
    s_report.icmp_rtt_ms = elapsed_ms;
    s_ping_success = true;
}
static void ping_on_timeout(esp_ping_handle_t hdl, void *args)
{
    (void)hdl; (void)args;
    s_report.icmp_reachable = false;
    s_report.icmp_rtt_ms = 0U;
    s_ping_success = false;
}
static void ping_on_end(esp_ping_handle_t hdl, void *args)
{
    (void)args;
    s_ping_active = false;
    s_report.icmp_reachable = s_ping_success;
    if (!s_ping_success) s_report.icmp_rtt_ms = 0U;
    (void)esp_ping_delete_session(hdl);
    s_ping_handle = NULL;
    s_report.current_state = NET_DIAG_ICMP_RESULT;
}
static mk_status_t start_async_ping(const char *target_ip)
{
    ip_addr_t target_addr;
    if (!ipaddr_aton(target_ip, &target_addr)) return MK_STATUS_INVALID_ARG;
    esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();
    config.target_addr = target_addr;
    config.count = 1U;
    config.interval_ms = 10U;
    config.timeout_ms = 1000U;
    esp_ping_callbacks_t callbacks = {
        .on_ping_success = ping_on_success,
        .on_ping_timeout = ping_on_timeout,
        .on_ping_end = ping_on_end,
        .cb_args = NULL
    };
    if (esp_ping_new_session(&config, &callbacks, &s_ping_handle) != ESP_OK) {
        s_ping_handle = NULL;
        return MK_STATUS_ERROR;
    }
    s_ping_active = true;
    s_ping_success = false;
    if (esp_ping_start(s_ping_handle) != ESP_OK) {
        (void)esp_ping_delete_session(s_ping_handle);
        s_ping_handle = NULL;
        s_ping_active = false;
        return MK_STATUS_ERROR;
    }
    return MK_STATUS_OK;
}
#endif

static bool check_tcp_nonblocking(const char *host, uint16_t port)
{
#if defined(ESP_PLATFORM)
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_addr.s_addr = inet_addr(host);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (s < 0) return false;
    int flags = fcntl(s, F_GETFL, 0);
    if (flags < 0 || fcntl(s, F_SETFL, flags | O_NONBLOCK) < 0) { close(s); return false; }
    int res = connect(s, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (res == 0) { close(s); return true; }
    fd_set fdset;
    FD_ZERO(&fdset); FD_SET(s, &fdset);
    struct timeval tv = { .tv_sec = 0, .tv_usec = 10000 };
    if (select(s + 1, NULL, &fdset, NULL, &tv) == 1) {
        int so_error = 0; socklen_t len = sizeof(so_error);
        (void)getsockopt(s, SOL_SOCKET, SO_ERROR, &so_error, &len);
        close(s); return so_error == 0;
    }
    close(s); return false;
#else
    (void)host; (void)port; return false;
#endif
}

mk_status_t network_diagnostics_init(void)
{
    memset(&s_report, 0, sizeof(s_report));
    s_report.current_state = NET_DIAG_IDLE;
    s_step_timer = 0U;
#if defined(ESP_PLATFORM)
    s_ping_handle = NULL; s_ping_active = false; s_ping_success = false;
#endif
    return network_diagnostics_set_target("127.0.0.1");
}

mk_status_t network_diagnostics_set_target(const char *target_ip)
{
    if (target_ip == NULL || strlen(target_ip) == 0U) return MK_STATUS_INVALID_ARG;
    strncpy(s_report.target_ip, target_ip, sizeof(s_report.target_ip) - 1U);
    s_report.target_ip[sizeof(s_report.target_ip) - 1U] = '\0';
    s_report.target_configured = true;
    s_report.icmp_reachable = false; s_report.icmp_rtt_ms = 0U;
    s_report.tcp_22_open = false; s_report.tcp_80_open = false; s_report.tcp_443_open = false;
    s_report.current_state = NET_DIAG_TARGET_CONFIGURED; s_step_timer = 0U;
    return MK_STATUS_OK;
}

void network_diagnostics_task(void *context)
{
    (void)context;
    switch (s_report.current_state) {
        case NET_DIAG_IDLE: break;
        case NET_DIAG_TARGET_CONFIGURED:
            s_step_timer = 0U; s_report.current_state = NET_DIAG_ICMP_PENDING; break;
        case NET_DIAG_ICMP_PENDING:
#if defined(ESP_PLATFORM)
            if (!s_ping_active) {
                if (start_async_ping(s_report.target_ip) != MK_STATUS_OK) {
                    s_report.icmp_reachable = false; s_report.icmp_rtt_ms = 0U;
                    s_report.current_state = NET_DIAG_ICMP_RESULT;
                }
            }
#else
            s_report.icmp_reachable = false; s_report.icmp_rtt_ms = 0U;
            s_report.current_state = NET_DIAG_ICMP_RESULT;
#endif
            break;
        case NET_DIAG_ICMP_RESULT: s_report.current_state = NET_DIAG_TCP_22_PENDING; break;
        case NET_DIAG_TCP_22_PENDING: s_report.tcp_22_open = check_tcp_nonblocking(s_report.target_ip, 22U); s_report.current_state = NET_DIAG_TCP_22_RESULT; break;
        case NET_DIAG_TCP_22_RESULT: s_report.current_state = NET_DIAG_TCP_80_PENDING; break;
        case NET_DIAG_TCP_80_PENDING: s_report.tcp_80_open = check_tcp_nonblocking(s_report.target_ip, 80U); s_report.current_state = NET_DIAG_TCP_80_RESULT; break;
        case NET_DIAG_TCP_80_RESULT: s_report.current_state = NET_DIAG_TCP_443_PENDING; break;
        case NET_DIAG_TCP_443_PENDING: s_report.tcp_443_open = check_tcp_nonblocking(s_report.target_ip, 443U); s_report.current_state = NET_DIAG_TCP_443_RESULT; break;
        case NET_DIAG_TCP_443_RESULT: s_report.current_state = NET_DIAG_COMPLETE; break;
        case NET_DIAG_COMPLETE: {
            static uint32_t s_last_audit_tick = 0U;
            uint32_t now = mk_kernel_get_tick();
            if ((now - s_last_audit_tick) >= 10000U) {
                char log_buf[128];
                snprintf(log_buf, sizeof(log_buf), "[NET_DIAG] IP:%s ICMP:%u (%ums) 22:%u 80:%u 443:%u\n", s_report.target_ip,
                         s_report.icmp_reachable ? 1U : 0U, s_report.icmp_rtt_ms, s_report.tcp_22_open ? 1U : 0U,
                         s_report.tcp_80_open ? 1U : 0U, s_report.tcp_443_open ? 1U : 0U);
                (void)vfs_log_append("wifi_audit.log", log_buf);
                s_last_audit_tick = now;
            }
            break;
        }
        default: s_report.current_state = NET_DIAG_IDLE; break;
    }
}

void network_diagnostics_get_report(net_diag_report_t *out_report) { if (out_report != NULL) *out_report = s_report; }
mk_status_t network_diagnostics_start(void) { s_report.current_state = s_report.target_configured ? NET_DIAG_ICMP_PENDING : NET_DIAG_IDLE; return MK_STATUS_OK; }
mk_status_t network_diagnostics_stop(void)
{
#if defined(ESP_PLATFORM)
    if (s_ping_handle != NULL) { (void)esp_ping_stop(s_ping_handle); (void)esp_ping_delete_session(s_ping_handle); s_ping_handle = NULL; }
    s_ping_active = false;
#endif
    s_report.current_state = NET_DIAG_IDLE; return MK_STATUS_OK;
}
mk_status_t network_diagnostics_reset(void) { return network_diagnostics_init(); }
static void network_diagnostics_handle_event(const mk_event_t *event) { (void)event; }
static const mk_app_interface_t s_net_diag_interface = {
    .id = MK_APP_NETWORK_DIAGNOSTICS, .name = "NET_DIAG", .init = network_diagnostics_init,
    .start = network_diagnostics_start, .tick = network_diagnostics_task,
    .handle_event = network_diagnostics_handle_event, .stop = network_diagnostics_stop, .reset = network_diagnostics_reset
};
const mk_app_interface_t *network_diagnostics_get_interface(void) { return &s_net_diag_interface; }
