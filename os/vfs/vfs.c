/**
 * @file vfs.c
 * @brief OS-owned Virtual File System implementation over 'osfs'.
 */

#include "vfs.h"
#include "fault_manager.h"
#include <string.h>

#define VFS_MAX_FILES           16U
#define VFS_DIR_SECTOR          0U

typedef struct {
    char name[VFS_MAX_FILENAME_LEN];
    uint32_t start_sector;
    uint32_t size_bytes;
    uint32_t flags;
    uint32_t in_use;
} vfs_inode_t;

typedef struct {
    uint32_t magic;
    uint32_t file_count;
    vfs_inode_t inodes[VFS_MAX_FILES];
    uint32_t crc32;
} vfs_superblock_t;

typedef struct {
    int32_t inode_idx;
    uint32_t cursor;
    uint32_t flags;
    bool is_open;
} vfs_file_desc_t;

static vfs_superblock_t s_superblock;
static vfs_file_desc_t s_open_files[VFS_MAX_OPEN_FILES];
static bool s_vfs_ready = false;

static void vfs_sync_superblock(void)
{
    s_superblock.magic = VFS_BLOCK_MAGIC;
    s_superblock.crc32 = vfs_block_calc_crc32(
        (const uint8_t *)&s_superblock,
        sizeof(vfs_superblock_t) - sizeof(uint32_t)
    );
    (void)vfs_block_erase(VFS_DIR_SECTOR);
    (void)vfs_block_write(VFS_DIR_SECTOR, &s_superblock, sizeof(vfs_superblock_t));
}

static void create_default_files(void)
{
    /* Seed default macro file for BLE HID */
    const char *default_macro = 
        "STRING Hello MicroKernel\n"
        "DELAY 200\n"
        "ENTER\n";

    strncpy(s_superblock.inodes[0].name, "macro.txt", VFS_MAX_FILENAME_LEN - 1);
    s_superblock.inodes[0].start_sector = 1U;
    s_superblock.inodes[0].size_bytes = (uint32_t)strlen(default_macro);
    s_superblock.inodes[0].in_use = 1U;
    s_superblock.inodes[0].flags = 0U;

    (void)vfs_block_erase(1U);
    (void)vfs_block_write(1U, default_macro, strlen(default_macro));

    /* Seed default audit log for Wi-Fi */
    strncpy(s_superblock.inodes[1].name, "wifi_audit.log", VFS_MAX_FILENAME_LEN - 1);
    s_superblock.inodes[1].start_sector = 2U;
    s_superblock.inodes[1].size_bytes = 0U;
    s_superblock.inodes[1].in_use = 1U;
    s_superblock.inodes[1].flags = 0U;
    (void)vfs_block_erase(2U);

    s_superblock.file_count = 2U;
    vfs_sync_superblock();
}

mk_status_t vfs_init(void)
{
    memset(s_open_files, 0, sizeof(s_open_files));
    memset(&s_superblock, 0, sizeof(s_superblock));

    mk_status_t status = vfs_block_init();
    if (status != MK_STATUS_OK) {
        mk_fault_record_full(MK_FAULT_VFS_FAIL, MK_FAULT_SRC_VFS, MK_FAULT_SEV_CRITICAL, 0x10, 0U);
        return status;
    }

    /* Read Superblock from Sector 0 */
    status = vfs_block_read(VFS_DIR_SECTOR, &s_superblock, sizeof(vfs_superblock_t));
    if (status != MK_STATUS_OK || s_superblock.magic != VFS_BLOCK_MAGIC) {
        /* Format new file system */
        memset(&s_superblock, 0, sizeof(s_superblock));
        s_superblock.magic = VFS_BLOCK_MAGIC;
        create_default_files();
    }

    s_vfs_ready = true;
    return MK_STATUS_OK;
}

mk_status_t vfs_open(const char *path, uint32_t flags, int32_t *out_fd)
{
    if (!s_vfs_ready || path == NULL || out_fd == NULL) {
        return MK_STATUS_INVALID_ARG;
    }

    /* Strip leading slash */
    const char *fname = (path[0] == '/') ? &path[1] : path;

    /* Find existing inode */
    int32_t target_inode = -1;
    for (uint32_t i = 0; i < VFS_MAX_FILES; i++) {
        if (s_superblock.inodes[i].in_use && strncmp(s_superblock.inodes[i].name, fname, VFS_MAX_FILENAME_LEN) == 0) {
            target_inode = (int32_t)i;
            break;
        }
    }

    /* Create file if requested and not found */
    if (target_inode < 0) {
        if (!(flags & VFS_O_CREAT)) {
            return MK_STATUS_NOT_FOUND;
        }

        for (uint32_t i = 0; i < VFS_MAX_FILES; i++) {
            if (!s_superblock.inodes[i].in_use) {
                target_inode = (int32_t)i;
                strncpy(s_superblock.inodes[i].name, fname, VFS_MAX_FILENAME_LEN - 1);
                s_superblock.inodes[i].start_sector = 3U + i; /* Dynamic sector offset */
                s_superblock.inodes[i].size_bytes = 0U;
                s_superblock.inodes[i].in_use = 1U;
                s_superblock.inodes[i].flags = flags;
                s_superblock.file_count++;
                (void)vfs_block_erase(s_superblock.inodes[i].start_sector);
                vfs_sync_superblock();
                break;
            }
        }

        if (target_inode < 0) {
            return MK_STATUS_NO_MEMORY;
        }
    }

    /* Allocate free file descriptor */
    int32_t fd = -1;
    for (int32_t i = 0; i < (int32_t)VFS_MAX_OPEN_FILES; i++) {
        if (!s_open_files[i].is_open) {
            fd = i;
            break;
        }
    }

    if (fd < 0) {
        return MK_STATUS_BUSY;
    }

    s_open_files[fd].inode_idx = target_inode;
    s_open_files[fd].flags = flags;
    s_open_files[fd].cursor = (flags & VFS_O_APPEND) ? s_superblock.inodes[target_inode].size_bytes : 0U;
    s_open_files[fd].is_open = true;

    if (flags & VFS_O_TRUNC) {
        s_superblock.inodes[target_inode].size_bytes = 0U;
        s_open_files[fd].cursor = 0U;
        (void)vfs_block_erase(s_superblock.inodes[target_inode].start_sector);
        vfs_sync_superblock();
    }

    *out_fd = fd;
    return MK_STATUS_OK;
}

mk_status_t vfs_read(int32_t fd, void *buffer, size_t size, size_t *bytes_read)
{
    if (!s_vfs_ready || buffer == NULL || bytes_read == NULL || fd < 0 || fd >= (int32_t)VFS_MAX_OPEN_FILES) {
        return MK_STATUS_INVALID_ARG;
    }

    vfs_file_desc_t *desc = &s_open_files[fd];
    if (!desc->is_open) {
        return MK_STATUS_INVALID_STATE;
    }

    vfs_inode_t *inode = &s_superblock.inodes[desc->inode_idx];
    if (desc->cursor >= inode->size_bytes) {
        *bytes_read = 0;
        return MK_STATUS_OK; /* EOF */
    }

    size_t to_read = size;
    if ((desc->cursor + to_read) > inode->size_bytes) {
        to_read = inode->size_bytes - desc->cursor;
    }

    /* Read from sector */
    static uint8_t s_sector_buf[VFS_SECTOR_SIZE];
    mk_status_t st = vfs_block_read(inode->start_sector, s_sector_buf, VFS_SECTOR_SIZE);
    if (st != MK_STATUS_OK) {
        return st;
    }

    memcpy(buffer, &s_sector_buf[desc->cursor], to_read);
    desc->cursor += (uint32_t)to_read;
    *bytes_read = to_read;

    return MK_STATUS_OK;
}

mk_status_t vfs_write(int32_t fd, const void *buffer, size_t size, size_t *bytes_written)
{
    if (!s_vfs_ready || buffer == NULL || bytes_written == NULL || fd < 0 || fd >= (int32_t)VFS_MAX_OPEN_FILES) {
        return MK_STATUS_INVALID_ARG;
    }

    vfs_file_desc_t *desc = &s_open_files[fd];
    if (!desc->is_open || (desc->flags & VFS_O_RDONLY)) {
        return MK_STATUS_INVALID_STATE;
    }

    vfs_inode_t *inode = &s_superblock.inodes[desc->inode_idx];
    if ((desc->cursor + size) > VFS_SECTOR_SIZE) {
        return MK_STATUS_NO_MEMORY;
    }

    static uint8_t s_sector_buf[VFS_SECTOR_SIZE];
    (void)vfs_block_read(inode->start_sector, s_sector_buf, VFS_SECTOR_SIZE);
    memcpy(&s_sector_buf[desc->cursor], buffer, size);

    (void)vfs_block_erase(inode->start_sector);
    mk_status_t st = vfs_block_write(inode->start_sector, s_sector_buf, desc->cursor + size);
    if (st != MK_STATUS_OK) {
        return st;
    }

    desc->cursor += (uint32_t)size;
    if (desc->cursor > inode->size_bytes) {
        inode->size_bytes = desc->cursor;
        vfs_sync_superblock();
    }

    *bytes_written = size;
    return MK_STATUS_OK;
}

mk_status_t vfs_close(int32_t fd)
{
    if (fd < 0 || fd >= (int32_t)VFS_MAX_OPEN_FILES || !s_open_files[fd].is_open) {
        return MK_STATUS_INVALID_ARG;
    }

    s_open_files[fd].is_open = false;
    return MK_STATUS_OK;
}

mk_status_t vfs_delete(const char *path)
{
    if (!s_vfs_ready || path == NULL) {
        return MK_STATUS_INVALID_ARG;
    }

    const char *fname = (path[0] == '/') ? &path[1] : path;

    for (uint32_t i = 0; i < VFS_MAX_FILES; i++) {
        if (s_superblock.inodes[i].in_use && strncmp(s_superblock.inodes[i].name, fname, VFS_MAX_FILENAME_LEN) == 0) {
            s_superblock.inodes[i].in_use = 0U;
            (void)vfs_block_erase(s_superblock.inodes[i].start_sector);
            s_superblock.file_count--;
            vfs_sync_superblock();
            return MK_STATUS_OK;
        }
    }

    return MK_STATUS_NOT_FOUND;
}

mk_status_t vfs_stat(const char *path, mk_vfs_stat_t *out_stat)
{
    if (!s_vfs_ready || path == NULL || out_stat == NULL) {
        return MK_STATUS_INVALID_ARG;
    }

    const char *fname = (path[0] == '/') ? &path[1] : path;

    for (uint32_t i = 0; i < VFS_MAX_FILES; i++) {
        if (s_superblock.inodes[i].in_use && strncmp(s_superblock.inodes[i].name, fname, VFS_MAX_FILENAME_LEN) == 0) {
            out_stat->size = s_superblock.inodes[i].size_bytes;
            out_stat->flags = s_superblock.inodes[i].flags;
            out_stat->created_tick = 0U;
            out_stat->modified_tick = 0U;
            return MK_STATUS_OK;
        }
    }

    return MK_STATUS_NOT_FOUND;
}
