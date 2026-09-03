/**
 * @file vfs.h
 * @brief OS-owned Virtual File System interface over dedicated 'osfs' flash partition.
 */

#ifndef MK_VFS_H
#define MK_VFS_H

#include "kernel_types.h"
#include "vfs_block.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VFS_MAX_OPEN_FILES      8U
#define VFS_MAX_FILENAME_LEN    32U

/* Open Flags */
#define VFS_O_RDONLY    0x0001U
#define VFS_O_WRONLY    0x0002U
#define VFS_O_RDWR      0x0003U
#define VFS_O_CREAT     0x0004U
#define VFS_O_APPEND    0x0008U
#define VFS_O_TRUNC     0x0010U

typedef struct {
    size_t size;
    uint32_t flags;
    uint32_t created_tick;
    uint32_t modified_tick;
} mk_vfs_stat_t;

mk_status_t vfs_init(void);
mk_status_t vfs_open(const char *path, uint32_t flags, int32_t *out_fd);
mk_status_t vfs_read(int32_t fd, void *buffer, size_t size, size_t *bytes_read);
mk_status_t vfs_write(int32_t fd, const void *buffer, size_t size, size_t *bytes_written);
mk_status_t vfs_close(int32_t fd);
mk_status_t vfs_delete(const char *path);
mk_status_t vfs_stat(const char *path, mk_vfs_stat_t *out_stat);

#ifdef __cplusplus
}
#endif

#endif /* MK_VFS_H */
