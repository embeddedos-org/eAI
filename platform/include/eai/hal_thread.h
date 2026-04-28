// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// HAL Thread sub-vtable — nullable (bare-metal single-threaded omit this)

#ifndef EAI_HAL_THREAD_H
#define EAI_HAL_THREAD_H

#include "eai/types.h"

typedef void *eai_thread_t;
typedef void *eai_mutex_t;
typedef void *eai_semaphore_t;
typedef void (*eai_thread_fn_t)(void *arg);

typedef struct {
    eai_status_t (*thread_create)(eai_thread_t *thread, eai_thread_fn_t fn, void *arg);
    eai_status_t (*thread_join)(eai_thread_t thread);

    eai_status_t (*mutex_create)(eai_mutex_t *mutex);
    eai_status_t (*mutex_lock)(eai_mutex_t mutex);
    eai_status_t (*mutex_unlock)(eai_mutex_t mutex);
    void         (*mutex_destroy)(eai_mutex_t mutex);

    eai_status_t (*semaphore_create)(eai_semaphore_t *sem, int initial);
    eai_status_t (*semaphore_wait)(eai_semaphore_t sem);
    eai_status_t (*semaphore_post)(eai_semaphore_t sem);
    void         (*semaphore_destroy)(eai_semaphore_t sem);
} eai_hal_thread_ops_t;

#endif /* EAI_HAL_THREAD_H */
