// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// ISO/IEC 25000 | ISO/IEC/IEEE 15288:2023

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "eai/common.h"
#include "eai_min/eai_min.h"

static int tests_run = 0, tests_passed = 0, tests_failed = 0;
#define TEST(name) do { tests_run++; printf("  TEST %-40s ", #name); } while(0)
#define PASS() do { tests_passed++; printf("[PASS]\n"); } while(0)
#define FAIL(msg) do { tests_failed++; printf("[FAIL] %s\n", msg); } while(0)

static void test_memory_lite(void)
{
    TEST(memory_lite);
    eai_mem_lite_t mem;
    eai_status_t st = eai_mem_lite_init(&mem, NULL);
    if (st != EAI_OK) { FAIL("init failed"); return; }

    eai_mem_lite_set(&mem, "x", "hello", false);

    const char *val = eai_mem_lite_get(&mem, "x");
    if (!val || strcmp(val, "hello") != 0) {
        FAIL("expected 'hello' for key 'x'");
        return;
    }

    const char *unknown = eai_mem_lite_get(&mem, "nonexistent");
    if (unknown != NULL) {
        FAIL("expected NULL for unknown key");
        return;
    }
    PASS();
}

/* A memory key longer than the stored 63 characters must not be stored as
 * an entry that get() can never find and every set() duplicates. */
#define MEM_KEY_63 "sensor.calibration.thermistor_ntc_10k_beta_3950.channel_0_offse"
#define MEM_KEY_70 "sensor.calibration.thermistor_ntc_10k_beta_3950.channel_0_offset_deg_c"

static void test_memory_lite_long_key(void)
{
    TEST(memory_lite_long_key);
    static eai_mem_lite_t mem;
    eai_mem_lite_init(&mem, NULL);

    if (strlen(MEM_KEY_63) != EAI_MEM_KEY_MAX - 1 || strlen(MEM_KEY_70) != 70) {
        FAIL("test key lengths are wrong"); return;
    }

    /* The longest key that fits still round-trips and updates in place. */
    if (eai_mem_lite_set(&mem, MEM_KEY_63, "0.25", true) != EAI_OK ||
        eai_mem_lite_set(&mem, MEM_KEY_63, "0.50", true) != EAI_OK) {
        FAIL("set of a 63-character key failed"); return;
    }
    const char *v = eai_mem_lite_get(&mem, MEM_KEY_63);
    if (!v || strcmp(v, "0.50") != 0 || eai_mem_lite_count(&mem) != 1) {
        FAIL("63-character key did not round-trip"); return;
    }

    /* A 70-character key: set() must report the failure, not return OK
     * and leave an unreachable (and, on every call, duplicated) entry. */
    for (int i = 0; i < 3; i++) {
        if (eai_mem_lite_set(&mem, MEM_KEY_70, "1.5", true) != EAI_ERR_INVALID) {
            FAIL("set of a 70-character key returned OK but get() cannot find it");
            return;
        }
    }
    if (eai_mem_lite_count(&mem) != 1) {
        FAIL("set of a 70-character key added entries"); return;
    }
    if (eai_mem_lite_get(&mem, MEM_KEY_70) != NULL) {
        FAIL("get of a 70-character key found something"); return;
    }
    v = eai_mem_lite_get(&mem, MEM_KEY_63);
    if (!v || strcmp(v, "0.50") != 0) {
        FAIL("70-character key changed the 63-character entry"); return;
    }
    PASS();
}

/* A counter name longer than the stored 63 characters must keep finding the
 * counter it created. */
#define LONG_COUNTER_NAME "inference.errors.model_tinyllama_1_1b_chat_q4_0.backend_cpu.thread_0"

static void test_obs_lite_long_counter_name(void)
{
    TEST(obs_lite_long_counter_name);
    eai_min_obs_lite_t obs;
    eai_min_obs_init(&obs);

    for (int i = 0; i < 3; i++) {
        if (eai_min_obs_counter_inc(&obs, LONG_COUNTER_NAME, 1) != EAI_OK) {
            FAIL("counter_inc failed"); return;
        }
    }
    if (obs.counter_count != 1) { FAIL("each counter_inc created a new counter"); return; }
    if (eai_min_obs_counter_get(&obs, LONG_COUNTER_NAME) != 3) {
        FAIL("counter_get did not return 3"); return;
    }

    eai_min_obs_counter_inc(&obs, "err", 1);
    eai_min_obs_counter_inc(&obs, "errors", 5);
    if (eai_min_obs_counter_get(&obs, "err") != 1 ||
        eai_min_obs_counter_get(&obs, "errors") != 5) {
        FAIL("prefix names were merged"); return;
    }
    PASS();
}

/* A sensor name longer than the stored 63 characters must not be registered
 * as a sensor that eai_sensor_read() can never find. */
#define SENSOR_NAME_63 "board0.i2c1.bme280_0x76.ambient_temperature_degrees_celsius_raw"
#define SENSOR_NAME_70 "board0.i2c1.bme280_0x76.ambient_temperature_degrees_celsius_raw_avg_1s"

static eai_status_t fixed_sensor_read(int sensor_id, float *value, void *user_data)
{
    (void)sensor_id;
    *value = *(const float *)user_data;
    return EAI_OK;
}

static void test_sensor_long_name(void)
{
    TEST(sensor_long_name);
    static eai_sensor_registry_t reg;
    static float reading = 21.5f;
    float v = 0.0f;
    eai_sensor_registry_init(&reg);

    if (strlen(SENSOR_NAME_63) != EAI_SENSOR_NAME_MAX - 1 || strlen(SENSOR_NAME_70) != 70) {
        FAIL("test name lengths are wrong"); return;
    }

    /* The longest name that fits registers once and can be read back. */
    if (eai_sensor_register(&reg, SENSOR_NAME_63, EAI_SENSOR_TEMPERATURE,
                            fixed_sensor_read, &reading) != EAI_OK) {
        FAIL("register of a 63-character name failed"); return;
    }
    if (eai_sensor_register(&reg, SENSOR_NAME_63, EAI_SENSOR_TEMPERATURE,
                            fixed_sensor_read, &reading) == EAI_OK) {
        FAIL("63-character name registered twice"); return;
    }
    if (eai_sensor_read(&reg, SENSOR_NAME_63, &v) != EAI_OK || v != 21.5f) {
        FAIL("63-character name could not be read"); return;
    }

    /* A 70-character name: register must report the failure, not return OK
     * and leave a sensor that read() cannot find (and, on every call, add
     * another one). */
    for (int i = 0; i < 3; i++) {
        if (eai_sensor_register(&reg, SENSOR_NAME_70, EAI_SENSOR_TEMPERATURE,
                                fixed_sensor_read, &reading) != EAI_ERR_INVALID) {
            FAIL("register of a 70-character name returned OK but read() cannot find it");
            return;
        }
    }
    if (reg.count != 1 || eai_sensor_count_active(&reg) != 1) {
        FAIL("register of a 70-character name added sensors"); return;
    }
    if (eai_sensor_find(&reg, SENSOR_NAME_70) != NULL) {
        FAIL("find of a 70-character name found something"); return;
    }
    PASS();
}

static void test_router_mode(void)
{
    TEST(router_mode);
    eai_min_router_t router;
    eai_min_router_init(&router, EAI_ROUTE_LOCAL);

    eai_inference_input_t input = {0};
    input.text = "test input";
    input.text_len = 10;

    eai_route_target_t decision = eai_min_router_decide(&router, &input);
    if (decision != EAI_ROUTE_LOCAL) {
        FAIL("expected EAI_ROUTE_LOCAL for local mode");
        return;
    }
    PASS();
}

/* The minimal build has no TLS. An https:// cloud endpoint used to be
 * downgraded to plain HTTP on port 80 while the "Authorization: Bearer"
 * header was still sent, leaking the API key in cleartext. The router must
 * refuse the request instead. 127.0.0.1 keeps the old behaviour local: it
 * tried 127.0.0.1:80 and returned a connect error, never UNSUPPORTED. */
static void test_cloud_https_not_downgraded(void)
{
    TEST(cloud_https_not_downgraded);
#ifdef EAI_HAS_HOST_SOCKETS
    eai_min_router_t router;
    eai_min_router_init(&router, EAI_ROUTE_CLOUD);
    eai_min_router_set_cloud(&router, "https://127.0.0.1/v1/chat/completions",
                             "sk-test-not-a-real-key");

    eai_inference_input_t in = {0};
    in.text = "hello";
    in.text_len = 5;
    eai_inference_output_t out;
    memset(&out, 0, sizeof(out));

    eai_status_t st = eai_min_router_infer_cloud(&router, &in, &out);
    if (st != EAI_ERR_UNSUPPORTED) {
        char msg[96];
        snprintf(msg, sizeof(msg),
                 "https endpoint must be refused (got status %d, want %d)",
                 (int)st, (int)EAI_ERR_UNSUPPORTED);
        FAIL(msg);
        return;
    }
    if (out.text_len != 0) {
        FAIL("refused request must not produce output");
        return;
    }
#endif
    PASS();
}

/* Issue #43: stub_load_model closed the model FILE twice on the short-read
 * path (fclose after fread, then fclose again in the else branch) -- undefined
 * behavior. The fix removes the second close. A short read is not
 * deterministically triggerable with a regular file, so these pin the
 * observable contract instead: any unreadable/unparseable model falls back
 * to stub mode cleanly (EAI_OK, runtime usable), with no crash or UB.
 * Under ASan the old double-close is caught if the path executes. */
static void write_garbage_file(const char *path, size_t nbytes)
{
    FILE *f = fopen(path, "wb");
    if (!f) { FAIL("could not create garbage model file"); return; }
    for (size_t i = 0; i < nbytes; i++) fputc((int)(i & 0xFF), f);
    fclose(f);
}

static void test_load_model_fallback(const char *label, const char *model_path)
{
    TEST(load_fallback);
    eai_min_runtime_t rt;
    eai_status_t st = eai_min_runtime_create(&rt, EAI_RUNTIME_ONNX);
    if (st != EAI_OK) { FAIL("runtime create failed"); return; }

    eai_model_manifest_t manifest;
    memset(&manifest, 0, sizeof(manifest));
    strncpy(manifest.name, "fallback-test", sizeof(manifest.name) - 1);

    st = eai_min_runtime_load(&rt, model_path, &manifest);
    if (st != EAI_OK) { FAIL("load did not fall back to stub cleanly"); return; }
    if (!rt.base.loaded) { FAIL("runtime not marked loaded after fallback"); return; }

    /* The runtime must still be usable after the fallback. */
    char out[64];
    st = eai_min_runtime_infer(&rt, "hello", out, sizeof(out));
    if (st != EAI_OK) { FAIL("infer failed after fallback load"); return; }

    eai_min_runtime_destroy(&rt);
    printf("(%s) ", label);
    PASS();
}

static void test_load_unparseable_model_falls_back(void)
{
    const char *path = "test_min_garbage.eaim";
    write_garbage_file(path, 1024);
    test_load_model_fallback("unparseable", path);
    remove(path);
}

static void test_load_missing_model_falls_back(void)
{
    test_load_model_fallback("missing", "test_min_does_not_exist.eaim");
}

static void test_load_empty_model_falls_back(void)
{
    const char *path = "test_min_empty.eaim";
    write_garbage_file(path, 0);
    test_load_model_fallback("empty", path);
    remove(path);
}

static void test_load_mismatched_bias_model_falls_back(void)
{
    const char *path = "test_min_bad_bias.eaim";
    FILE *f = fopen(path, "wb");
    if (!f) { FAIL("could not create bad bias model file"); return; }

    uint32_t hdr[8] = {0x4D494145u, 1u, 1u, 2u, 4u, 0u, 0u, 0u};
    /* bias_count = 1 != rows = 4 */
    uint32_t desc[4] = {4u, 2u, 1u, 0u};
    float weights[9] = {0};
    fwrite(hdr, sizeof(uint32_t), 8, f);
    fwrite(desc, sizeof(uint32_t), 4, f);
    fwrite(weights, sizeof(float), 9, f);
    fclose(f);

    test_load_model_fallback("bad-bias", path);
    remove(path);
}

/* After a "CALL:<tool>(...)" reply the agent runs the tool and thinks again.
 * The next prompt has to carry the tool's result: the agent used to rebuild
 * it from the goal alone, so the model never saw the result, asked for the
 * same call again, and the run ended at max_iterations with the model's
 * question unanswered. This scripted model answers only once the prompt
 * contains the result. */
static char agent_prompts[4][EAI_AGENT_PROMPT_MAX];
static int  agent_infer_calls;

static eai_status_t scripted_infer(eai_runtime_t *rt, const eai_inference_input_t *in,
                                   eai_inference_output_t *out)
{
    (void)rt;
    if (agent_infer_calls < 4) {
        snprintf(agent_prompts[agent_infer_calls], sizeof(agent_prompts[0]),
                 "%.*s", (int)in->text_len, in->text);
    }
    agent_infer_calls++;
    const char *reply = strstr(in->text, "Tool clock.now returned: 12:34")
                            ? "It is 12:34."
                            : "CALL:clock.now()";
    snprintf(out->text, sizeof(out->text), "%s", reply);
    out->text_len = strlen(out->text);
    return EAI_OK;
}

static const eai_runtime_ops_t scripted_ops = { .name = "scripted", .infer = scripted_infer };

static eai_status_t clock_now_exec(const eai_kv_t *args, int arg_count,
                                   eai_tool_result_t *result)
{
    (void)args; (void)arg_count;
    snprintf(result->data, sizeof(result->data), "12:34");
    result->len = strlen(result->data);
    return EAI_OK;
}

static void test_agent_tool_result_reaches_model(void)
{
    TEST(agent_tool_result_reaches_model);
    eai_min_runtime_t rt;
    memset(&rt, 0, sizeof(rt));
    rt.base.ops = &scripted_ops;
    rt.base.loaded = true;
    rt.initialized = true;

    eai_tool_registry_t tools;
    eai_tool_registry_init(&tools);
    eai_tool_t clock_tool = {0};
    snprintf(clock_tool.name, sizeof(clock_tool.name), "clock.now");
    clock_tool.exec = clock_now_exec;
    if (eai_tool_register(&tools, &clock_tool) != EAI_OK) { FAIL("tool register failed"); return; }

    eai_min_agent_t agent;
    eai_min_agent_init(&agent, &rt, &tools, NULL);
    agent_infer_calls = 0;
    eai_agent_task_t task = { .goal = "What time is it?", .max_iterations = 6 };

    eai_status_t st = eai_min_agent_run(&agent, &task);
    if (st != EAI_OK) { FAIL("agent run failed"); return; }
    if (agent_infer_calls < 2) { FAIL("agent did not think again after the tool call"); return; }
    if (!strstr(agent_prompts[1], "Tool clock.now returned: 12:34")) {
        FAIL("prompt after the tool call does not contain the tool's result"); return;
    }
    if (strcmp(eai_min_agent_output(&agent), "It is 12:34.") != 0) {
        char msg[160];
        snprintf(msg, sizeof(msg), "final output is \"%.100s\" after %d model calls",
                 eai_min_agent_output(&agent), agent_infer_calls);
        FAIL(msg); return;
    }
    if (agent_infer_calls != 2) { FAIL("expected exactly two model calls"); return; }
    if (strstr(agent_prompts[0], "Tool clock.now returned")) {
        FAIL("first prompt must not carry a tool result"); return;
    }
    PASS();
}

int main(void)
{
    printf("=== EAI Min Tests ===\n");

    test_memory_lite();
    test_memory_lite_long_key();
    test_obs_lite_long_counter_name();
    test_sensor_long_name();
    test_router_mode();
    test_cloud_https_not_downgraded();
    test_load_unparseable_model_falls_back();
    test_load_missing_model_falls_back();
    test_load_empty_model_falls_back();
    test_load_mismatched_bias_model_falls_back();
    test_agent_tool_result_reaches_model();

    printf("\nResults: %d/%d passed, %d failed\n", tests_passed, tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
