// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// Tests for the stable C API surface (eai_api.h)

#include <stdio.h>
#include <string.h>
#include "eai/common.h"
#include "eai/platform.h"

static int tests_run = 0, tests_passed = 0, tests_failed = 0;
#define TEST(name) do { tests_run++; printf("  TEST %-44s ", #name); } while(0)
#define PASS() do { tests_passed++; printf("[PASS]\n"); } while(0)
#define FAIL(msg) do { tests_failed++; printf("[FAIL] %s\n", msg); } while(0)

static void test_version_string(void)
{
    TEST(version_string);
    const char *v = eai_version();
    if (!v) { FAIL("eai_version returned NULL"); return; }
    if (strlen(v) == 0) { FAIL("empty version string"); return; }
    if (strcmp(v, "0.2.0") != 0) { FAIL("version not 0.2.0"); return; }
    printf("(%s) ", v);
    PASS();
}

static void test_version_components(void)
{
    TEST(version_components);
    if (eai_version_major() != 0) { FAIL("major != 0"); return; }
    if (eai_version_minor() != 2) { FAIL("minor != 2"); return; }
    if (eai_version_patch() != 0) { FAIL("patch != 0"); return; }
    PASS();
}

static void test_api_status_str(void)
{
    TEST(api_status_str);
    const char *s = eai_api_status_str(EAI_OK);
    if (!s) { FAIL("NULL for EAI_OK"); return; }
    if (strcmp(s, "OK") != 0) { FAIL("expected 'OK'"); return; }

    s = eai_api_status_str(EAI_ERR_PLATFORM);
    if (!s || strcmp(s, "ERR_PLATFORM") != 0) { FAIL("ERR_PLATFORM mismatch"); return; }
    PASS();
}

static void test_api_platform_detect(void)
{
    TEST(api_platform_detect);
    eai_platform_t plat;
    eai_status_t st = eai_api_platform_detect(&plat);
    if (st != EAI_OK) { FAIL("detect failed"); return; }
    if (!plat.initialized) { FAIL("not initialized"); return; }
    eai_api_platform_shutdown(&plat);
    PASS();
}

static void test_api_platform_get_info(void)
{
    TEST(api_platform_get_info);
    eai_platform_t plat;
    eai_api_platform_detect(&plat);

    char buf[256] = {0};
    eai_status_t st = eai_api_platform_get_info(&plat, buf, sizeof(buf));
    if (st != EAI_OK) { FAIL("get_info failed"); eai_api_platform_shutdown(&plat); return; }
    if (strlen(buf) == 0) { FAIL("empty info"); eai_api_platform_shutdown(&plat); return; }
    printf("(%s) ", buf);
    eai_api_platform_shutdown(&plat);
    PASS();
}

static void test_api_platform_get_info_null(void)
{
    TEST(api_platform_get_info_null);
    eai_status_t st = eai_api_platform_get_info(NULL, NULL, 0);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID"); return; }
    PASS();
}

static void test_api_platform_get_info_not_initialized(void)
{
    TEST(api_platform_get_info_not_init);
    eai_platform_t plat;
    memset(&plat, 0, sizeof(plat));
    char buf[64];
    eai_status_t st = eai_api_platform_get_info(&plat, buf, sizeof(buf));
    if (st != EAI_ERR_RUNTIME) { FAIL("expected RUNTIME for uninitialized"); return; }
    PASS();
}

static void test_api_platform_get_memory(void)
{
    TEST(api_platform_get_memory);
    eai_platform_t plat;
    eai_api_platform_detect(&plat);

    uint64_t total = 0, avail = 0;
    eai_status_t st = eai_api_platform_get_memory(&plat, &total, &avail);
    if (st != EAI_OK) { FAIL("get_memory failed"); eai_api_platform_shutdown(&plat); return; }
    if (total == 0) { FAIL("total is 0"); eai_api_platform_shutdown(&plat); return; }
    if (avail > total) { FAIL("avail > total"); eai_api_platform_shutdown(&plat); return; }
    printf("(%lu MB) ", (unsigned long)(total / (1024*1024)));
    eai_api_platform_shutdown(&plat);
    PASS();
}

static void test_api_platform_get_memory_null(void)
{
    TEST(api_platform_get_memory_null);
    eai_status_t st = eai_api_platform_get_memory(NULL, NULL, NULL);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID"); return; }
    PASS();
}

static void test_api_platform_shutdown_safe(void)
{
    TEST(api_platform_shutdown_safe);
    eai_api_platform_shutdown(NULL); /* should not crash */
    eai_platform_t plat;
    memset(&plat, 0, sizeof(plat));
    eai_api_platform_shutdown(&plat); /* uninitialized — should be safe */
    PASS();
}

int main(void)
{
    printf("=== EAI Stable C API Tests ===\n\n");

    test_version_string();
    test_version_components();
    test_api_status_str();
    test_api_platform_detect();
    test_api_platform_get_info();
    test_api_platform_get_info_null();
    test_api_platform_get_info_not_initialized();
    test_api_platform_get_memory();
    test_api_platform_get_memory_null();
    test_api_platform_shutdown_safe();

    printf("\n=== Results: %d/%d passed, %d failed ===\n",
           tests_passed, tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
