// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// HAL Core sub-vtable — required for all platforms

#ifndef EAI_HAL_CORE_H
#define EAI_HAL_CORE_H

#include "eai/types.h"

struct eai_platform_s;

typedef struct {
    eai_status_t (*init)(struct eai_platform_s *plat);
    void         (*shutdown)(struct eai_platform_s *plat);
    eai_status_t (*get_device_info)(struct eai_platform_s *plat, char *buf, size_t buf_size);
    eai_status_t (*get_os_name)(struct eai_platform_s *plat, char *buf, size_t buf_size);
    eai_status_t (*get_cpu_temp)(struct eai_platform_s *plat, float *temp_c);
    int          (*get_cpu_count)(struct eai_platform_s *plat);
} eai_hal_core_ops_t;

#endif /* EAI_HAL_CORE_H */
