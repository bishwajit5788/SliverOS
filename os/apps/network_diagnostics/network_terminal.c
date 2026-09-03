/**
 * @file network_terminal.c
 * @brief Bounded parser for the SliverOS Network Lab terminal.
 */
#include "network_terminal.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>

static void trim(char *s)
{
    char *start = s;
    size_t len;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1U);
    len = strlen(s);
    while (len > 0U && isspace((unsigned char)s[len - 1U])) s[--len] = '\0';
}

static bool blocked_command(const char *line)
{
    static const char *const blocked[] = {
        "attack", "deauth", "pmkid", "credential", "password",
        "exploit", "bruteforce", "brute-force", "handshake"
    };
    for (size_t i = 0; i < sizeof(blocked) / sizeof(blocked[0]); ++i) {
        if (strstr(line, blocked[i]) != NULL) return true;
    }
    return false;
}

net_terminal_result_t network_terminal_parse(const char *line,
                                             net_terminal_request_t *out_request)
{
    char buf[96];
    char arg[48] = {0};
    unsigned int port = 0U;

    if (line == NULL || out_request == NULL) return NET_TERM_INVALID;
    memset(out_request, 0, sizeof(*out_request));
    snprintf(buf, sizeof(buf), "%s", line);
    trim(buf);
    if (buf[0] == '\0') return NET_TERM_EMPTY;
    if (blocked_command(buf)) return NET_TERM_BLOCKED;

    if (strcmp(buf, "help") == 0) {
        out_request->command = NET_TERM_CMD_HELP;
    } else if (strcmp(buf, "status") == 0) {
        out_request->command = NET_TERM_CMD_STATUS;
    } else if (strcmp(buf, "wifi scan") == 0) {
        out_request->command = NET_TERM_CMD_WIFI_SCAN;
    } else if (strncmp(buf, "network scan ", 13U) == 0) {
        if (strlen(buf + 13U) >= sizeof(out_request->argument)) return NET_TERM_INVALID;
        out_request->command = NET_TERM_CMD_NETWORK_SCAN;
        snprintf(out_request->argument, sizeof(out_request->argument), "%s", buf + 13U);
    } else if (strncmp(buf, "port check ", 11U) == 0) {
        if (sscanf(buf + 11U, "%47s %u", arg, &port) != 2 || port > 65535U || port == 0U) {
            return NET_TERM_INVALID;
        }
        out_request->command = NET_TERM_CMD_PORT_CHECK;
        snprintf(out_request->argument, sizeof(out_request->argument), "%s", arg);
        out_request->port = (uint16_t)port;
        out_request->has_port = true;
    } else if (strcmp(buf, "clear") == 0) {
        out_request->command = NET_TERM_CMD_CLEAR;
    } else {
        return NET_TERM_UNKNOWN;
    }

    return NET_TERM_OK;
}

const char *network_terminal_result_string(net_terminal_result_t result)
{
    switch (result) {
        case NET_TERM_OK: return "ok";
        case NET_TERM_EMPTY: return "empty command";
        case NET_TERM_UNKNOWN: return "unknown command";
        case NET_TERM_INVALID: return "invalid command";
        case NET_TERM_BLOCKED: return "command blocked by safety policy";
        default: return "parser error";
    }
}
