/**
 * @file vfs_log.c
 * @brief Structured append-only audit logger implementation.
 */

#include "vfs_log.h"
#include <string.h>

mk_status_t vfs_log_init(const char *log_filename)
{
    int32_t fd = -1;
    mk_status_t st = vfs_open(log_filename, VFS_O_CREAT | VFS_O_WRONLY, &fd);
    if (st == MK_STATUS_OK) {
        (void)vfs_close(fd);
    }
    return st;
}

mk_status_t vfs_log_append(const char *log_filename, const char *entry)
{
    if (log_filename == NULL || entry == NULL) {
        return MK_STATUS_INVALID_ARG;
    }

    int32_t fd = -1;
    mk_status_t st = vfs_open(log_filename, VFS_O_CREAT | VFS_O_WRONLY | VFS_O_APPEND, &fd);
    if (st != MK_STATUS_OK) {
        return st;
    }

    size_t written = 0;
    size_t len = strlen(entry);
    st = vfs_write(fd, entry, len, &written);
    (void)vfs_close(fd);

    return st;
}

mk_status_t vfs_log_clear(const char *log_filename)
{
    int32_t fd = -1;
    mk_status_t st = vfs_open(log_filename, VFS_O_CREAT | VFS_O_WRONLY | VFS_O_TRUNC, &fd);
    if (st == MK_STATUS_OK) {
        (void)vfs_close(fd);
    }
    return st;
}
