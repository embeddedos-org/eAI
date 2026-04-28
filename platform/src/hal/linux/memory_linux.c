// SPDX-License-Identifier: MIT
// HAL Memory implementation for Linux

#include "eai/platform.h"
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#ifdef __linux__
#include <sys/sysinfo.h>

static eai_status_t linux_hal_get_memory_info(eai_platform_t *plat,
                                               uint64_t *total, uint64_t *available)
{
    struct sysinfo si;
    if (sysinfo(&si) != 0) return EAI_ERR_IO;
    if (total)     *total     = (uint64_t)si.totalram * si.mem_unit;
    if (available) *available = (uint64_t)si.freeram  * si.mem_unit;
    return EAI_OK;
}

static void *linux_hal_alloc_aligned(size_t size, size_t alignment)
{
    void *ptr = NULL;
    if (posix_memalign(&ptr, alignment, size) != 0) return NULL;
    return ptr;
}

static void linux_hal_free_aligned(void *ptr)
{
    free(ptr);
}

static eai_status_t linux_hal_get_heap_stats(eai_platform_t *plat, eai_heap_stats_t *stats)
{
    (void)plat;
    if (!stats) return EAI_ERR_INVALID;
    memset(stats, 0, sizeof(*stats));
    return EAI_ERR_NOT_IMPLEMENTED;
}

const eai_hal_memory_ops_t eai_hal_linux_memory_ops = {
    .get_memory_info = linux_hal_get_memory_info,
    .alloc_aligned   = linux_hal_alloc_aligned,
    .free_aligned    = linux_hal_free_aligned,
    .get_heap_stats  = linux_hal_get_heap_stats,
};

#endif /* __linux__ */
#endif /* !_WIN32 */
