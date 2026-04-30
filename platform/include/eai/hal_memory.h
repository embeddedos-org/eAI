// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// HAL Memory sub-vtable — required for all platforms

#ifndef EAI_HAL_MEMORY_H
#define EAI_HAL_MEMORY_H

#include "eai/types.h"

struct eai_platform_s;

typedef struct {
    uint64_t total;
    uint64_t available;
    uint64_t heap_used;
    uint64_t heap_peak;
} eai_heap_stats_t;

typedef struct {
    eai_status_t (*get_memory_info)(struct eai_platform_s *plat, uint64_t *total, uint64_t *available);
    void        *(*alloc_aligned)(size_t size, size_t alignment);
    void         (*free_aligned)(void *ptr);
    eai_status_t (*get_heap_stats)(struct eai_platform_s *plat, eai_heap_stats_t *stats);
} eai_hal_memory_ops_t;

#endif /* EAI_HAL_MEMORY_H */
