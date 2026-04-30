// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// HAL Timer sub-vtable — typically available on all platforms

#ifndef EAI_HAL_TIMER_H
#define EAI_HAL_TIMER_H

#include "eai/types.h"

typedef struct {
    uint64_t     (*get_time_ms)(void);
    uint64_t     (*get_time_us)(void);
    void         (*sleep_ms)(uint32_t ms);
    void         (*sleep_us)(uint32_t us);
} eai_hal_timer_ops_t;

#endif /* EAI_HAL_TIMER_H */
