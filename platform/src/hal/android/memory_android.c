// SPDX-License-Identifier: MIT
// HAL Memory implementation for Android

#include "eai/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__ANDROID__)

static eai_status_t android_hal_get_memory_info(eai_platform_t *plat,
                                                 uint64_t *total, uint64_t *available)
{
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) return EAI_ERR_IO;

    char line[256];
    uint64_t t = 0, a = 0;
    while (fgets(line, sizeof(line), fp)) {
        unsigned long val = 0;
        if (sscanf(line, "MemTotal: %lu kB", &val) == 1)
            t = (uint64_t)val * 1024;
        else if (sscanf(line, "MemAvailable: %lu kB", &val) == 1)
            a = (uint64_t)val * 1024;
    }
    fclose(fp);

    if (total) *total = t;
    if (available) *available = a;
    return EAI_OK;
}

static void *android_hal_alloc_aligned(size_t size, size_t alignment)
{
    void *ptr = NULL;
    if (posix_memalign(&ptr, alignment, size) != 0) return NULL;
    return ptr;
}

static void android_hal_free_aligned(void *ptr)
{
    free(ptr);
}

static eai_status_t android_hal_get_heap_stats(eai_platform_t *plat, eai_heap_stats_t *stats)
{
    if (!stats) return EAI_ERR_INVALID;
    memset(stats, 0, sizeof(*stats));
    return EAI_ERR_NOT_IMPLEMENTED;
}

const eai_hal_memory_ops_t eai_hal_android_memory_ops = {
    .get_memory_info = android_hal_get_memory_info,
    .alloc_aligned   = android_hal_alloc_aligned,
    .free_aligned    = android_hal_free_aligned,
    .get_heap_stats  = android_hal_get_heap_stats,
};

#endif /* __ANDROID__ */
