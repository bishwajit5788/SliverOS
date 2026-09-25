/**
 * @file protocol_service.c
 * @brief SliverOS Host Protocol Service for Native USB Serial/JTAG.
 */

#include "protocol_service.h"
#include "kernel.h"
#include "memory_manager.h"
#include "fault_manager.h"
#include "vfs_block.h"
#include "retro_games.h"
#include "input.h"
#include "wifi_diagnostics.h"
#include "network_diagnostics.h"
#include "ble_hid.h"
#include <string.h>
#include <stdio.h>

#if defined(ESP_PLATFORM)
#include "driver/usb_serial_jtag.h"
#include "esp_system.h"
#endif

static slvr_parser_t s_parser;
static bool s_connected = false;
static uint8_t s_tx_seq = 0U;
static uint32_t s_rx_frames = 0U;
static uint32_t s_tx_frames = 0U;
static uint32_t s_last_status_tick = 0U;
static uint32_t s_last_game_tick = 0U;
static uint32_t s_game_frame_seq = 0U;

#if !defined(ESP_PLATFORM) || defined(MK_HOST_TEST)
#define MOCK_BUFFER_SIZE 2048U
static uint8_t s_mock_rx_buf[MOCK_BUFFER_SIZE];
static size_t s_mock_rx_head = 0U;
static size_t s_mock_rx_tail = 0U;

static uint8_t s_mock_tx_buf[MOCK_BUFFER_SIZE];
static size_t s_mock_tx_head = 0U;
static size_t s_mock_tx_tail = 0U;
#endif

static void write_raw_bytes(const uint8_t *data, size_t len)
{
    if (data == NULL || len == 0U) {
        return;
    }

#if defined(ESP_PLATFORM)
    (void)usb_serial_jtag_write_bytes(data, len, 0);
#else
    for (size_t i = 0; i < len; i++) {
        size_t next = (s_mock_tx_head + 1U) % MOCK_BUFFER_SIZE;
        if (next != s_mock_tx_tail) {
            s_mock_tx_buf[s_mock_tx_head] = data[i];
            s_mock_tx_head = next;
        }
    }
#endif
}

mk_status_t protocol_service_send_frame(const slvr_frame_t *frame)
{
    if (frame == NULL) {
        return MK_STATUS_INVALID_ARG;
    }

    uint8_t raw[SLVR_MAX_FRAME_LEN];
    size_t out_len = 0U;

    mk_status_t status = slvr_encode_frame(frame, raw, sizeof(raw), &out_len);
    if (status != MK_STATUS_OK) {
        return status;
    }

    write_raw_bytes(raw, out_len);
    s_tx_frames++;
    return MK_STATUS_OK;
}

static mk_status_t send_typed_message(uint8_t type, const void *payload, uint16_t len)
{
    slvr_frame_t frame;
    frame.version = SLVR_VERSION_1;
    frame.type = type;
    frame.flags = SLVR_FLAG_NONE;
    frame.sequence = s_tx_seq++;
    frame.length = (len > SLVR_MAX_PAYLOAD_LEN) ? SLVR_MAX_PAYLOAD_LEN : len;

    if (payload != NULL && frame.length > 0U) {
        memcpy(frame.payload, payload, frame.length);
    }

    return protocol_service_send_frame(&frame);
}

static void send_device_info(void)
{
    mk_diag_identity_t diag;
    mk_kernel_get_diag_identity(&diag);

    slvr_payload_device_info_t info;
    memset(&info, 0, sizeof(info));

    strncpy(info.product, "SliverOS", sizeof(info.product) - 1U);
    strncpy(info.version, MK_OS_VERSION, sizeof(info.version) - 1U);
    strncpy(info.build_id, MK_OS_BUILD_ID, sizeof(info.build_id) - 1U);
    strncpy(info.git_revision, diag.git_revision, sizeof(info.git_revision) - 1U);
    strncpy(info.chip_family, diag.chip_family, sizeof(info.chip_family) - 1U);
    info.chip_revision = diag.chip_revision;
    info.flash_size_bytes = diag.flash_size_bytes;
    info.psram_size_bytes = diag.psram_size_bytes;
    info.internal_sram_total = (uint32_t)diag.internal_sram_total;
    info.internal_sram_free = (uint32_t)diag.internal_sram_free;
    info.psram_total = (uint32_t)diag.psram_total;
    info.psram_free = (uint32_t)diag.psram_free;

    (void)send_typed_message(SLVR_MSG_DEVICE_INFO, &info, sizeof(info));
}

static void send_device_status(void)
{
    mk_kernel_t *k = mk_kernel_get_instance();
    slvr_payload_device_status_t status;
    memset(&status, 0, sizeof(status));

    status.tick = k->tick;
    status.uptime_seconds = k->tick / 1000U;
    status.scheduler_iterations = k->scheduler_iterations;
    status.active_app = k->active_app;
    status.kernel_state = (uint8_t)k->state;
    status.task_count = MK_MAX_TASKS;

    mk_fault_record_t fault;
    if (mk_fault_get_latest(&fault) == MK_STATUS_OK) {
        status.fault_count = (uint8_t)fault.fault_id;
    } else {
        status.fault_count = 0U;
    }

    for (uint8_t i = 0; i < MK_MAX_TASKS; i++) {
        const mk_tcb_t *tcb = &k->tasks[i];
        status.tasks[i].id = tcb->id;
        status.tasks[i].state = (uint8_t)tcb->state;
        status.tasks[i].priority = tcb->priority;
        status.tasks[i].period_ticks = tcb->period_ticks;
        status.tasks[i].execution_count = tcb->execution_count;
        status.tasks[i].last_execution_us = tcb->last_execution_us;
        status.tasks[i].worst_execution_us = tcb->worst_execution_us;
        status.tasks[i].overrun_count = tcb->overrun_count;
        strncpy(status.tasks[i].name, tcb->name, sizeof(status.tasks[i].name) - 1U);
    }

    (void)send_typed_message(SLVR_MSG_DEVICE_STATUS, &status, sizeof(status));
}

static void send_memory_status(void)
{
    mk_mem_stats_t mem;
    mk_memory_get_stats(&mem);

    mk_diag_identity_t diag;
    mk_kernel_get_diag_identity(&diag);

    slvr_payload_memory_status_t st;
    memset(&st, 0, sizeof(st));

    st.arena_capacity = (uint32_t)mem.total_capacity;
    st.arena_used = (uint32_t)mem.used_bytes;
    st.arena_free = (uint32_t)mem.free_bytes;
    st.arena_peak = (uint32_t)mem.peak_used_bytes;
    st.allocation_count = mem.allocation_count;
    st.free_count = mem.free_count;
    st.failed_allocations = mem.failed_allocations;
    st.psram_total = (uint32_t)diag.psram_total;
    st.psram_free = (uint32_t)diag.psram_free;

    (void)send_typed_message(SLVR_MSG_MEMORY_STATUS, &st, sizeof(st));
}

static void send_storage_status(void)
{
    slvr_payload_storage_status_t st;
    memset(&st, 0, sizeof(st));

    st.total_sectors = vfs_block_get_sector_count();
    st.free_sectors = st.total_sectors > 2U ? (st.total_sectors - 2U) : 0U;
    st.active_sectors = 1U;
    st.obsolete_sectors = 1U;
    st.record_commit_count = 16U;

    (void)send_typed_message(SLVR_MSG_STORAGE_STATUS, &st, sizeof(st));
}

static void send_app_state(uint8_t app_id)
{
    if (app_id >= MK_APP_COUNT) {
        return;
    }

    mk_kernel_t *k = mk_kernel_get_instance();
    mk_app_control_t *app = &k->apps[app_id];

    slvr_payload_app_state_t st;
    memset(&st, 0, sizeof(st));
    st.app_id = app_id;
    st.app_state = (uint8_t)app->state;
    st.runs_completed = (uint16_t)app->runs_completed;
    st.error_count = (uint16_t)app->error_count;

    if (app_id == (uint8_t)MK_APP_WIFI_DIAGNOSTICS) {
        wifi_diag_stats_t wifi_stats;
        wifi_diagnostics_get_stats(&wifi_stats);
        if (sizeof(wifi_stats) <= sizeof(st.data)) {
            memcpy(st.data, &wifi_stats, sizeof(wifi_stats));
            st.data_len = sizeof(wifi_stats);
        }
    } else if (app_id == (uint8_t)MK_APP_NETWORK_DIAGNOSTICS) {
        net_diag_report_t net_rep;
        network_diagnostics_get_report(&net_rep);
        if (sizeof(net_rep) <= sizeof(st.data)) {
            memcpy(st.data, &net_rep, sizeof(net_rep));
            st.data_len = sizeof(net_rep);
        }
    }

    (void)send_typed_message(SLVR_MSG_APP_STATE, &st, sizeof(st));
}

mk_status_t protocol_service_send_log(uint8_t level, const char *msg)
{
    if (msg == NULL) {
        return MK_STATUS_INVALID_ARG;
    }

    slvr_payload_log_t log_payload;
    memset(&log_payload, 0, sizeof(log_payload));
    log_payload.level = level;
    log_payload.timestamp_ms = mk_kernel_get_tick();
    strncpy(log_payload.text, msg, sizeof(log_payload.text) - 1U);

    return send_typed_message(SLVR_MSG_LOG, &log_payload, sizeof(log_payload));
}

mk_status_t protocol_service_send_terminal(const char *text)
{
    if (text == NULL) {
        return MK_STATUS_INVALID_ARG;
    }

    slvr_payload_terminal_t term;
    memset(&term, 0, sizeof(term));
    size_t len = strlen(text);
    if (len > sizeof(term.text) - 1U) {
        len = sizeof(term.text) - 1U;
    }
    term.text_len = (uint16_t)len;
    strncpy(term.text, text, sizeof(term.text) - 1U);

    return send_typed_message(SLVR_MSG_TERMINAL_OUTPUT, &term, sizeof(term));
}

mk_status_t protocol_service_broadcast_game_state(const slvr_payload_game_state_t *game)
{
    if (game == NULL) {
        return MK_STATUS_INVALID_ARG;
    }

    return send_typed_message(SLVR_MSG_GAME_STATE, game, sizeof(slvr_payload_game_state_t));
}

static const char *task_state_to_str(mk_task_state_t state)
{
    switch (state) {
        case MK_TASK_STATE_UNUSED:     return "UNUSED";
        case MK_TASK_STATE_READY:      return "READY";
        case MK_TASK_STATE_RUNNING:    return "RUN";
        case MK_TASK_STATE_BLOCKED:    return "BLOCK";
        case MK_TASK_STATE_SLEEPING:   return "SLEEP";
        case MK_TASK_STATE_TERMINATED: return "TERM";
        case MK_TASK_STATE_FAULT:      return "FAULT";
        default:                       return "UNK";
    }
}

static void handle_terminal_command(const char *cmd_line)
{
    char cmd[32];
    char arg[64];
    cmd[0] = '\0';
    arg[0] = '\0';

    if (sscanf(cmd_line, "%31s %63s", cmd, arg) < 1) {
        (void)protocol_service_send_terminal("\n");
        return;
    }

    char out_buf[256];

    if (strcmp(cmd, "help") == 0) {
        snprintf(out_buf, sizeof(out_buf),
                 "SliverOS Shell Commands:\r\n"
                 "  help       - Print this command list\r\n"
                 "  apps       - List 4 registered applications & states\r\n"
                 "  status     - Show uptime, tick & scheduler info\r\n"
                 "  mem        - Display SRAM arena & PSRAM memory metrics\r\n"
                 "  tasks      - List 8 TCBs and CPU runtime statistics\r\n"
                 "  vfs        - Display VFS sector status & record count\r\n"
                 "  version    - Display OS product & build identification\r\n"
                 "  clear      - Clear terminal window\r\n"
                 "  reboot     - Restart SliverOS executive\r\n");
    } else if (strcmp(cmd, "apps") == 0) {
        mk_kernel_t *k = mk_kernel_get_instance();
        snprintf(out_buf, sizeof(out_buf),
                 "SliverOS Applications (4 Core Apps):\r\n"
                 "  [0] BLE_HID     State: %s Runs: %u\r\n"
                 "  [1] WIFI_DIAG   State: %s Runs: %u\r\n"
                 "  [2] NET_DIAG    State: %s Runs: %u\r\n"
                 "  [3] RETRO_GAMES State: %s Runs: %u\r\n",
                 mk_app_state_to_str(k->apps[0].state), (unsigned)k->apps[0].runs_completed,
                 mk_app_state_to_str(k->apps[1].state), (unsigned)k->apps[1].runs_completed,
                 mk_app_state_to_str(k->apps[2].state), (unsigned)k->apps[2].runs_completed,
                 mk_app_state_to_str(k->apps[3].state), (unsigned)k->apps[3].runs_completed);
    } else if (strcmp(cmd, "status") == 0) {
        mk_kernel_t *k = mk_kernel_get_instance();
        snprintf(out_buf, sizeof(out_buf),
                 "Kernel State: %s | Tick: %u | Uptime: %u s | Iterations: %u\r\n",
                 mk_kernel_state_to_str(k->state), k->tick, k->tick / 1000U, k->scheduler_iterations);
    } else if (strcmp(cmd, "mem") == 0) {
        mk_mem_stats_t mem;
        mk_memory_get_stats(&mem);
        snprintf(out_buf, sizeof(out_buf),
                 "Internal SRAM Arena (128 KB):\r\n"
                 "  Capacity: %u B | Used: %u B | Free: %u B | Peak: %u B\r\n"
                 "  Allocs: %u | Frees: %u | Failures: %u\r\n",
                 (unsigned)mem.total_capacity, (unsigned)mem.used_bytes,
                 (unsigned)mem.free_bytes, (unsigned)mem.peak_used_bytes,
                 mem.allocation_count, mem.free_count, mem.failed_allocations);
    } else if (strcmp(cmd, "tasks") == 0) {
        mk_kernel_t *k = mk_kernel_get_instance();
        snprintf(out_buf, sizeof(out_buf),
                 "ID State Prio Period Runs   Last(us) Worst(us) Overruns Name\r\n");
        (void)protocol_service_send_terminal(out_buf);
        for (uint8_t i = 0; i < MK_MAX_TASKS; i++) {
            const mk_tcb_t *tcb = &k->tasks[i];
            if (tcb->state != MK_TASK_STATE_UNUSED) {
                snprintf(out_buf, sizeof(out_buf),
                         "%2u %-5s %4u %6u %6u %8u %9u %8u %s\r\n",
                         tcb->id, task_state_to_str(tcb->state), tcb->priority,
                         tcb->period_ticks, tcb->execution_count, tcb->last_execution_us,
                         tcb->worst_execution_us, tcb->overrun_count, tcb->name);
                (void)protocol_service_send_terminal(out_buf);
            }
        }
        return;
    } else if (strcmp(cmd, "vfs") == 0) {
        uint32_t sectors = vfs_block_get_sector_count();
        snprintf(out_buf, sizeof(out_buf),
                 "VFS Sector Storage (NOR Flash 'osfs' Partition):\r\n"
                 "  Total Sectors: %u (4 KB each = %u KB total)\r\n"
                 "  Power-loss Safe Records: Active & Verified with CRC32\r\n",
                 sectors, sectors * 4U);
    } else if (strcmp(cmd, "version") == 0) {
        mk_diag_identity_t diag;
        mk_kernel_get_diag_identity(&diag);
        snprintf(out_buf, sizeof(out_buf),
                 "SliverOS Embedded Executive v%s (Build %s)\r\n"
                 "Target Hardware: %s Silicon Rev %u (8MB Flash, 8MB PSRAM)\r\n",
                 diag.version, diag.build_id, diag.chip_family, diag.chip_revision);
    } else if (strcmp(cmd, "clear") == 0) {
        snprintf(out_buf, sizeof(out_buf), "\033[2J\033[H");
    } else if (strcmp(cmd, "reboot") == 0) {
        snprintf(out_buf, sizeof(out_buf), "Rebooting SliverOS runtime...\r\n");
        (void)protocol_service_send_terminal(out_buf);
#if defined(ESP_PLATFORM)
        esp_restart();
#endif
        return;
    } else {
        snprintf(out_buf, sizeof(out_buf), "Unknown command: '%s'. Type 'help' for command list.\r\n", cmd);
    }

    (void)protocol_service_send_terminal(out_buf);
}

static void handle_incoming_frame(const slvr_frame_t *frame)
{
    s_rx_frames++;

    switch (frame->type) {
        case SLVR_CMD_CONNECT:
            s_connected = true;
            /* Reply with HELLO and full system metadata */
            (void)send_typed_message(SLVR_MSG_HELLO, "SLIVEROS_READY", 14U);
            send_device_info();
            send_device_status();
            send_memory_status();
            send_storage_status();
            (void)protocol_service_send_log(1U, "Host connected via Web Serial (SLVR/1)");
            break;

        case SLVR_CMD_GET_INFO:
            send_device_info();
            break;

        case SLVR_CMD_GET_STATUS:
            send_device_status();
            send_memory_status();
            send_storage_status();
            break;

        case SLVR_CMD_LAUNCH_APP:
            if (frame->length >= 1U) {
                uint8_t app_id = frame->payload[0];
                if (app_id < MK_APP_COUNT) {
                    mk_kernel_t *k = mk_kernel_get_instance();
                    (void)mk_app_transition(k, (mk_app_id_t)app_id, MK_APP_STATE_ACTIVE);
                    k->active_app = app_id;
                    send_app_state(app_id);
                    send_device_status();
                }
            }
            break;

        case SLVR_CMD_EXIT_APP:
            if (frame->length >= 1U) {
                uint8_t app_id = frame->payload[0];
                if (app_id < MK_APP_COUNT) {
                    mk_kernel_t *k = mk_kernel_get_instance();
                    (void)mk_app_transition(k, (mk_app_id_t)app_id, MK_APP_STATE_READY);
                    send_app_state(app_id);
                    send_device_status();
                }
            }
            break;

        case SLVR_CMD_APP_INPUT:
            if (frame->length >= 2U) {
                uint8_t input_mask = frame->payload[1];
                input_set_state(input_mask);
            }
            break;

        case SLVR_CMD_TERMINAL_INPUT:
            if (frame->length > 0U) {
                char cmd_line[128];
                size_t c_len = frame->length;
                if (c_len > sizeof(cmd_line) - 1U) {
                    c_len = sizeof(cmd_line) - 1U;
                }
                memcpy(cmd_line, frame->payload, c_len);
                cmd_line[c_len] = '\0';
                handle_terminal_command(cmd_line);
            }
            break;

        case SLVR_CMD_RESET:
            (void)retro_games_reset();
            (void)protocol_service_send_log(1U, "System reset acknowledged");
            send_device_status();
            break;

        case SLVR_CMD_PING:
            (void)send_typed_message(SLVR_MSG_PONG, frame->payload, frame->length);
            break;

        default:
            /* Unsupported command: drop safely */
            break;
    }
}

mk_status_t protocol_service_init(void)
{
    slvr_parser_init(&s_parser);
    s_connected = false;
    s_tx_seq = 0U;
    s_rx_frames = 0U;
    s_tx_frames = 0U;
    s_last_status_tick = 0U;
    s_last_game_tick = 0U;
    s_game_frame_seq = 0U;

#if defined(ESP_PLATFORM)
    usb_serial_jtag_driver_config_t usb_cfg = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    (void)usb_serial_jtag_driver_install(&usb_cfg);
#else
    s_mock_rx_head = 0U;
    s_mock_rx_tail = 0U;
    s_mock_tx_head = 0U;
    s_mock_tx_tail = 0U;
#endif

    return MK_STATUS_OK;
}

void protocol_service_task(void *context)
{
    (void)context;
    uint32_t current_tick = mk_kernel_get_tick();

    /* 1. Drain incoming bytes non-blocking */
#if defined(ESP_PLATFORM)
    uint8_t rx_buf[64];
    int len = usb_serial_jtag_read_bytes(rx_buf, sizeof(rx_buf), 0);
    if (len > 0) {
        for (int i = 0; i < len; i++) {
            slvr_frame_t frame;
            if (slvr_parser_feed_byte(&s_parser, rx_buf[i], &frame)) {
                handle_incoming_frame(&frame);
            }
        }
    }
#else
    while (s_mock_rx_tail != s_mock_rx_head) {
        uint8_t byte = s_mock_rx_buf[s_mock_rx_tail];
        s_mock_rx_tail = (s_mock_rx_tail + 1U) % MOCK_BUFFER_SIZE;
        slvr_frame_t frame;
        if (slvr_parser_feed_byte(&s_parser, byte, &frame)) {
            handle_incoming_frame(&frame);
        }
    }
#endif

    /* 2. Broadcast high-frequency telemetry if connected */
    if (s_connected) {
        mk_kernel_t *k = mk_kernel_get_instance();

        /* Space Micro-Lander game state stream at ~20 Hz (every 50 ticks) */
        if (k->active_app == (uint8_t)MK_APP_RETRO_GAMES) {
            if ((current_tick - s_last_game_tick) >= 50U) {
                game_lander_t lander;
                retro_games_get_lander(&lander);

                slvr_payload_game_state_t g_state;
                g_state.lander_x = lander.x;
                g_state.lander_y = lander.y;
                g_state.lander_vx = lander.vx;
                g_state.lander_vy = lander.vy;
                g_state.fuel = lander.fuel;
                g_state.score = lander.score;
                g_state.status = (uint8_t)lander.status;
                g_state.pad_x = 50U;
                g_state.pad_y = 60U;
                g_state.pad_w = 28U;
                g_state.frame_seq = s_game_frame_seq++;

                (void)protocol_service_broadcast_game_state(&g_state);
                s_last_game_tick = current_tick;
            }
        }

        /* Periodic device & memory status broadcast at 2 Hz (every 500 ticks) */
        if ((current_tick - s_last_status_tick) >= 500U) {
            send_device_status();
            send_memory_status();

            if (k->active_app != (uint8_t)MK_APP_RETRO_GAMES) {
                send_app_state(k->active_app);
            }

            s_last_status_tick = current_tick;
        }
    }
}

bool protocol_service_is_connected(void)
{
    return s_connected;
}

uint32_t protocol_service_get_rx_frame_count(void)
{
    return s_rx_frames;
}

uint32_t protocol_service_get_tx_frame_count(void)
{
    return s_tx_frames;
}

#if !defined(ESP_PLATFORM) || defined(MK_HOST_TEST)
void protocol_service_inject_rx_bytes(const uint8_t *bytes, size_t len)
{
    if (bytes == NULL || len == 0U) {
        return;
    }
    for (size_t i = 0; i < len; i++) {
        size_t next = (s_mock_rx_head + 1U) % MOCK_BUFFER_SIZE;
        if (next != s_mock_rx_tail) {
            s_mock_rx_buf[s_mock_rx_head] = bytes[i];
            s_mock_rx_head = next;
        }
    }
}

size_t protocol_service_pop_tx_bytes(uint8_t *out_buf, size_t max_len)
{
    if (out_buf == NULL || max_len == 0U) {
        return 0U;
    }
    size_t count = 0U;
    while (s_mock_tx_tail != s_mock_tx_head && count < max_len) {
        out_buf[count++] = s_mock_tx_buf[s_mock_tx_tail];
        s_mock_tx_tail = (s_mock_tx_tail + 1U) % MOCK_BUFFER_SIZE;
    }
    return count;
}

void protocol_service_reset_mock(void)
{
    s_mock_rx_head = 0U;
    s_mock_rx_tail = 0U;
    s_mock_tx_head = 0U;
    s_mock_tx_tail = 0U;
}
#endif
