// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// HAL Filesystem sub-vtable — nullable (MCUs/bare-metal may omit)

#ifndef EAI_HAL_FS_H
#define EAI_HAL_FS_H

#include "eai/types.h"

typedef void *eai_file_t;

typedef enum {
    EAI_FILE_READ   = 0x01,
    EAI_FILE_WRITE  = 0x02,
    EAI_FILE_APPEND = 0x04,
    EAI_FILE_CREATE = 0x08,
} eai_file_flags_t;

typedef struct {
    eai_status_t (*file_open)(eai_file_t *file, const char *path, int flags);
    eai_status_t (*file_read)(eai_file_t file, void *buf, size_t size, size_t *bytes_read);
    eai_status_t (*file_write)(eai_file_t file, const void *buf, size_t size, size_t *bytes_written);
    void         (*file_close)(eai_file_t file);
    eai_status_t (*file_size)(const char *path, uint64_t *size);
    bool         (*file_exists)(const char *path);
    eai_status_t (*dir_create)(const char *path);
} eai_hal_fs_ops_t;

#endif /* EAI_HAL_FS_H */
