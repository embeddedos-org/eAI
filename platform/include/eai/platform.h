// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// ISO/IEC 25000 | ISO/IEC/IEEE 15288:2023

#ifndef EAI_PLATFORM_H
#define EAI_PLATFORM_H

#include "eai/types.h"

/* Include HAL sub-vtable headers (they use forward-declared struct eai_platform_s) */
#include "eai/hal_core.h"
#include "eai/hal_memory.h"
#include "eai/hal_gpio.h"
#include "eai/hal_thread.h"
#include "eai/hal_fs.h"
#include "eai/hal_net.h"
#include "eai/hal_timer.h"
#include "eai/hal_accel.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== Legacy flat vtable (backward compatibility) ========== */

typedef struct eai_platform_s eai_platform_t;

typedef struct {
    const char *name;
    eai_status_t (*init)(eai_platform_t *plat);
    eai_status_t (*get_device_info)(eai_platform_t *plat, char *buf, size_t buf_size);
    eai_status_t (*read_gpio)(eai_platform_t *plat, int pin, int *value);
    eai_status_t (*write_gpio)(eai_platform_t *plat, int pin, int value);
    eai_status_t (*get_memory_info)(eai_platform_t *plat, uint64_t *total, uint64_t *available);
    eai_status_t (*get_cpu_temp)(eai_platform_t *plat, float *temp_c);
    void         (*shutdown)(eai_platform_t *plat);
} eai_platform_ops_t;

/* ========== New composed HAL platform ========== */

typedef struct {
    const char                   *name;
    const eai_hal_core_ops_t     *core;     /* Required */
    const eai_hal_memory_ops_t   *memory;   /* Required */
    const eai_hal_gpio_ops_t     *gpio;     /* Nullable — desktops/containers omit */
    const eai_hal_thread_ops_t   *thread;   /* Nullable — bare-metal single-threaded omit */
    const eai_hal_fs_ops_t       *fs;       /* Nullable — MCUs without filesystem omit */
    const eai_hal_net_ops_t      *net;      /* Nullable — MCUs without networking omit */
    const eai_hal_timer_ops_t    *timer;    /* Nullable — but typically available */
    const eai_hal_accel_ops_t    *accel;    /* Nullable — platforms without GPU/NPU omit */
} eai_platform_hal_t;

/* ========== Platform instance ========== */

struct eai_platform_s {
    const eai_platform_ops_t *ops;          /* Legacy flat vtable */
    const eai_platform_hal_t *hal;          /* New composed HAL (NULL if using legacy) */
    void                     *ctx;          /* Platform-specific context */
    bool                      initialized;
};

/* ========== Core API ========== */

eai_status_t eai_platform_init(eai_platform_t *plat, const eai_platform_ops_t *ops);
eai_status_t eai_platform_init_hal(eai_platform_t *plat, const eai_platform_hal_t *hal);
eai_status_t eai_platform_detect(eai_platform_t *plat);
void         eai_platform_shutdown(eai_platform_t *plat);

/* Convenience: check if a HAL sub-module is available */
#define eai_hal_has_gpio(plat)   ((plat)->hal && (plat)->hal->gpio)
#define eai_hal_has_thread(plat) ((plat)->hal && (plat)->hal->thread)
#define eai_hal_has_fs(plat)     ((plat)->hal && (plat)->hal->fs)
#define eai_hal_has_net(plat)    ((plat)->hal && (plat)->hal->net)
#define eai_hal_has_timer(plat)  ((plat)->hal && (plat)->hal->timer)
#define eai_hal_has_accel(plat)  ((plat)->hal && (plat)->hal->accel)

/* Container detection */
#if !defined(_WIN32)
bool eai_platform_is_container(void);
#endif

/* ========== Platform ops externs ========== */

extern const eai_platform_ops_t eai_platform_linux_ops;
extern const eai_platform_ops_t eai_platform_windows_ops;
extern const eai_platform_ops_t eai_platform_container_ops;
extern const eai_platform_ops_t eai_platform_eos_ops;

#ifdef __cplusplus
}
#endif

#endif /* EAI_PLATFORM_H */
