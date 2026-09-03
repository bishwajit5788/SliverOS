/**
 * @file test_state_machine.c
 * @brief Unit tests for strict state machine transitions.
 */

#include <stdio.h>
#include <assert.h>
#include "state_machine.h"
#include "kernel.h"

void test_state_machine(void)
{
    printf("[TEST] Starting State Machine Unit Tests...\n");

    mk_kernel_t kernel;
    kernel.state = MK_KERNEL_STATE_RESET;

    /* 1. Valid Kernel Transition: RESET -> INIT */
    assert(mk_kernel_transition(&kernel, MK_KERNEL_STATE_INIT) == MK_STATUS_OK);
    assert(kernel.state == MK_KERNEL_STATE_INIT);

    /* 2. Invalid Kernel Transition: INIT -> RUNNING (must go through READY) */
    assert(mk_kernel_transition(&kernel, MK_KERNEL_STATE_RUNNING) == MK_STATUS_INVALID_STATE);
    assert(kernel.state == MK_KERNEL_STATE_INIT);

    /* 3. Valid Progression: INIT -> READY -> RUNNING */
    assert(mk_kernel_transition(&kernel, MK_KERNEL_STATE_READY) == MK_STATUS_OK);
    assert(mk_kernel_transition(&kernel, MK_KERNEL_STATE_RUNNING) == MK_STATUS_OK);

    /* 4. Application Transition Verification */
    kernel.apps[MK_APP_BLE_HID].id = MK_APP_BLE_HID;
    kernel.apps[MK_APP_BLE_HID].state = MK_APP_STATE_OFF;

    /* Valid: OFF -> INIT */
    assert(mk_app_transition(&kernel, MK_APP_BLE_HID, MK_APP_STATE_INIT) == MK_STATUS_OK);

    /* Invalid: INIT -> ACTIVE (must go through READY) */
    assert(mk_app_transition(&kernel, MK_APP_BLE_HID, MK_APP_STATE_ACTIVE) == MK_STATUS_INVALID_STATE);

    /* Valid: INIT -> READY -> ACTIVE -> STOPPING -> READY */
    assert(mk_app_transition(&kernel, MK_APP_BLE_HID, MK_APP_STATE_READY) == MK_STATUS_OK);
    assert(mk_app_transition(&kernel, MK_APP_BLE_HID, MK_APP_STATE_ACTIVE) == MK_STATUS_OK);
    assert(mk_app_transition(&kernel, MK_APP_BLE_HID, MK_APP_STATE_STOPPING) == MK_STATUS_OK);
    assert(mk_app_transition(&kernel, MK_APP_BLE_HID, MK_APP_STATE_READY) == MK_STATUS_OK);

    /* Error recovery transition: ACTIVE -> ERROR -> INIT */
    assert(mk_app_transition(&kernel, MK_APP_BLE_HID, MK_APP_STATE_ACTIVE) == MK_STATUS_OK);
    assert(mk_app_transition(&kernel, MK_APP_BLE_HID, MK_APP_STATE_ERROR) == MK_STATUS_OK);
    assert(mk_app_transition(&kernel, MK_APP_BLE_HID, MK_APP_STATE_INIT) == MK_STATUS_OK);

    printf("[PASS] State Machine Unit Tests Passed Successfully.\n");
}
