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

/* A name longer than the stored 63 characters must keep finding the entry it
 * created: one metric that accumulates, and a span that can be ended. */
#define LONG_OBS_NAME "inference.latency.model_tinyllama_1_1b_chat_q4_0.backend_cpu.thread_0"

static void test_observability_long_metric_name(void)
{
    TEST(observability_long_metric_name);
    eai_fw_observability_t obs;
    eai_fw_obs_init(&obs, true);

    for (int i = 0; i < 3; i++) {
        if (eai_fw_obs_counter_inc(&obs, LONG_OBS_NAME, 1.0) != EAI_OK) {
            FAIL("counter_inc failed"); return;
        }
    }
    if (obs.metric_count != 1) { FAIL("each counter_inc created a new metric"); return; }
    if (obs.metrics[0].value != 3.0) { FAIL("counter did not accumulate to 3"); return; }

    eai_fw_obs_init(&obs, true);
    eai_fw_obs_gauge_set(&obs, LONG_OBS_NAME, 1.0);
    eai_fw_obs_gauge_set(&obs, LONG_OBS_NAME, 2.0);
    if (obs.metric_count != 1) { FAIL("each gauge_set created a new metric"); return; }
    if (obs.metrics[0].value != 2.0) { FAIL("gauge does not hold the last value"); return; }
    PASS();
}

static void test_observability_long_span_name(void)
{
    TEST(observability_long_span_name);
    eai_fw_observability_t obs;
    eai_fw_obs_init(&obs, true);

    if (eai_fw_obs_span_start(&obs, LONG_OBS_NAME, NULL) != EAI_OK) {
        FAIL("span_start failed"); return;
    }
    if (eai_fw_obs_span_end(&obs, LONG_OBS_NAME) != EAI_OK) {
        FAIL("span_end did not find the span it started"); return;
    }
    if (obs.spans[0].active) { FAIL("span still active"); return; }
    PASS();
}

static void test_observability_short_names_stay_distinct(void)
{
    TEST(observability_short_names_stay_distinct);
    eai_fw_observability_t obs;
    eai_fw_obs_init(&obs, true);

    eai_fw_obs_counter_inc(&obs, "req", 1.0);
    eai_fw_obs_counter_inc(&obs, "requests", 1.0);
    eai_fw_obs_counter_inc(&obs, "req", 1.0);
    if (obs.metric_count != 2) { FAIL("expected two metrics"); return; }
    if (obs.metrics[0].value != 2.0 || obs.metrics[1].value != 1.0) {
        FAIL("prefix names were merged"); return;
    }
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

/* eai_fw_memory_t holds 1024 entries (~2.2 MB): keep it off the stack. */
static eai_fw_memory_t g_mem;
static eai_fw_memory_t g_mem_reloaded;

/* Save g_mem to dir and load it into g_mem_reloaded. */
static int mem_roundtrip(const char *dir)
{
    if (eai_fw_mem_save(&g_mem) != EAI_OK) return -1;
    eai_fw_mem_init(&g_mem_reloaded, dir);
    if (eai_fw_mem_load(&g_mem_reloaded) != EAI_OK) return -1;
    return 0;
}

static void test_fw_mem_update_to_no_ttl_is_saved(void)
{
    TEST(fw_mem_update_to_no_ttl_is_saved);
    const char *dir = ".";
    eai_fw_mem_init(&g_mem, dir);
    /* Stored first with a TTL (not persistent), then updated to never expire. */
    eai_fw_mem_set(&g_mem, "agent", "goal", "draft", 60);
    eai_fw_mem_set(&g_mem, "agent", "goal", "final", 0);

    if (mem_roundtrip(dir) != 0) { FAIL("save/load failed"); return; }
    const char *v = eai_fw_mem_get(&g_mem_reloaded, "agent", "goal");
    remove("./memory.dat");
    if (!v || strcmp(v, "final") != 0) {
        FAIL("entry updated to ttl 0 was not saved");
        return;
    }
    PASS();
}

static void test_fw_mem_update_to_ttl_is_not_saved(void)
{
    TEST(fw_mem_update_to_ttl_is_not_saved);
    const char *dir = ".";
    eai_fw_mem_init(&g_mem, dir);
    /* Stored first with no expiry (persistent), then given a TTL. */
    eai_fw_mem_set(&g_mem, "agent", "session", "abc", 0);
    eai_fw_mem_set(&g_mem, "agent", "session", "abc", 60);

    if (mem_roundtrip(dir) != 0) { FAIL("save/load failed"); return; }
    const char *v = eai_fw_mem_get(&g_mem_reloaded, "agent", "session");
    remove("./memory.dat");
    if (v) {
        FAIL("entry updated to a ttl was saved and reloaded with no expiry");
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
    test_observability_long_metric_name();
    test_observability_long_span_name();
    test_observability_short_names_stay_distinct();
    test_fed_aggregate_skips_removed_participant();
    test_fw_mem_update_to_no_ttl_is_saved();
    test_fw_mem_update_to_ttl_is_not_saved();

    printf("\n=== Results: %d/%d passed, %d failed ===\n",
           tests_passed, tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
