// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// ISO/IEC 25000 | ISO/IEC/IEEE 15288:2023
//
// eai_bci_create() backend selection. The handle is opaque, so these tests
// check the observable contract: a name the build can satisfy creates a
// handle, and a name it cannot is refused rather than quietly served with a
// default backend.

#include <stdio.h>
#include <string.h>
#include "eai/common.h"
#include "eai_bci/api.h"
#include "eai_bci/eai_bci.h"

static int tests_run = 0, tests_passed = 0, tests_failed = 0;
#define TEST(name) do { tests_run++; printf("  TEST %-40s ", #name); } while(0)
#define PASS() do { tests_passed++; printf("[PASS]\n"); } while(0)
#define FAIL(msg) do { tests_failed++; printf("[FAIL] %s\n", msg); } while(0)

static void test_default_device_is_the_simulator(void)
{
    TEST(default_device_is_the_simulator);
    eai_bci_handle_t *h = eai_bci_create(NULL, NULL, NULL);
    if (!h) { FAIL("NULL names should select the defaults"); return; }
    eai_bci_destroy(h);
    PASS();
}

static void test_named_simulator_is_accepted(void)
{
    TEST(named_simulator_is_accepted);
    eai_bci_handle_t *h = eai_bci_create("simulator", "threshold", "log");
    if (!h) { FAIL("the simulator is always built in"); return; }
    eai_bci_destroy(h);
    PASS();
}

// The regression this file exists for: eai_bci_create used to (void) its
// device_name and always install eai_bci_device_simulator_ops, so a caller
// asking for a headset received fabricated simulator data that looked real.
static void test_unknown_device_is_refused(void)
{
    TEST(unknown_device_is_refused);
    eai_bci_handle_t *h = eai_bci_create("no-such-device", "threshold", "log");
    if (h) {
        eai_bci_destroy(h);
        FAIL("an unknown device name must not fall back to the simulator");
        return;
    }
    PASS();
}

static void test_unknown_decoder_is_refused(void)
{
    TEST(unknown_decoder_is_refused);
    eai_bci_handle_t *h = eai_bci_create("simulator", "no-such-decoder", "log");
    if (h) { eai_bci_destroy(h); FAIL("unknown decoder must be refused"); return; }
    PASS();
}

static void test_unknown_output_is_refused(void)
{
    TEST(unknown_output_is_refused);
    eai_bci_handle_t *h = eai_bci_create("simulator", "threshold", "no-such-output");
    if (h) { eai_bci_destroy(h); FAIL("unknown output must be refused"); return; }
    PASS();
}

static void test_known_alternate_decoder_is_accepted(void)
{
    TEST(known_alternate_decoder_is_accepted);
    eai_bci_handle_t *h = eai_bci_create("simulator", "snn", "gpio");
    if (!h) { FAIL("snn/gpio are built in and must be selectable"); return; }
    eai_bci_destroy(h);
    PASS();
}

// A device whose backend is compiled out is as unavailable as one that does
// not exist, and must be refused for the same reason. When the backend IS
// compiled in the name must be accepted, which is what proves the name is
// read rather than ignored.
static void test_optional_device_matches_its_build_flag(void)
{
    TEST(optional_device_matches_its_build_flag);
    eai_bci_handle_t *h = eai_bci_create("openbci", "threshold", "log");
#ifdef EAI_BCI_OPENBCI_ENABLED
    if (!h) { FAIL("openbci is enabled in this build and must be selectable"); return; }
    eai_bci_destroy(h);
#else
    if (h) {
        eai_bci_destroy(h);
        FAIL("openbci is not in this build; it must not yield a simulator");
        return;
    }
#endif
    PASS();
}

static void test_a_created_handle_still_runs(void)
{
    TEST(a_created_handle_still_runs);
    eai_bci_handle_t *h = eai_bci_create("simulator", "threshold", "log");
    if (!h) { FAIL("create failed"); return; }
    if (eai_bci_start(h) != 0) { eai_bci_destroy(h); FAIL("start failed"); return; }
    if (eai_bci_get_channel_count(h) <= 0) {
        eai_bci_stop(h); eai_bci_destroy(h);
        FAIL("no channels"); return;
    }
    eai_bci_stop(h);
    eai_bci_destroy(h);
    PASS();
}

int main(void)
{
    printf("=== EAI BCI C API Tests ===\n");

    test_default_device_is_the_simulator();
    test_named_simulator_is_accepted();
    test_unknown_device_is_refused();
    test_unknown_decoder_is_refused();
    test_unknown_output_is_refused();
    test_known_alternate_decoder_is_accepted();
    test_optional_device_matches_its_build_flag();
    test_a_created_handle_still_runs();

    printf("\nResults: %d/%d passed, %d failed\n", tests_passed, tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
