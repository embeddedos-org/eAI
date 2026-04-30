// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// Comprehensive tests for platform HAL init, shutdown, and edge cases

#include <stdio.h>
#include <string.h>
#include "eai/platform.h"

static int tests_run = 0, tests_passed = 0, tests_failed = 0;
#define TEST(name) do { tests_run++; printf("  TEST %-44s ", #name); } while(0)
#define PASS() do { tests_passed++; printf("[PASS]\n"); } while(0)
#define FAIL(msg) do { tests_failed++; printf("[FAIL] %s\n", msg); } while(0)

/* ---- eai_platform_init_hal edge cases ---- */

static void test_init_hal_null_plat(void)
{
    TEST(init_hal_null_plat);
    eai_status_t st = eai_platform_init_hal(NULL, NULL);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for NULL plat"); return; }
    PASS();
}

static void test_init_hal_null_hal(void)
{
    TEST(init_hal_null_hal);
    eai_platform_t plat;
    eai_status_t st = eai_platform_init_hal(&plat, NULL);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for NULL hal"); return; }
    PASS();
}

static void test_init_hal_missing_core(void)
{
    TEST(init_hal_missing_core);
    /* HAL with NULL core should fail — core is required */
    static const eai_hal_memory_ops_t dummy_mem = { .get_memory_info = NULL };
    static const eai_platform_hal_t bad_hal = {
        .name = "bad",
        .core = NULL,
        .memory = &dummy_mem,
    };
    eai_platform_t plat;
    eai_status_t st = eai_platform_init_hal(&plat, &bad_hal);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for missing core"); return; }
    PASS();
}

static void test_init_hal_missing_memory(void)
{
    TEST(init_hal_missing_memory);
    /* HAL with NULL memory should fail — memory is required */
    /* We need a core ops with a valid init function, but we can just use the
       same pattern as missing_core test — the check is done before init is called */
    static const eai_hal_core_ops_t dummy_core = { .init = NULL };
    static const eai_platform_hal_t bad_hal = {
        .name = "bad",
        .core = &dummy_core,
        .memory = NULL,
    };
    eai_platform_t plat;
    eai_status_t st = eai_platform_init_hal(&plat, &bad_hal);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for missing memory"); return; }
    PASS();
}

/* Helper functions for tests */
static eai_status_t helper_ok_init(eai_platform_t *p) { (void)p; return EAI_OK; }
static void helper_ok_shutdown(eai_platform_t *p) { (void)p; }
static eai_status_t helper_ok_meminfo(eai_platform_t *p, uint64_t *t, uint64_t *a) {
    (void)p; if (t) *t = 1024; if (a) *a = 512; return EAI_OK;
}

/* ---- Legacy vs HAL state ---- */

static void test_legacy_init_sets_ops_not_hal(void)
{
    TEST(legacy_init_sets_ops_not_hal);
    eai_platform_t plat;
    eai_status_t st = eai_platform_detect(&plat);
    if (st != EAI_OK) { FAIL("detect failed"); return; }
    if (!plat.ops) { FAIL("ops should not be NULL"); eai_platform_shutdown(&plat); return; }
    if (plat.hal != NULL) { FAIL("hal should be NULL for legacy init"); eai_platform_shutdown(&plat); return; }
    eai_platform_shutdown(&plat);
    PASS();
}

static void test_hal_init_sets_hal_not_ops(void)
{
    TEST(hal_init_sets_hal_not_ops);

    static const eai_hal_core_ops_t test_core = { .init = helper_ok_init, .shutdown = helper_ok_shutdown };
    static const eai_hal_memory_ops_t test_mem = { .get_memory_info = helper_ok_meminfo };
    static const eai_platform_hal_t test_hal = {
        .name = "unit-test-hal",
        .core = &test_core,
        .memory = &test_mem,
    };

    eai_platform_t plat;
    eai_status_t st = eai_platform_init_hal(&plat, &test_hal);
    if (st != EAI_OK) { FAIL("init_hal failed"); return; }
    if (plat.ops != NULL) { FAIL("ops should be NULL for HAL init"); eai_platform_shutdown(&plat); return; }
    if (plat.hal != &test_hal) { FAIL("hal pointer mismatch"); eai_platform_shutdown(&plat); return; }
    if (!plat.initialized) { FAIL("should be initialized"); eai_platform_shutdown(&plat); return; }
    eai_platform_shutdown(&plat);
    PASS();
}

/* ---- HAL init failure propagation ---- */

static eai_status_t fail_init(eai_platform_t *p) { return EAI_ERR_PLATFORM; }

static void test_hal_init_failure_propagated(void)
{
    TEST(hal_init_failure_propagated);

    static const eai_hal_core_ops_t fail_core = { .init = fail_init };
    static const eai_hal_memory_ops_t dummy_mem = { .get_memory_info = helper_ok_meminfo };
    static const eai_platform_hal_t bad_hal = {
        .name = "fail-test",
        .core = &fail_core,
        .memory = &dummy_mem,
    };

    eai_platform_t plat;
    eai_status_t st = eai_platform_init_hal(&plat, &bad_hal);
    if (st != EAI_ERR_PLATFORM) { FAIL("expected EAI_ERR_PLATFORM"); return; }
    if (plat.initialized) { FAIL("should NOT be initialized after init failure"); return; }
    PASS();
}

/* ---- HAL shutdown dispatch ---- */

static int shutdown_called = 0;
static void tracking_shutdown(eai_platform_t *p) { shutdown_called++; }

static void test_hal_shutdown_calls_core_shutdown(void)
{
    TEST(hal_shutdown_calls_core_shutdown);
    shutdown_called = 0;

    static const eai_hal_core_ops_t core = { .init = helper_ok_init, .shutdown = tracking_shutdown };
    static const eai_hal_memory_ops_t mem = { .get_memory_info = helper_ok_meminfo };
    static const eai_platform_hal_t hal = { .name = "track", .core = &core, .memory = &mem };

    eai_platform_t plat;
    eai_platform_init_hal(&plat, &hal);
    eai_platform_shutdown(&plat);

    if (shutdown_called != 1) { FAIL("shutdown not called exactly once"); return; }
    if (plat.initialized) { FAIL("still initialized after shutdown"); return; }
    PASS();
}

/* ---- eai_hal_has_* macros ---- */

static void test_hal_has_macros(void)
{
    TEST(hal_has_macros);

    static const eai_hal_core_ops_t core = { .init = helper_ok_init };
    static const eai_hal_memory_ops_t mem = { .get_memory_info = helper_ok_meminfo };
    static const eai_hal_gpio_ops_t gpio = { .read = NULL };
    static const eai_platform_hal_t hal = {
        .name = "macro-test",
        .core = &core,
        .memory = &mem,
        .gpio = &gpio,
        .thread = NULL,
        .fs = NULL,
        .net = NULL,
        .timer = NULL,
        .accel = NULL,
    };

    eai_platform_t plat;
    eai_platform_init_hal(&plat, &hal);

    if (!eai_hal_has_gpio(&plat)) { FAIL("should have GPIO"); eai_platform_shutdown(&plat); return; }
    if (eai_hal_has_thread(&plat)) { FAIL("should NOT have thread"); eai_platform_shutdown(&plat); return; }
    if (eai_hal_has_fs(&plat)) { FAIL("should NOT have fs"); eai_platform_shutdown(&plat); return; }
    if (eai_hal_has_net(&plat)) { FAIL("should NOT have net"); eai_platform_shutdown(&plat); return; }
    if (eai_hal_has_timer(&plat)) { FAIL("should NOT have timer"); eai_platform_shutdown(&plat); return; }
    if (eai_hal_has_accel(&plat)) { FAIL("should NOT have accel"); eai_platform_shutdown(&plat); return; }

    eai_platform_shutdown(&plat);
    PASS();
}

/* ---- Platform detect produces valid platform name ---- */

static void test_detect_platform_name_not_empty(void)
{
    TEST(detect_platform_name_not_empty);
    eai_platform_t plat;
    eai_status_t st = eai_platform_detect(&plat);
    if (st != EAI_OK) { FAIL("detect failed"); return; }
    if (!plat.ops || !plat.ops->name) { FAIL("name is NULL"); eai_platform_shutdown(&plat); return; }
    if (strlen(plat.ops->name) == 0) { FAIL("name is empty string"); eai_platform_shutdown(&plat); return; }

    /* Name should be a known platform */
    const char *name = plat.ops->name;
    int known = (strcmp(name, "linux") == 0 || strcmp(name, "windows") == 0 ||
                 strcmp(name, "container") == 0 || strcmp(name, "eos") == 0 ||
                 strcmp(name, "linux-unavailable") == 0 || strcmp(name, "windows-unavailable") == 0);
    if (!known) { printf("(unknown name: %s) ", name); }
    printf("(%s) ", name);
    eai_platform_shutdown(&plat);
    PASS();
}

/* ---- Detect + all ops non-NULL ---- */

static void test_detect_all_required_ops(void)
{
    TEST(detect_all_required_ops);
    eai_platform_t plat;
    eai_status_t st = eai_platform_detect(&plat);
    if (st != EAI_OK) { FAIL("detect failed"); return; }

    if (!plat.ops->init) { FAIL("init is NULL"); eai_platform_shutdown(&plat); return; }
    if (!plat.ops->get_device_info) { FAIL("get_device_info is NULL"); eai_platform_shutdown(&plat); return; }
    if (!plat.ops->get_memory_info) { FAIL("get_memory_info is NULL"); eai_platform_shutdown(&plat); return; }
    if (!plat.ops->shutdown) { FAIL("shutdown is NULL"); eai_platform_shutdown(&plat); return; }

    eai_platform_shutdown(&plat);
    PASS();
}

/* ---- Available memory <= total memory ---- */

static void test_memory_available_lte_total(void)
{
    TEST(memory_available_lte_total);
    eai_platform_t plat;
    eai_platform_detect(&plat);

    uint64_t total = 0, avail = 0;
    eai_status_t st = plat.ops->get_memory_info(&plat, &total, &avail);
    if (st != EAI_OK) { FAIL("get_memory_info failed"); eai_platform_shutdown(&plat); return; }
    if (avail > total) { FAIL("available > total"); eai_platform_shutdown(&plat); return; }

    eai_platform_shutdown(&plat);
    PASS();
}

/* ---- GPIO on desktop should return unsupported ---- */

static void test_gpio_unsupported_on_desktop(void)
{
    TEST(gpio_unsupported_on_desktop);
    eai_platform_t plat;
    eai_platform_detect(&plat);

#if defined(_WIN32) || defined(__APPLE__)
    /* GPIO on Windows/macOS should be unsupported */
    if (plat.ops->read_gpio) {
        int val = 0;
        eai_status_t st = plat.ops->read_gpio(&plat, 0, &val);
        if (st != EAI_ERR_UNSUPPORTED) { FAIL("expected UNSUPPORTED for GPIO on desktop"); eai_platform_shutdown(&plat); return; }
    }
#endif

    eai_platform_shutdown(&plat);
    PASS();
}

int main(void)
{
    printf("=== EAI Platform HAL Tests (Comprehensive) ===\n\n");

    test_init_hal_null_plat();
    test_init_hal_null_hal();
    test_init_hal_missing_core();
    test_init_hal_missing_memory();
    test_legacy_init_sets_ops_not_hal();
    test_hal_init_sets_hal_not_ops();
    test_hal_init_failure_propagated();
    test_hal_shutdown_calls_core_shutdown();
    test_hal_has_macros();
    test_detect_platform_name_not_empty();
    test_detect_all_required_ops();
    test_memory_available_lte_total();
    test_gpio_unsupported_on_desktop();

    printf("\n=== Results: %d/%d passed, %d failed ===\n",
           tests_passed, tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
