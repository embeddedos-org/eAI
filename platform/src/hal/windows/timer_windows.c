// SPDX-License-Identifier: MIT
// HAL Timer implementation for Windows

#include "eai/platform.h"

#ifdef _WIN32

#include <windows.h>

static uint64_t win_get_time_ms(void)
{
    LARGE_INTEGER freq, count;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    return (uint64_t)(count.QuadPart * 1000 / freq.QuadPart);
}

static uint64_t win_get_time_us(void)
{
    LARGE_INTEGER freq, count;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    return (uint64_t)(count.QuadPart * 1000000 / freq.QuadPart);
}

static void win_sleep_ms(uint32_t ms)
{
    Sleep(ms);
}

static void win_sleep_us(uint32_t us)
{
    /* Windows doesn't have sub-ms sleep natively; use busy-wait for short durations */
    if (us >= 1000) {
        Sleep(us / 1000);
    } else {
        LARGE_INTEGER freq, start, now;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&start);
        uint64_t target = (uint64_t)us * freq.QuadPart / 1000000;
        do {
            QueryPerformanceCounter(&now);
        } while ((uint64_t)(now.QuadPart - start.QuadPart) < target);
    }
}

const eai_hal_timer_ops_t eai_hal_windows_timer_ops = {
    .get_time_ms = win_get_time_ms,
    .get_time_us = win_get_time_us,
    .sleep_ms    = win_sleep_ms,
    .sleep_us    = win_sleep_us,
};

#endif /* _WIN32 */
