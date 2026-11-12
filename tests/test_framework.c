// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// ISO/IEC 25000 | ISO/IEC/IEEE 15288:2023

#include <stdio.h>
#include <string.h>
#include "eai/common.h"
#include "eai_fw/eai_framework.h"

static int tests_run = 0, tests_passed = 0, tests_failed = 0;
#define TEST(name) do { tests_run++; printf("  TEST %-40s ", #name); } while(0)
#define PASS() do { tests_passed++; printf("[PASS]\n"); } while(0)
#define FAIL(msg) do { tests_failed++; printf("[FAIL] %s\n", msg); } while(0)

static void test_policy_init(void)
{
    TEST(policy_init);
    eai_fw_policy_t pol;
    eai_status_t st = eai_fw_policy_init(&pol);
    if (st != EAI_OK) { FAIL("init failed"); return; }
    if (pol.rule_count != 0) { FAIL("expected 0 rules"); return; }
    PASS();
}

static void test_policy_check(void)
{
    TEST(policy_check);
    eai_fw_policy_t pol;
    eai_fw_policy_init(&pol);

    eai_policy_rule_t rule = {
        .subject   = "orchestrator",
        .resource  = "tool1",
        .operation = "exec",
        .action    = EAI_POLICY_ALLOW,
    };
    eai_fw_policy_add_rule(&pol, &rule);

    eai_policy_action_t act = eai_fw_policy_check(&pol, "orchestrator", "tool1", "exec");
    if (act != EAI_POLICY_ALLOW) { FAIL("should allow orchestrator/tool1/exec"); return; }

    act = eai_fw_policy_check(&pol, "orchestrator", "tool2", "exec");
    if (act != EAI_POLICY_ALLOW) { FAIL("default policy should allow unregistered resource"); return; }
    PASS();
}

static void test_observability_counters(void)
{
    TEST(observability_counters);
    eai_fw_observability_t obs;
    eai_status_t st = eai_fw_obs_init(&obs, true);
    if (st != EAI_OK) { FAIL("obs init failed"); return; }

    eai_fw_obs_counter_inc(&obs, "events", 1.0);
    eai_fw_obs_counter_inc(&obs, "events", 1.0);
    eai_fw_obs_counter_inc(&obs, "events", 1.0);

    PASS();
}

static void test_fed_aggregate_skips_removed_participant(void)
{
    TEST(fed_aggregate_skips_removed_participant);
    eai_fw_federated_t fed;
    eai_fw_fed_init(&fed, EAI_FED_COORDINATOR, NULL);
    eai_fw_fed_add_participant(&fed, "dev-a");
    eai_fw_fed_add_participant(&fed, "dev-b");
    eai_fw_fed_add_participant(&fed, "dev-c");

    /* Round 1: all three report; dev-c is far worse than the others. */
    if (eai_fw_fed_start_round(&fed) != EAI_OK) { FAIL("round 1 did not start"); return; }
    eai_fw_fed_submit_update(&fed, "dev-a", 0.5f, 100);
    eai_fw_fed_submit_update(&fed, "dev-b", 0.5f, 100);
    eai_fw_fed_submit_update(&fed, "dev-c", 9.0f, 100);
    if (eai_fw_fed_aggregate(&fed) != EAI_OK) { FAIL("round 1 aggregate failed"); return; }

    /* dev-c leaves; round 2 runs with dev-a and dev-b only. */
    eai_fw_fed_remove_participant(&fed, "dev-c");
    if (eai_fw_fed_start_round(&fed) != EAI_OK) { FAIL("round 2 did not start"); return; }
    eai_fw_fed_submit_update(&fed, "dev-a", 0.1f, 100);
    eai_fw_fed_submit_update(&fed, "dev-b", 0.1f, 100);
    if (eai_fw_fed_aggregate(&fed) != EAI_OK) { FAIL("round 2 aggregate failed"); return; }

    /* Round 2's loss is the mean of dev-a and dev-b, not dev-c's round-1 loss too. */
    if (fed.global_loss < 0.099f || fed.global_loss > 0.101f) {
        char msg[96];
        snprintf(msg, sizeof(msg), "global_loss %.4f, expected 0.1000", fed.global_loss);
        FAIL(msg);
        return;
    }
    PASS();
}

int main(void)
{
    printf("=== EAI Framework Tests ===\n\n");

    test_policy_init();
    test_policy_check();
    test_observability_counters();
    test_fed_aggregate_skips_removed_participant();

    printf("\n=== Results: %d/%d passed, %d failed ===\n",
           tests_passed, tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
