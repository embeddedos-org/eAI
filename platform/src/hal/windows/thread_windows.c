// SPDX-License-Identifier: MIT
// HAL Thread implementation for Windows

#include "eai/platform.h"
#include <stdlib.h>

#ifdef _WIN32

#include <windows.h>

typedef struct {
    eai_thread_fn_t fn;
    void *arg;
} win_thread_ctx_t;

static DWORD WINAPI win_thread_trampoline(LPVOID ctx)
{
    win_thread_ctx_t *t = (win_thread_ctx_t *)ctx;
    t->fn(t->arg);
    free(t);
    return 0;
}

static eai_status_t win_thread_create(eai_thread_t *thread, eai_thread_fn_t fn, void *arg)
{
    win_thread_ctx_t *t = (win_thread_ctx_t *)malloc(sizeof(*t));
    if (!t) return EAI_ERR_NOMEM;
    t->fn = fn;
    t->arg = arg;

    HANDLE h = CreateThread(NULL, 0, win_thread_trampoline, t, 0, NULL);
    if (!h) { free(t); return EAI_ERR_RUNTIME; }
    *thread = (eai_thread_t)h;
    return EAI_OK;
}

static eai_status_t win_thread_join(eai_thread_t thread)
{
    HANDLE h = (HANDLE)thread;
    if (!h) return EAI_ERR_INVALID;
    DWORD rc = WaitForSingleObject(h, INFINITE);
    CloseHandle(h);
    return (rc == WAIT_OBJECT_0) ? EAI_OK : EAI_ERR_RUNTIME;
}

static eai_status_t win_mutex_create(eai_mutex_t *mutex)
{
    CRITICAL_SECTION *cs = (CRITICAL_SECTION *)malloc(sizeof(CRITICAL_SECTION));
    if (!cs) return EAI_ERR_NOMEM;
    InitializeCriticalSection(cs);
    *mutex = (eai_mutex_t)cs;
    return EAI_OK;
}

static eai_status_t win_mutex_lock(eai_mutex_t mutex)
{
    EnterCriticalSection((CRITICAL_SECTION *)mutex);
    return EAI_OK;
}

static eai_status_t win_mutex_unlock(eai_mutex_t mutex)
{
    LeaveCriticalSection((CRITICAL_SECTION *)mutex);
    return EAI_OK;
}

static void win_mutex_destroy(eai_mutex_t mutex)
{
    CRITICAL_SECTION *cs = (CRITICAL_SECTION *)mutex;
    if (cs) { DeleteCriticalSection(cs); free(cs); }
}

static eai_status_t win_semaphore_create(eai_semaphore_t *sem, int initial)
{
    HANDLE h = CreateSemaphoreW(NULL, initial, 0x7FFFFFFF, NULL);
    if (!h) return EAI_ERR_RUNTIME;
    *sem = (eai_semaphore_t)h;
    return EAI_OK;
}

static eai_status_t win_semaphore_wait(eai_semaphore_t sem)
{
    return (WaitForSingleObject((HANDLE)sem, INFINITE) == WAIT_OBJECT_0)
           ? EAI_OK : EAI_ERR_RUNTIME;
}

static eai_status_t win_semaphore_post(eai_semaphore_t sem)
{
    return ReleaseSemaphore((HANDLE)sem, 1, NULL) ? EAI_OK : EAI_ERR_RUNTIME;
}

static void win_semaphore_destroy(eai_semaphore_t sem)
{
    if (sem) CloseHandle((HANDLE)sem);
}

const eai_hal_thread_ops_t eai_hal_windows_thread_ops = {
    .thread_create     = win_thread_create,
    .thread_join       = win_thread_join,
    .mutex_create      = win_mutex_create,
    .mutex_lock        = win_mutex_lock,
    .mutex_unlock      = win_mutex_unlock,
    .mutex_destroy     = win_mutex_destroy,
    .semaphore_create  = win_semaphore_create,
    .semaphore_wait    = win_semaphore_wait,
    .semaphore_post    = win_semaphore_post,
    .semaphore_destroy = win_semaphore_destroy,
};

#endif /* _WIN32 */
