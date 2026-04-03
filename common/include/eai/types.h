// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// ISO/IEC 25000 | ISO/IEC/IEEE 15288:2023

#ifndef EAI_TYPES_H
#define EAI_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum {
    EAI_OK = 0,
    EAI_ERR_NOMEM,
    EAI_ERR_INVALID,
    EAI_ERR_NOT_FOUND,
    EAI_ERR_IO,
    EAI_ERR_TIMEOUT,
    EAI_ERR_PERMISSION,
    EAI_ERR_RUNTIME,
    EAI_ERR_CONNECT,
    EAI_ERR_PROTOCOL,
    EAI_ERR_CONFIG,
    EAI_ERR_UNSUPPORTED,
    EAI_ERR_LEARNING,
    EAI_ERR_ADAPTER,
    EAI_ERR_STALE_MODEL,
    EAI_ERR_RESOURCE_BUDGET,
    EAI_ERR_BCI_SIGNAL,
    EAI_ERR_BCI_CALIBRATION,
    EAI_ERR_BCI_DECODE,
} eai_status_t;

typedef enum {
    EAI_VARIANT_MIN,
    EAI_VARIANT_FRAMEWORK,
} eai_variant_t;

typedef enum {
    EAI_MODE_LOCAL,
    EAI_MODE_CLOUD,
    EAI_MODE_HYBRID,
} eai_mode_t;

typedef enum {
    EAI_LVL_TRACE,
    EAI_LVL_DEBUG,
    EAI_LVL_INFO,
    EAI_LVL_WARN,
    EAI_LVL_ERROR,
    EAI_LVL_FATAL,
} eai_log_level_t;

typedef struct {
    const char *key;
    const char *value;
} eai_kv_t;

typedef struct {
    uint8_t *data;
    size_t   len;
    size_t   cap;
} eai_buffer_t;

const char *eai_status_str(eai_status_t status);

#endif /* EAI_TYPES_H */
