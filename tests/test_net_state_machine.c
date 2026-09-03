/**
 * @file test_net_state_machine.c
 * @brief Unit tests for non-blocking network diagnostics state machine.
 */

#include "network_diagnostics.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

void run_net_state_machine_tests(void)
{
    printf("[TEST] Starting Network Diagnostics State Machine Unit Tests...\n");

    mk_status_t status = network_diagnostics_init();
    assert(status == MK_STATUS_OK);

    net_diag_report_t report;
    network_diagnostics_get_report(&report);
    assert(report.target_configured == true);
    assert(strcmp(report.target_ip, "127.0.0.1") == 0);

    /* 1. Set explicit authorized target */
    status = network_diagnostics_set_target("192.168.1.1");
    assert(status == MK_STATUS_OK);

    network_diagnostics_get_report(&report);
    assert(strcmp(report.target_ip, "192.168.1.1") == 0);
    assert(report.current_state == NET_DIAG_TARGET_CONFIGURED);

    /* 2. Step through non-blocking states */
    /* Step: TARGET_CONFIGURED -> ICMP_PENDING */
    network_diagnostics_task(NULL);
    network_diagnostics_get_report(&report);
    assert(report.current_state == NET_DIAG_ICMP_PENDING);

    /* Step: ICMP_PENDING -> ICMP_RESULT */
    network_diagnostics_task(NULL);
    network_diagnostics_get_report(&report);
    assert(report.current_state == NET_DIAG_ICMP_RESULT);

    /* Step: ICMP_RESULT -> TCP_22_PENDING */
    network_diagnostics_task(NULL);
    network_diagnostics_get_report(&report);
    assert(report.current_state == NET_DIAG_TCP_22_PENDING);

    /* Step: TCP_22_PENDING -> TCP_22_RESULT */
    network_diagnostics_task(NULL);
    network_diagnostics_get_report(&report);
    assert(report.current_state == NET_DIAG_TCP_22_RESULT);

    /* Step: TCP_22_RESULT -> TCP_80_PENDING */
    network_diagnostics_task(NULL);
    network_diagnostics_get_report(&report);
    assert(report.current_state == NET_DIAG_TCP_80_PENDING);

    /* Step: TCP_80_PENDING -> TCP_80_RESULT */
    network_diagnostics_task(NULL);
    network_diagnostics_get_report(&report);
    assert(report.current_state == NET_DIAG_TCP_80_RESULT);

    /* Step: TCP_80_RESULT -> TCP_443_PENDING */
    network_diagnostics_task(NULL);
    network_diagnostics_get_report(&report);
    assert(report.current_state == NET_DIAG_TCP_443_PENDING);

    /* Step: TCP_443_PENDING -> TCP_443_RESULT */
    network_diagnostics_task(NULL);
    network_diagnostics_get_report(&report);
    assert(report.current_state == NET_DIAG_TCP_443_RESULT);

    /* Step: TCP_443_RESULT -> COMPLETE */
    network_diagnostics_task(NULL);
    network_diagnostics_get_report(&report);
    assert(report.current_state == NET_DIAG_COMPLETE);

    /* Verify subsequent ticks remain bounded in COMPLETE state */
    network_diagnostics_task(NULL);
    network_diagnostics_get_report(&report);
    assert(report.current_state == NET_DIAG_COMPLETE);

    /* Clean reset */
    assert(network_diagnostics_reset() == MK_STATUS_OK);

    printf("[PASS] Network Diagnostics State Machine Unit Tests Passed Successfully.\n");
}
