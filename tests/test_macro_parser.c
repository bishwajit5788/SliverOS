/**
 * @file test_macro_parser.c
 * @brief Unit tests for incremental macro file parser.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "macro_parser.h"
#include "vfs.h"

void test_macro_parser(void)
{
    printf("[TEST] Starting Macro Parser Unit Tests...\n");

    (void)vfs_init();

    /* Write test macro file */
    int32_t fd = -1;
    assert(vfs_open("test_macro.txt", VFS_O_CREAT | VFS_O_WRONLY | VFS_O_TRUNC, &fd) == MK_STATUS_OK);

    const char *macro_content = "STRING Hi\nDELAY 50\nENTER\n";
    size_t written = 0;
    assert(vfs_write(fd, macro_content, strlen(macro_content), &written) == MK_STATUS_OK);
    (void)vfs_close(fd);

    /* Open for reading with parser */
    assert(vfs_open("test_macro.txt", VFS_O_RDONLY, &fd) == MK_STATUS_OK);
    macro_parser_t parser;
    assert(macro_parser_init(&parser, fd) == MK_STATUS_OK);

    macro_event_t evt;

    /* 'H' */
    assert(macro_parser_next_event(&parser, &evt) == MK_STATUS_OK);
    assert(evt.type == MACRO_CMD_STRING_CHAR);
    assert(evt.ch == 'H');

    /* 'i' */
    assert(macro_parser_next_event(&parser, &evt) == MK_STATUS_OK);
    assert(evt.type == MACRO_CMD_STRING_CHAR);
    assert(evt.ch == 'i');

    /* DELAY 50 */
    assert(macro_parser_next_event(&parser, &evt) == MK_STATUS_OK);
    assert(evt.type == MACRO_CMD_DELAY);
    assert(evt.delay_ms == 50);

    /* ENTER */
    assert(macro_parser_next_event(&parser, &evt) == MK_STATUS_OK);
    assert(evt.type == MACRO_CMD_ENTER);

    /* EOF */
    assert(macro_parser_next_event(&parser, &evt) == MK_STATUS_OK);
    assert(evt.type == MACRO_CMD_EOF);

    (void)vfs_close(fd);
    (void)vfs_delete("test_macro.txt");

    printf("[PASS] Macro Parser Unit Tests Passed Successfully.\n");
}
