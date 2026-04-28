// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// Tests for eai_api_accel_*, eai_api_platform HAL-path routing,
// and eai_api_platform edge cases not covered by test_eai_api.c

#include <stdio.h>
#include <string.h>
#include "eai/common.h"
#include "eai/platform.h"

static int tests_run = 0, tests_passed = 0, tests_failed = 0;
#define TEST(name) do { tests_run++; printf("  TEST %-44s ", #name); } while(0)
#define PASS() do { tests_passed++; printf("[PASS]\n"); } while(0)
#define FAIL(msg) do { tests_failed++; printf("[FAIL] %s\n", msg); } while(0)

/* ========== eai_api_accel_count / get_name ========== */

static void test_api_accel_count(void)
{
    TEST(api_accel_count);
    int count = eai_api_accel_count();
    /* Currently returns 0 since accel module is separate */
    if (count < 0) { FAIL("count should be >= 0"); return; }
    printf("(count=%d) ", count);
    PASS();
}

static void test_api_accel_get_name_null_buf(void)
{
    TEST(api_accel_get_name_null_buf);
    eai_status_t st = eai_api_accel_get_name(0, NULL, 64);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for NULL buf"); return; }
    PASS();
}

static void test_api_accel_get_name_zero_size(void)
{
    TEST(api_accel_get_name_zero_size);
    char buf[16];
    eai_status_t st = eai_api_accel_get_name(0, buf, 0);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for size=0"); return; }
    PASS();
}

static void test_api_accel_get_name_valid(void)
{
    TEST(api_accel_get_name_valid);
    char buf[64] = "initial";
    eai_status_t st = eai_api_accel_get_name(0, buf, sizeof(buf));
    /* Currently returns NOT_IMPLEMENTED and clears buf */
    if (st != EAI_ERR_NOT_IMPLEMENTED) { FAIL("expected NOT_IMPLEMENTED"); return; }
    if (buf[0] != '\0') { FAIL("buf should be cleared"); return; }
    PASS();
}

/* ========== eai_api_platform via HAL path ========== */

#ifdef _WIN32
extern const eai_hal_core_ops_t eai_hal_windows_core_ops;
extern const eai_hal_memory_ops_t eai_hal_windows_memory_ops;
#define HAL_CORE eai_hal_windows_core_ops
#define HAL_MEM  eai_hal_windows_memory_ops
#elif defined(__linux__)
extern const eai_hal_core_ops_t eai_hal_linux_core_ops;
extern const eai_hal_memory_ops_t eai_hal_linux_memory_ops;
#define HAL_CORE eai_hal_linux_core_ops
#define HAL_MEM  eai_hal_linux_memory_ops
#endif

static void test_api_get_info_via_hal(void)
{
    TEST(api_get_info_via_hal);

    /* Create a HAL-initialized platform and route through eai_api_platform_get_info */
    static const eai_platform_hal_t test_hal = {
        .name = "api-hal-test",
        .core = &HAL_CORE,
        .memory = &HAL_MEM,
    };

    eai_platform_t plat;
    eai_status_t st = eai_platform_init_hal(&plat, &test_hal);
    if (st != EAI_OK) { FAIL("init_hal failed"); return; }

    char buf[256] = {0};
    st = eai_api_platform_get_info(&plat, buf, sizeof(buf));
    if (st != EAI_OK) { FAIL("get_info via HAL failed"); eai_platform_shutdown(&plat); return; }
    if (strlen(buf) == 0) { FAIL("HAL info is empty"); eai_platform_shutdown(&plat); return; }
    printf("(%s) ", buf);

    eai_platform_shutdown(&plat);
    PASS();
}

static void test_api_get_memory_via_hal(void)
{
    TEST(api_get_memory_via_hal);

    static const eai_platform_hal_t test_hal = {
        .name = "api-hal-mem-test",
        .core = &HAL_CORE,
        .memory = &HAL_MEM,
    };

    eai_platform_t plat;
    eai_platform_init_hal(&plat, &test_hal);

    uint64_t total = 0, avail = 0;
    eai_status_t st = eai_api_platform_get_memory(&plat, &total, &avail);
    if (st != EAI_OK) { FAIL("get_memory via HAL failed"); eai_platform_shutdown(&plat); return; }
    if (total == 0) { FAIL("total is 0"); eai_platform_shutdown(&plat); return; }
    printf("(%lu MB) ", (unsigned long)(total / (1024*1024)));

    eai_platform_shutdown(&plat);
    PASS();
}

static void test_api_get_info_no_ops_no_hal(void)
{
    TEST(api_get_info_no_ops_no_hal);
    /* Manually create a platform with initialized=true but no ops and no hal */
    eai_platform_t plat;
    memset(&plat, 0, sizeof(plat));
    plat.initialized = true;
    plat.ops = NULL;
    plat.hal = NULL;

    char buf[64];
    eai_status_t st = eai_api_platform_get_info(&plat, buf, sizeof(buf));
    if (st != EAI_ERR_UNSUPPORTED) { FAIL("expected UNSUPPORTED for empty platform"); return; }
    PASS();
}

static void test_api_get_memory_no_ops_no_hal(void)
{
    TEST(api_get_memory_no_ops_no_hal);
    eai_platform_t plat;
    memset(&plat, 0, sizeof(plat));
    plat.initialized = true;
    plat.ops = NULL;
    plat.hal = NULL;

    uint64_t total = 0, avail = 0;
    eai_status_t st = eai_api_platform_get_memory(&plat, &total, &avail);
    if (st != EAI_ERR_UNSUPPORTED) { FAIL("expected UNSUPPORTED"); return; }
    PASS();
}

static void test_api_get_info_zero_buf_size(void)
{
    TEST(api_get_info_zero_buf_size);
    eai_platform_t plat;
    eai_api_platform_detect(&plat);
    char buf[64];
    eai_status_t st = eai_api_platform_get_info(&plat, buf, 0);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for buf_size=0"); eai_api_platform_shutdown(&plat); return; }
    eai_api_platform_shutdown(&plat);
    PASS();
}

static void test_api_get_info_null_buf(void)
{
    TEST(api_get_info_null_buf);
    eai_platform_t plat;
    eai_api_platform_detect(&plat);
    eai_status_t st = eai_api_platform_get_info(&plat, NULL, 64);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for NULL buf"); eai_api_platform_shutdown(&plat); return; }
    eai_api_platform_shutdown(&plat);
    PASS();
}

static void test_api_get_memory_not_initialized(void)
{
    TEST(api_get_memory_not_initialized);
    eai_platform_t plat;
    memset(&plat, 0, sizeof(plat));
    /* initialized = false */
    uint64_t total = 0, avail = 0;
    eai_status_t st = eai_api_platform_get_memory(&plat, &total, &avail);
    if (st != EAI_ERR_RUNTIME) { FAIL("expected RUNTIME for uninitialized"); return; }
    PASS();
}

int main(void)
{
    printf("=== EAI API Extended Tests ===\n\n");

    printf("--- Accelerator API ---\n");
    test_api_accel_count();
    test_api_accel_get_name_null_buf();
    test_api_accel_get_name_zero_size();
    test_api_accel_get_name_valid();

    printf("\n--- Platform API via HAL ---\n");
    test_api_get_info_via_hal();
    test_api_get_memory_via_hal();
    test_api_get_info_no_ops_no_hal();
    test_api_get_memory_no_ops_no_hal();
    test_api_get_info_zero_buf_size();
    test_api_get_info_null_buf();
    test_api_get_memory_not_initialized();

    printf("\n=== Results: %d/%d passed, %d failed ===\n",
           tests_passed, tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
