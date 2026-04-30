// SPDX-License-Identifier: MIT
// Tests for HAL Core sub-vtable

#include <stdio.h>
#include <string.h>
#include "eai/platform.h"

static int tests_run = 0, tests_passed = 0, tests_failed = 0;
#define TEST(name) do { tests_run++; printf("  TEST %-40s ", #name); } while(0)
#define PASS() do { tests_passed++; printf("[PASS]\n"); } while(0)
#define FAIL(msg) do { tests_failed++; printf("[FAIL] %s\n", msg); } while(0)

/* Extern declarations for HAL ops */
#ifdef _WIN32
extern const eai_hal_core_ops_t eai_hal_windows_core_ops;
extern const eai_hal_memory_ops_t eai_hal_windows_memory_ops;
extern const eai_hal_timer_ops_t eai_hal_windows_timer_ops;
#define CORE_OPS   eai_hal_windows_core_ops
#define MEM_OPS    eai_hal_windows_memory_ops
#define TIMER_OPS  eai_hal_windows_timer_ops
#elif defined(__linux__)
extern const eai_hal_core_ops_t eai_hal_linux_core_ops;
extern const eai_hal_memory_ops_t eai_hal_linux_memory_ops;
extern const eai_hal_timer_ops_t eai_hal_posix_timer_ops;
#define CORE_OPS   eai_hal_linux_core_ops
#define MEM_OPS    eai_hal_linux_memory_ops
#define TIMER_OPS  eai_hal_posix_timer_ops
#endif

static void test_hal_core_init(void)
{
    TEST(hal_core_init);
    eai_platform_t plat;
    memset(&plat, 0, sizeof(plat));

    eai_status_t st = CORE_OPS.init(&plat);
    if (st != EAI_OK) { FAIL("init failed"); return; }

    CORE_OPS.shutdown(&plat);
    PASS();
}

static void test_hal_core_device_info(void)
{
    TEST(hal_core_device_info);
    eai_platform_t plat;
    memset(&plat, 0, sizeof(plat));

    char buf[256] = {0};
    eai_status_t st = CORE_OPS.get_device_info(&plat, buf, sizeof(buf));
    if (st != EAI_OK) { FAIL("get_device_info failed"); return; }
    if (strlen(buf) == 0) { FAIL("empty string"); return; }
    printf("(%s) ", buf);
    PASS();
}

static void test_hal_core_cpu_count(void)
{
    TEST(hal_core_cpu_count);
    eai_platform_t plat;
    memset(&plat, 0, sizeof(plat));

    int n = CORE_OPS.get_cpu_count(&plat);
    if (n <= 0) { FAIL("cpu count <= 0"); return; }
    printf("(%d cores) ", n);
    PASS();
}

static void test_hal_memory_info(void)
{
    TEST(hal_memory_info);
    eai_platform_t plat;
    memset(&plat, 0, sizeof(plat));

    uint64_t total = 0, avail = 0;
    eai_status_t st = MEM_OPS.get_memory_info(&plat, &total, &avail);
    if (st != EAI_OK) { FAIL("get_memory_info failed"); return; }
    if (total == 0) { FAIL("total is 0"); return; }
    printf("(%lu MB) ", (unsigned long)(total / (1024*1024)));
    PASS();
}

static void test_hal_memory_aligned_alloc(void)
{
    TEST(hal_memory_aligned_alloc);
    void *ptr = MEM_OPS.alloc_aligned(1024, 64);
    if (!ptr) { FAIL("alloc returned NULL"); return; }
    if (((uintptr_t)ptr & 63) != 0) { FAIL("not aligned to 64"); MEM_OPS.free_aligned(ptr); return; }
    MEM_OPS.free_aligned(ptr);
    PASS();
}

static void test_hal_timer_monotonic(void)
{
    TEST(hal_timer_monotonic);
    uint64_t t1 = TIMER_OPS.get_time_us();
    /* Small busy loop */
    volatile int x = 0;
    for (int i = 0; i < 100000; i++) x++;
    uint64_t t2 = TIMER_OPS.get_time_us();
    if (t2 <= t1) { FAIL("time did not advance"); return; }
    printf("(%lu us) ", (unsigned long)(t2 - t1));
    PASS();
}

static void test_hal_platform_init_hal(void)
{
    TEST(hal_platform_init_hal);

    static const eai_platform_hal_t test_hal = {
        .name   = "test-hal",
        .core   = &CORE_OPS,
        .memory = &MEM_OPS,
        .timer  = &TIMER_OPS,
    };

    eai_platform_t plat;
    eai_status_t st = eai_platform_init_hal(&plat, &test_hal);
    if (st != EAI_OK) { FAIL("init_hal failed"); return; }
    if (!plat.initialized) { FAIL("not initialized"); return; }
    if (plat.hal != &test_hal) { FAIL("hal pointer mismatch"); return; }

    eai_platform_shutdown(&plat);
    if (plat.initialized) { FAIL("still initialized after shutdown"); return; }
    PASS();
}

static void test_legacy_backward_compat(void)
{
    TEST(legacy_backward_compat);
    eai_platform_t plat;
    eai_status_t st = eai_platform_detect(&plat);
    if (st != EAI_OK) { FAIL("detect failed"); return; }
    /* Legacy API should still work */
    if (!plat.ops) { FAIL("ops is NULL"); eai_platform_shutdown(&plat); return; }
    if (!plat.ops->name) { FAIL("name is NULL"); eai_platform_shutdown(&plat); return; }
    printf("(%s) ", plat.ops->name);
    eai_platform_shutdown(&plat);
    PASS();
}

int main(void)
{
    printf("=== EAI HAL Tests ===\n\n");

    test_hal_core_init();
    test_hal_core_device_info();
    test_hal_core_cpu_count();
    test_hal_memory_info();
    test_hal_memory_aligned_alloc();
    test_hal_timer_monotonic();
    test_hal_platform_init_hal();
    test_legacy_backward_compat();

    printf("\n=== Results: %d/%d passed, %d failed ===\n",
           tests_passed, tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
