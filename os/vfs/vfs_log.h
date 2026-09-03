/**
 * @file vfs_log.h
 * @brief Structured append-only audit logger over VFS.
 */

#ifndef MK_VFS_LOG_H
#define MK_VFS_LOG_H

#include "kernel_types.h"
#include "vfs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VFS_LOG_MAX_LINE 128U

mk_status_t vfs_log_init(const char *log_filename);
mk_status_t vfs_log_append(const char *log_filename, const char *entry);
mk_status_t vfs_log_clear(const char *log_filename);

#ifdef __cplusplus
}
#endif

#endif /* MK_VFS_LOG_H */
