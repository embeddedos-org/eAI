// SPDX-License-Identifier: MIT
// HAL Thread implementation for POSIX (Linux, macOS, Android)

#include "eai/platform.h"
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>

#ifndef _WIN32

typedef struct {
    eai_thread_fn_t fn;
    void *arg;
} thread_trampoline_t;

static void *thread_trampoline(void *ctx)
{
    thread_trampoline_t *t = (thread_trampoline_t *)ctx;
    t->fn(t->arg);
    free(t);
    return NULL;
}

static eai_status_t posix_thread_create(eai_thread_t *thread, eai_thread_fn_t fn, void *arg)
{
    thread_trampoline_t *t = (thread_trampoline_t *)malloc(sizeof(*t));
    if (!t) return EAI_ERR_NOMEM;
    t->fn = fn;
    t->arg = arg;

    pthread_t *pt = (pthread_t *)malloc(sizeof(pthread_t));
    if (!pt) { free(t); return EAI_ERR_NOMEM; }

    if (pthread_create(pt, NULL, thread_trampoline, t) != 0) {
        free(t);
        free(pt);
        return EAI_ERR_RUNTIME;
    }
    *thread = (eai_thread_t)pt;
    return EAI_OK;
}

static eai_status_t posix_thread_join(eai_thread_t thread)
{
    pthread_t *pt = (pthread_t *)thread;
    if (!pt) return EAI_ERR_INVALID;
    int rc = pthread_join(*pt, NULL);
    free(pt);
    return (rc == 0) ? EAI_OK : EAI_ERR_RUNTIME;
}

static eai_status_t posix_mutex_create(eai_mutex_t *mutex)
{
    pthread_mutex_t *m = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    if (!m) return EAI_ERR_NOMEM;
    if (pthread_mutex_init(m, NULL) != 0) { free(m); return EAI_ERR_RUNTIME; }
    *mutex = (eai_mutex_t)m;
    return EAI_OK;
}

static eai_status_t posix_mutex_lock(eai_mutex_t mutex)
{
    return (pthread_mutex_lock((pthread_mutex_t *)mutex) == 0) ? EAI_OK : EAI_ERR_RUNTIME;
}

static eai_status_t posix_mutex_unlock(eai_mutex_t mutex)
{
    return (pthread_mutex_unlock((pthread_mutex_t *)mutex) == 0) ? EAI_OK : EAI_ERR_RUNTIME;
}

static void posix_mutex_destroy(eai_mutex_t mutex)
{
    pthread_mutex_t *m = (pthread_mutex_t *)mutex;
    if (m) { pthread_mutex_destroy(m); free(m); }
}

static eai_status_t posix_semaphore_create(eai_semaphore_t *sem, int initial)
{
    sem_t *s = (sem_t *)malloc(sizeof(sem_t));
    if (!s) return EAI_ERR_NOMEM;
    if (sem_init(s, 0, initial) != 0) { free(s); return EAI_ERR_RUNTIME; }
    *sem = (eai_semaphore_t)s;
    return EAI_OK;
}

static eai_status_t posix_semaphore_wait(eai_semaphore_t sem)
{
    return (sem_wait((sem_t *)sem) == 0) ? EAI_OK : EAI_ERR_RUNTIME;
}

static eai_status_t posix_semaphore_post(eai_semaphore_t sem)
{
    return (sem_post((sem_t *)sem) == 0) ? EAI_OK : EAI_ERR_RUNTIME;
}

static void posix_semaphore_destroy(eai_semaphore_t sem)
{
    sem_t *s = (sem_t *)sem;
    if (s) { sem_destroy(s); free(s); }
}

const eai_hal_thread_ops_t eai_hal_posix_thread_ops = {
    .thread_create     = posix_thread_create,
    .thread_join       = posix_thread_join,
    .mutex_create      = posix_mutex_create,
    .mutex_lock        = posix_mutex_lock,
    .mutex_unlock      = posix_mutex_unlock,
    .mutex_destroy     = posix_mutex_destroy,
    .semaphore_create  = posix_semaphore_create,
    .semaphore_wait    = posix_semaphore_wait,
    .semaphore_post    = posix_semaphore_post,
    .semaphore_destroy = posix_semaphore_destroy,
};

#endif /* !_WIN32 */
