// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// ISO/IEC 25000 | ISO/IEC/IEEE 15288:2023

#include <stdio.h>
#include <string.h>
#include "eai/platform.h"

static int tests_run = 0, tests_passed = 0, tests_failed = 0;
#define TEST(name) do { tests_run++; printf("  TEST %-40s ", #name); } while(0)
#define PASS() do { tests_passed++; printf("[PASS]\n"); } while(0)
#define FAIL(msg) do { tests_failed++; printf("[FAIL] %s\n", msg); } while(0)

static void test_platform_detect(void)
{
    TEST(platform_detect);
    eai_platform_t plat;
    eai_status_t st = eai_platform_detect(&plat);
    if (st != EAI_OK) { FAIL("detect failed"); return; }
    if (!plat.initialized) { FAIL("not initialized"); return; }
    if (!plat.ops) { FAIL("ops is NULL"); return; }
    if (!plat.ops->name) { FAIL("ops->name is NULL"); return; }
    printf("(%s) ", plat.ops->name);
    eai_platform_shutdown(&plat);
    PASS();
}

static void test_platform_detect_null(void)
{
    TEST(platform_detect_null);
    eai_status_t st = eai_platform_detect(NULL);
    if (st != EAI_ERR_INVALID) { FAIL("expected EAI_ERR_INVALID"); return; }
    PASS();
}

static void test_platform_init_null(void)
{
    TEST(platform_init_null);
    eai_platform_t plat;
    eai_status_t st = eai_platform_init(NULL, NULL);
    if (st != EAI_ERR_INVALID) { FAIL("expected EAI_ERR_INVALID for NULL,NULL"); return; }
    st = eai_platform_init(&plat, NULL);
    if (st != EAI_ERR_INVALID) { FAIL("expected EAI_ERR_INVALID for plat,NULL"); return; }
    PASS();
}

static void test_platform_device_info(void)
{
    TEST(platform_device_info);
    eai_platform_t plat;
    eai_status_t st = eai_platform_detect(&plat);
    if (st != EAI_OK) { FAIL("detect failed"); return; }

    char buf[256] = {0};
    if (!plat.ops->get_device_info) { FAIL("get_device_info is NULL"); eai_platform_shutdown(&plat); return; }
    st = plat.ops->get_device_info(&plat, buf, sizeof(buf));
    if (st != EAI_OK) { FAIL("get_device_info failed"); eai_platform_shutdown(&plat); return; }
    if (strlen(buf) == 0) { FAIL("empty device info"); eai_platform_shutdown(&plat); return; }

    /* On Windows, should NOT show "0.0 (Build 0)" */
#ifdef _WIN32
    if (strstr(buf, "0.0 (Build 0)")) { FAIL("Windows version not detected (got 0.0 Build 0)"); eai_platform_shutdown(&plat); return; }
#endif

    printf("(%s) ", buf);
    eai_platform_shutdown(&plat);
    PASS();
}

static void test_platform_memory_info(void)
{
    TEST(platform_memory_info);
    eai_platform_t plat;
    eai_status_t st = eai_platform_detect(&plat);
    if (st != EAI_OK) { FAIL("detect failed"); return; }

    uint64_t total = 0, avail = 0;
    if (!plat.ops->get_memory_info) { FAIL("get_memory_info is NULL"); eai_platform_shutdown(&plat); return; }
    st = plat.ops->get_memory_info(&plat, &total, &avail);
    if (st != EAI_OK) { FAIL("get_memory_info failed"); eai_platform_shutdown(&plat); return; }
    if (total == 0) { FAIL("total memory is 0"); eai_platform_shutdown(&plat); return; }

    printf("(%lu MB) ", (unsigned long)(total / (1024*1024)));
    eai_platform_shutdown(&plat);
    PASS();
}

static void test_platform_shutdown_idempotent(void)
{
    TEST(platform_shutdown_idempotent);
    eai_platform_t plat;
    eai_platform_detect(&plat);
    eai_platform_shutdown(&plat);
    /* Second shutdown should be safe */
    eai_platform_shutdown(&plat);
    /* Shutdown on NULL should be safe */
    eai_platform_shutdown(NULL);
    PASS();
}

int main(void)
{
    printf("=== EAI Platform Tests ===\n\n");

    test_platform_detect();
    test_platform_detect_null();
    test_platform_init_null();
    test_platform_device_info();
    test_platform_memory_info();
    test_platform_shutdown_idempotent();

    printf("\n=== Results: %d/%d passed, %d failed ===\n",
           tests_passed, tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
