/**
 * @file network_terminal.h
 * @brief Safe terminal command parser for the Network Diagnostics application.
 *
 * The terminal is an operator interface for authorized diagnostics. It does not
 * implement credential harvesting, PMKID capture, deauthentication, exploitation,
 * or unrestricted attack automation.
 */
#ifndef MK_NETWORK_TERMINAL_H
#define MK_NETWORK_TERMINAL_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NET_TERM_OK = 0,
    NET_TERM_EMPTY,
    NET_TERM_UNKNOWN,
    NET_TERM_INVALID,
    NET_TERM_BLOCKED
} net_terminal_result_t;

typedef enum {
    NET_TERM_CMD_HELP = 0,
    NET_TERM_CMD_STATUS,
    NET_TERM_CMD_WIFI_SCAN,
    NET_TERM_CMD_NETWORK_SCAN,
    NET_TERM_CMD_PORT_CHECK,
    NET_TERM_CMD_CLEAR
} net_terminal_command_t;

typedef struct {
    net_terminal_command_t command;
    char argument[48];
    uint16_t port;
    bool has_port;
} net_terminal_request_t;

net_terminal_result_t network_terminal_parse(const char *line,
                                             net_terminal_request_t *out_request);
const char *network_terminal_result_string(net_terminal_result_t result);

#ifdef __cplusplus
}
#endif

#endif /* MK_NETWORK_TERMINAL_H */
