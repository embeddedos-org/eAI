// SPDX-License-Identifier: MIT
// HAL Memory implementation for Windows

#include "eai/platform.h"
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32

#include <windows.h>

static eai_status_t win_hal_get_memory_info(eai_platform_t *plat,
                                             uint64_t *total, uint64_t *available)
{
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (!GlobalMemoryStatusEx(&ms)) return EAI_ERR_IO;
    if (total)     *total     = ms.ullTotalPhys;
    if (available) *available = ms.ullAvailPhys;
    return EAI_OK;
}

static void *win_hal_alloc_aligned(size_t size, size_t alignment)
{
    return _aligned_malloc(size, alignment);
}

static void win_hal_free_aligned(void *ptr)
{
    _aligned_free(ptr);
}

static eai_status_t win_hal_get_heap_stats(eai_platform_t *plat, eai_heap_stats_t *stats)
{
    (void)plat;
    if (!stats) return EAI_ERR_INVALID;
    memset(stats, 0, sizeof(*stats));
    return EAI_ERR_NOT_IMPLEMENTED;
}

const eai_hal_memory_ops_t eai_hal_windows_memory_ops = {
    .get_memory_info = win_hal_get_memory_info,
    .alloc_aligned   = win_hal_alloc_aligned,
    .free_aligned    = win_hal_free_aligned,
    .get_heap_stats  = win_hal_get_heap_stats,
};

#endif /* _WIN32 */
