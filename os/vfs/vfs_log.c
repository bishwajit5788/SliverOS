/**
 * @file vfs_log.c
 * @brief Structured append-only audit logger with bounded sector rotation.
 */
#include "vfs_log.h"
#include <stdio.h>
#include <string.h>

#define VFS_LOG_ROTATION_SLOTS 4U
#define VFS_LOG_ROTATE_AT      (VFS_SECTOR_SIZE - 512U)

static mk_status_t make_slot_name(const char *base, uint32_t slot, char *out, size_t out_len)
{
    if (base == NULL || out == NULL || out_len == 0U) return MK_STATUS_INVALID_ARG;
    int n = snprintf(out, out_len, "%s.%u", base, (unsigned)slot);
    return (n > 0 && (size_t)n < out_len) ? MK_STATUS_OK : MK_STATUS_INVALID_ARG;
}

static mk_status_t ensure_slot(const char *base, uint32_t slot, int32_t *out_fd)
{
    char name[VFS_MAX_FILENAME_LEN];
    mk_status_t st = make_slot_name(base, slot, name, sizeof(name));
    if (st != MK_STATUS_OK) return st;
    return vfs_open(name, VFS_O_CREAT | VFS_O_WRONLY | VFS_O_APPEND, out_fd);
}

mk_status_t vfs_log_init(const char *log_filename)
{
    if (log_filename == NULL) return MK_STATUS_INVALID_ARG;
    int32_t fd = -1;
    mk_status_t st = ensure_slot(log_filename, 0U, &fd);
    if (st == MK_STATUS_OK) (void)vfs_close(fd);
    return st;
}

mk_status_t vfs_log_append(const char *log_filename, const char *entry)
{
    if (log_filename == NULL || entry == NULL) return MK_STATUS_INVALID_ARG;

    uint32_t slot = 0U;
    uint32_t first_free = VFS_LOG_ROTATION_SLOTS;
    uint32_t smallest_size = UINT32_MAX;
    uint32_t oldest_slot = 0U;

    for (uint32_t i = 0U; i < VFS_LOG_ROTATION_SLOTS; ++i) {
        char name[VFS_MAX_FILENAME_LEN];
        if (make_slot_name(log_filename, i, name, sizeof(name)) != MK_STATUS_OK) return MK_STATUS_INVALID_ARG;
        mk_vfs_stat_t st;
        if (vfs_stat(name, &st) != MK_STATUS_OK) {
            if (first_free == VFS_LOG_ROTATION_SLOTS) first_free = i;
            continue;
        }
        if (st.size < smallest_size) { smallest_size = st.size; oldest_slot = i; }
        if (i == 0U && st.size < VFS_LOG_ROTATE_AT) slot = i;
    }

    if (first_free != VFS_LOG_ROTATION_SLOTS) {
        slot = first_free;
    } else {
        /* Continue the first non-full segment; otherwise rotate to the smallest
         * segment, which is the oldest after normal sequential rotation. */
        bool found_nonfull = false;
        for (uint32_t i = 0U; i < VFS_LOG_ROTATION_SLOTS; ++i) {
            char name[VFS_MAX_FILENAME_LEN];
            if (make_slot_name(log_filename, i, name, sizeof(name)) != MK_STATUS_OK) return MK_STATUS_INVALID_ARG;
            mk_vfs_stat_t st;
            if (vfs_stat(name, &st) == MK_STATUS_OK && st.size < VFS_LOG_ROTATE_AT) {
                slot = i; found_nonfull = true; break;
            }
        }
        if (!found_nonfull) slot = (oldest_slot + 1U) % VFS_LOG_ROTATION_SLOTS;
    }

    char name[VFS_MAX_FILENAME_LEN];
    mk_status_t st = make_slot_name(log_filename, slot, name, sizeof(name));
    if (st != MK_STATUS_OK) return st;

    mk_vfs_stat_t stat_buf;
    if (vfs_stat(name, &stat_buf) == MK_STATUS_OK && stat_buf.size >= VFS_LOG_ROTATE_AT) {
        int32_t clear_fd = -1;
        st = vfs_open(name, VFS_O_WRONLY | VFS_O_TRUNC, &clear_fd);
        if (st == MK_STATUS_OK) (void)vfs_close(clear_fd);
        if (st != MK_STATUS_OK) return st;
    }

    int32_t fd = -1;
    st = vfs_open(name, VFS_O_CREAT | VFS_O_WRONLY | VFS_O_APPEND, &fd);
    if (st != MK_STATUS_OK) return st;
    size_t written = 0U;
    st = vfs_write(fd, entry, strlen(entry), &written);
    (void)vfs_close(fd);
    return st;
}

mk_status_t vfs_log_clear(const char *log_filename)
{
    if (log_filename == NULL) return MK_STATUS_INVALID_ARG;
    mk_status_t final = MK_STATUS_OK;
    for (uint32_t i = 0U; i < VFS_LOG_ROTATION_SLOTS; ++i) {
        char name[VFS_MAX_FILENAME_LEN];
        if (make_slot_name(log_filename, i, name, sizeof(name)) != MK_STATUS_OK) return MK_STATUS_INVALID_ARG;
        int32_t fd = -1;
        mk_status_t st = vfs_open(name, VFS_O_CREAT | VFS_O_WRONLY | VFS_O_TRUNC, &fd);
        if (st == MK_STATUS_OK) (void)vfs_close(fd); else if (st != MK_STATUS_NOT_FOUND) final = st;
    }
    return final;
}
