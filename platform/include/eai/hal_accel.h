// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// HAL Accelerator sub-vtable — nullable (platforms without GPU/NPU/DSP omit this)

#ifndef EAI_HAL_ACCEL_H
#define EAI_HAL_ACCEL_H

#include "eai/types.h"

typedef enum {
    EAI_ACCEL_TYPE_CPU  = 0,
    EAI_ACCEL_TYPE_GPU  = 1,
    EAI_ACCEL_TYPE_NPU  = 2,
    EAI_ACCEL_TYPE_DSP  = 3,
} eai_accel_type_t;

typedef struct {
    char             name[64];
    eai_accel_type_t type;
    uint64_t         memory_bytes;
    uint32_t         compute_units;
} eai_accel_info_t;

typedef struct {
    int          (*enumerate_devices)(eai_accel_info_t *devices, int max_devices);
    uint32_t     (*get_capabilities)(int device_id);
    eai_status_t (*get_info)(int device_id, eai_accel_info_t *info);
} eai_hal_accel_ops_t;

#endif /* EAI_HAL_ACCEL_H */
