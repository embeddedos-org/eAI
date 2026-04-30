// SPDX-License-Identifier: MIT
// HAL Timer implementation for Linux

#include "eai/platform.h"
#include <time.h>
#include <unistd.h>

#ifndef _WIN32

static uint64_t posix_get_time_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

static uint64_t posix_get_time_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000;
}

static void posix_sleep_ms(uint32_t ms)
{
    struct timespec ts = { .tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}

static void posix_sleep_us(uint32_t us)
{
    struct timespec ts = { .tv_sec = us / 1000000, .tv_nsec = (us % 1000000) * 1000L };
    nanosleep(&ts, NULL);
}

const eai_hal_timer_ops_t eai_hal_posix_timer_ops = {
    .get_time_ms = posix_get_time_ms,
    .get_time_us = posix_get_time_us,
    .sleep_ms    = posix_sleep_ms,
    .sleep_us    = posix_sleep_us,
};

#endif /* !_WIN32 */
