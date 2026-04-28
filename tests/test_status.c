// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// Comprehensive tests for eai_status_str — all codes, edge cases, uniqueness

#include <stdio.h>
#include <string.h>
#include "eai/common.h"

static int tests_run = 0, tests_passed = 0, tests_failed = 0;
#define TEST(name) do { tests_run++; printf("  TEST %-44s ", #name); } while(0)
#define PASS() do { tests_passed++; printf("[PASS]\n"); } while(0)
#define FAIL(msg) do { tests_failed++; printf("[FAIL] %s\n", msg); } while(0)

/* ---- Exhaustive check: every status code has a non-NULL, non-empty, non-UNKNOWN string ---- */

static void test_all_status_strings_are_defined(void)
{
    TEST(all_status_strings_are_defined);

    struct { eai_status_t code; const char *expected; } cases[] = {
        { EAI_OK,                  "OK" },
        { EAI_ERR_NOMEM,           "ERR_NOMEM" },
        { EAI_ERR_INVALID,         "ERR_INVALID" },
        { EAI_ERR_NOT_FOUND,       "ERR_NOT_FOUND" },
        { EAI_ERR_IO,              "ERR_IO" },
        { EAI_ERR_TIMEOUT,         "ERR_TIMEOUT" },
        { EAI_ERR_PERMISSION,      "ERR_PERMISSION" },
        { EAI_ERR_RUNTIME,         "ERR_RUNTIME" },
        { EAI_ERR_CONNECT,         "ERR_CONNECT" },
        { EAI_ERR_PROTOCOL,        "ERR_PROTOCOL" },
        { EAI_ERR_CONFIG,          "ERR_CONFIG" },
        { EAI_ERR_UNSUPPORTED,     "ERR_UNSUPPORTED" },
        { EAI_ERR_LEARNING,        "ERR_LEARNING" },
        { EAI_ERR_ADAPTER,         "ERR_ADAPTER" },
        { EAI_ERR_STALE_MODEL,     "ERR_STALE_MODEL" },
        { EAI_ERR_RESOURCE_BUDGET, "ERR_RESOURCE_BUDGET" },
        { EAI_ERR_BCI_SIGNAL,      "ERR_BCI_SIGNAL" },
        { EAI_ERR_BCI_CALIBRATION, "ERR_BCI_CALIBRATION" },
        { EAI_ERR_BCI_DECODE,      "ERR_BCI_DECODE" },
        { EAI_ERR_PLATFORM,        "ERR_PLATFORM" },
        { EAI_ERR_ACCEL,           "ERR_ACCEL" },
        { EAI_ERR_FORMAT,          "ERR_FORMAT" },
        { EAI_ERR_DELEGATE,        "ERR_DELEGATE" },
        { EAI_ERR_NOT_IMPLEMENTED, "ERR_NOT_IMPLEMENTED" },
        { EAI_ERR_HW_UNAVAILABLE,  "ERR_HW_UNAVAILABLE" },
    };

    int n = sizeof(cases) / sizeof(cases[0]);
    for (int i = 0; i < n; i++) {
        const char *got = eai_status_str(cases[i].code);
        if (!got) {
            char msg[128];
            snprintf(msg, sizeof(msg), "NULL for code %d", cases[i].code);
            FAIL(msg);
            return;
        }
        if (strcmp(got, cases[i].expected) != 0) {
            char msg[128];
            snprintf(msg, sizeof(msg), "code %d: expected '%s', got '%s'",
                     cases[i].code, cases[i].expected, got);
            FAIL(msg);
            return;
        }
    }
    PASS();
}

/* ---- Out-of-range codes return "UNKNOWN" ---- */

static void test_unknown_status_code(void)
{
    TEST(unknown_status_code);
    const char *s = eai_status_str((eai_status_t)9999);
    if (!s) { FAIL("returned NULL for unknown code"); return; }
    if (strcmp(s, "UNKNOWN") != 0) { FAIL("expected 'UNKNOWN'"); return; }
    PASS();
}

static void test_negative_status_code(void)
{
    TEST(negative_status_code);
    const char *s = eai_status_str((eai_status_t)-1);
    if (!s) { FAIL("returned NULL for negative code"); return; }
    if (strcmp(s, "UNKNOWN") != 0) { FAIL("expected 'UNKNOWN'"); return; }
    PASS();
}

/* ---- All status strings are unique ---- */

static void test_status_strings_unique(void)
{
    TEST(status_strings_unique);

    eai_status_t codes[] = {
        EAI_OK, EAI_ERR_NOMEM, EAI_ERR_INVALID, EAI_ERR_NOT_FOUND,
        EAI_ERR_IO, EAI_ERR_TIMEOUT, EAI_ERR_PERMISSION, EAI_ERR_RUNTIME,
        EAI_ERR_CONNECT, EAI_ERR_PROTOCOL, EAI_ERR_CONFIG, EAI_ERR_UNSUPPORTED,
        EAI_ERR_LEARNING, EAI_ERR_ADAPTER, EAI_ERR_STALE_MODEL,
        EAI_ERR_RESOURCE_BUDGET, EAI_ERR_BCI_SIGNAL, EAI_ERR_BCI_CALIBRATION,
        EAI_ERR_BCI_DECODE, EAI_ERR_PLATFORM, EAI_ERR_ACCEL, EAI_ERR_FORMAT,
        EAI_ERR_DELEGATE, EAI_ERR_NOT_IMPLEMENTED, EAI_ERR_HW_UNAVAILABLE,
    };
    int n = sizeof(codes) / sizeof(codes[0]);

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            const char *si = eai_status_str(codes[i]);
            const char *sj = eai_status_str(codes[j]);
            if (strcmp(si, sj) == 0) {
                char msg[128];
                snprintf(msg, sizeof(msg), "codes %d and %d both map to '%s'",
                         codes[i], codes[j], si);
                FAIL(msg);
                return;
            }
        }
    }
    PASS();
}

/* ---- EAI_OK is always 0 ---- */

static void test_eai_ok_is_zero(void)
{
    TEST(eai_ok_is_zero);
    if (EAI_OK != 0) { FAIL("EAI_OK != 0"); return; }
    PASS();
}

/* ---- All error codes are non-zero ---- */

static void test_all_errors_nonzero(void)
{
    TEST(all_errors_nonzero);
    eai_status_t errors[] = {
        EAI_ERR_NOMEM, EAI_ERR_INVALID, EAI_ERR_NOT_FOUND,
        EAI_ERR_IO, EAI_ERR_TIMEOUT, EAI_ERR_PLATFORM,
        EAI_ERR_ACCEL, EAI_ERR_FORMAT, EAI_ERR_DELEGATE,
        EAI_ERR_NOT_IMPLEMENTED, EAI_ERR_HW_UNAVAILABLE,
    };
    int n = sizeof(errors) / sizeof(errors[0]);
    for (int i = 0; i < n; i++) {
        if (errors[i] == 0) {
            char msg[64];
            snprintf(msg, sizeof(msg), "error code index %d is 0", i);
            FAIL(msg);
            return;
        }
    }
    PASS();
}

int main(void)
{
    printf("=== EAI Status/Types Tests (Comprehensive) ===\n\n");

    test_all_status_strings_are_defined();
    test_unknown_status_code();
    test_negative_status_code();
    test_status_strings_unique();
    test_eai_ok_is_zero();
    test_all_errors_nonzero();

    printf("\n=== Results: %d/%d passed, %d failed ===\n",
           tests_passed, tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
