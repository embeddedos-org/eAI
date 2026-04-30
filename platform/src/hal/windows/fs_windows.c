// SPDX-License-Identifier: MIT
// HAL Filesystem implementation for Windows

#include "eai/platform.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32

#include <windows.h>
#include <direct.h>
#include <sys/stat.h>

static eai_status_t win_file_open(eai_file_t *file, const char *path, int flags)
{
    const char *mode;
    if ((flags & EAI_FILE_APPEND) && (flags & EAI_FILE_READ))
        mode = "a+b";
    else if (flags & EAI_FILE_APPEND)
        mode = "ab";
    else if ((flags & EAI_FILE_WRITE) && (flags & EAI_FILE_READ))
        mode = (flags & EAI_FILE_CREATE) ? "w+b" : "r+b";
    else if (flags & EAI_FILE_WRITE)
        mode = "wb";
    else
        mode = "rb";

    FILE *fp = fopen(path, mode);
    if (!fp) return EAI_ERR_IO;
    *file = (eai_file_t)fp;
    return EAI_OK;
}

static eai_status_t win_file_read(eai_file_t file, void *buf, size_t size, size_t *bytes_read)
{
    size_t n = fread(buf, 1, size, (FILE *)file);
    if (bytes_read) *bytes_read = n;
    return (n > 0 || feof((FILE *)file)) ? EAI_OK : EAI_ERR_IO;
}

static eai_status_t win_file_write(eai_file_t file, const void *buf, size_t size, size_t *bytes_written)
{
    size_t n = fwrite(buf, 1, size, (FILE *)file);
    if (bytes_written) *bytes_written = n;
    return (n == size) ? EAI_OK : EAI_ERR_IO;
}

static void win_file_close(eai_file_t file)
{
    if (file) fclose((FILE *)file);
}

static eai_status_t win_file_size(const char *path, uint64_t *size)
{
    struct _stat64 st;
    if (_stat64(path, &st) != 0) return EAI_ERR_IO;
    *size = (uint64_t)st.st_size;
    return EAI_OK;
}

static bool win_file_exists(const char *path)
{
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES);
}

static eai_status_t win_dir_create(const char *path)
{
    if (_mkdir(path) != 0) {
        DWORD err = GetLastError();
        if (err != ERROR_ALREADY_EXISTS) return EAI_ERR_IO;
    }
    return EAI_OK;
}

const eai_hal_fs_ops_t eai_hal_windows_fs_ops = {
    .file_open    = win_file_open,
    .file_read    = win_file_read,
    .file_write   = win_file_write,
    .file_close   = win_file_close,
    .file_size    = win_file_size,
    .file_exists  = win_file_exists,
    .dir_create   = win_dir_create,
};

#endif /* _WIN32 */
