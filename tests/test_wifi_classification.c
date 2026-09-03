/**
 * @file test_wifi_classification.c
 * @brief Unit tests for passive Wi-Fi frame metadata classification.
 */

#include "hal_wifi.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

void run_wifi_classification_tests(void)
{
    printf("[TEST] Starting Wi-Fi Frame Metadata Classification Unit Tests...\n");

    mk_status_t status = hal_wifi_init();
    assert(status == MK_STATUS_OK);

    /* 1. Simulate Promiscuous RX of a Beacon Frame (Type 0, Subtype 8) */
    /* 802.11 Frame Control field: Type 0 (Mgmt), Subtype 8 (Beacon) = 0x80 */
    uint8_t beacon_packet[] = { 0x80, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    hal_wifi_inject_frame_for_test(beacon_packet, sizeof(beacon_packet), -65);

    hal_wifi_frame_meta_t meta;
    assert(hal_wifi_queue_pop(&meta) == true);
    assert(meta.frame_type == WIFI_FRAME_TYPE_MGMT);
    assert(meta.frame_subtype == 8U);
    assert(meta.rssi == -65);
    assert(meta.length == sizeof(beacon_packet));

    /* 2. Simulate Probe Request (Type 0, Subtype 4) */
    uint8_t probe_packet[] = { 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    hal_wifi_inject_frame_for_test(probe_packet, sizeof(probe_packet), -42);
    assert(hal_wifi_queue_pop(&meta) == true);
    assert(meta.frame_type == WIFI_FRAME_TYPE_MGMT);
    assert(meta.frame_subtype == 4U);
    assert(meta.rssi == -42);

    /* 3. Simulate Data Frame (Type 2, Subtype 0) = 0x08 */
    uint8_t data_packet[] = { 0x08, 0x00, 0x00, 0x00, 0x00, 0x00 };
    hal_wifi_inject_frame_for_test(data_packet, sizeof(data_packet), -78);
    assert(hal_wifi_queue_pop(&meta) == true);
    assert(meta.frame_type == WIFI_FRAME_TYPE_DATA);
    assert(meta.frame_subtype == 0U);
    assert(meta.rssi == -78);

    /* Queue should now be empty */
    assert(hal_wifi_queue_pop(&meta) == false);

    printf("[PASS] Wi-Fi Frame Metadata Classification Unit Tests Passed Successfully.\n");
}
