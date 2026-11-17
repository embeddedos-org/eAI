// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// ISO/IEC 25000 | ISO/IEC/IEEE 15288:2023

#include <stdio.h>
#include <string.h>
#include "eai/common.h"

static int tests_run = 0, tests_passed = 0, tests_failed = 0;
#define TEST(name) do { tests_run++; printf("  TEST %-40s ", #name); } while(0)
#define PASS() do { tests_passed++; printf("[PASS]\n"); } while(0)
#define FAIL(msg) do { tests_failed++; printf("[FAIL] %s\n", msg); } while(0)

static eai_status_t dummy_exec(const eai_kv_t *args, int arg_count, eai_tool_result_t *result)
{
    (void)args; (void)arg_count;
    const char *msg = "ok";
    memcpy(result->data, msg, 3);
    result->len = 2;
    result->status = EAI_OK;
    return EAI_OK;
}

static void test_config_init(void)
{
    TEST(config_init);
    eai_config_t cfg;
    eai_status_t st = eai_config_init(&cfg);
    if (st != EAI_OK) { FAIL("init returned error"); return; }
    if (cfg.variant != EAI_VARIANT_MIN) { FAIL("wrong variant"); return; }
    if (cfg.mode != EAI_MODE_LOCAL) { FAIL("wrong mode"); return; }
    PASS();
}

static void test_bci_assistive_profile(void)
{
    TEST(bci_assistive_profile);
    eai_config_t cfg;
    eai_config_init(&cfg);
    eai_status_t st = eai_config_load_profile(&cfg, "bci-assistive");
    if (st != EAI_OK) { eai_config_free(&cfg); FAIL("load bci-assistive failed"); return; }
    if (cfg.variant != EAI_VARIANT_MIN) { eai_config_free(&cfg); FAIL("wrong variant"); return; }
    if (cfg.mode != EAI_MODE_LOCAL) { eai_config_free(&cfg); FAIL("wrong mode"); return; }
    if (cfg.policy.cloud_fallback != false) { eai_config_free(&cfg); FAIL("wrong cloud_fallback"); return; }
    if (cfg.observability != true) { eai_config_free(&cfg); FAIL("wrong observability"); return; }
    eai_config_free(&cfg);
    PASS();
}

static void test_tool_registry(void)
{
    TEST(tool_registry);
    eai_tool_registry_t reg;
    eai_tool_registry_init(&reg);

    eai_tool_t tool = {0};
    strncpy(tool.name, "test.tool", EAI_TOOL_NAME_MAX - 1);
    tool.description = "A test tool";
    tool.exec = dummy_exec;

    eai_status_t st = eai_tool_register(&reg, &tool);
    if (st != EAI_OK) { FAIL("register failed"); return; }

    eai_tool_t *found = eai_tool_find(&reg, "test.tool");
    if (!found) { FAIL("find returned NULL"); return; }
    if (strcmp(found->name, "test.tool") != 0) { FAIL("wrong name"); return; }
    PASS();
}

static void test_tool_exec(void)
{
    TEST(tool_exec);
    eai_tool_registry_t reg;
    eai_tool_registry_init(&reg);

    eai_tool_t tool = {0};
    strncpy(tool.name, "exec.tool", EAI_TOOL_NAME_MAX - 1);
    tool.exec = dummy_exec;
    eai_tool_register(&reg, &tool);

    eai_tool_t *found = eai_tool_find(&reg, "exec.tool");
    if (!found) { FAIL("find returned NULL"); return; }

    eai_tool_result_t result = {0};
    eai_status_t st = eai_tool_exec(found, NULL, 0, &result);
    if (st != EAI_OK) { FAIL("exec failed"); return; }
    if (result.len != 2) { FAIL("wrong result len"); return; }
    PASS();
}

static void test_security_ctx(void)
{
    TEST(security_ctx);
    eai_security_ctx_t ctx;
    eai_security_ctx_init(&ctx, "test-agent");

    eai_security_grant(&ctx, "tool.read");

    if (!eai_security_check(&ctx, "tool.read")) { FAIL("should allow tool.read"); return; }
    if (eai_security_check(&ctx, "tool.write")) { FAIL("should deny tool.write"); return; }
    PASS();
}

static void test_status_strings(void)
{
    TEST(status_strings);
    const char *ok = eai_status_str(EAI_OK);
    if (!ok) { FAIL("EAI_OK returned NULL"); return; }
    const char *inv = eai_status_str(EAI_ERR_INVALID);
    if (!inv) { FAIL("EAI_ERR_INVALID returned NULL"); return; }
    PASS();
}

static void test_new_error_codes(void)
{
    TEST(new_error_codes);
    const char *s;

    s = eai_status_str(EAI_ERR_PLATFORM);
    if (!s || strcmp(s, "ERR_PLATFORM") != 0) { FAIL("ERR_PLATFORM"); return; }

    s = eai_status_str(EAI_ERR_ACCEL);
    if (!s || strcmp(s, "ERR_ACCEL") != 0) { FAIL("ERR_ACCEL"); return; }

    s = eai_status_str(EAI_ERR_FORMAT);
    if (!s || strcmp(s, "ERR_FORMAT") != 0) { FAIL("ERR_FORMAT"); return; }

    s = eai_status_str(EAI_ERR_DELEGATE);
    if (!s || strcmp(s, "ERR_DELEGATE") != 0) { FAIL("ERR_DELEGATE"); return; }

    s = eai_status_str(EAI_ERR_NOT_IMPLEMENTED);
    if (!s || strcmp(s, "ERR_NOT_IMPLEMENTED") != 0) { FAIL("ERR_NOT_IMPLEMENTED"); return; }

    s = eai_status_str(EAI_ERR_HW_UNAVAILABLE);
    if (!s || strcmp(s, "ERR_HW_UNAVAILABLE") != 0) { FAIL("ERR_HW_UNAVAILABLE"); return; }

    PASS();
}

/* A config file in the layout of profiles/<name>/profile.yaml: the list items
 * under runtime.providers and connectors are not tools. */
static void test_config_file_list_sections(void)
{
    TEST(config_file_list_sections);
    const char *path = "test_config_lists.yaml";
    FILE *fp = fopen(path, "w");
    if (!fp) { FAIL("cannot write config file"); return; }
    fputs("ai:\n"
          "  variant: framework\n"
          "  mode: local-first\n"
          "\n"
          "  runtime:\n"
          "    providers:\n"
          "      - onnxruntime\n"
          "      - llama.cpp\n"
          "\n"
          "  tools:\n"
          "    - mqtt.publish\n"
          "    - http.get\n"
          "\n"
          "  connectors:\n"
          "    - mqtt\n"
          "    - modbus\n"
          "\n"
          "  hardware:\n"
          "    accelerator: cpu\n", fp);
    fclose(fp);

    eai_config_t cfg;
    eai_config_init(&cfg);
    eai_status_t st = eai_config_load_file(&cfg, path);
    remove(path);
    if (st != EAI_OK) { eai_config_free(&cfg); FAIL("load failed"); return; }

    int ok = cfg.tool_count == 2 &&
             strcmp(cfg.tools[0], "mqtt.publish") == 0 &&
             strcmp(cfg.tools[1], "http.get") == 0 &&
             cfg.connector_count == 2 &&
             strcmp(cfg.connectors[0], "mqtt") == 0 &&
             strcmp(cfg.connectors[1], "modbus") == 0 &&
             cfg.runtime_multi.provider_count == 2 &&
             strcmp(cfg.runtime_multi.providers[0], "onnxruntime") == 0 &&
             strcmp(cfg.runtime_multi.providers[1], "llama.cpp") == 0;
    if (!ok) {
        printf("(tools=%d connectors=%d providers=%d) ",
               cfg.tool_count, cfg.connector_count, cfg.runtime_multi.provider_count);
        eai_config_free(&cfg);
        FAIL("list items filed under the wrong section");
        return;
    }
    eai_config_free(&cfg);
    if (cfg.connector_count != 0 || cfg.runtime_multi.provider_count != 0) {
        FAIL("free left connectors or providers behind");
        return;
    }
    PASS();
}

/* A built-in profile followed by eai_config_free() (as `eai run --profile` does). */
static void test_profile_with_connectors_free(void)
{
    TEST(profile_with_connectors_free);
    eai_config_t cfg;
    eai_config_init(&cfg);
    if (eai_config_load_profile(&cfg, "industrial-gateway") != EAI_OK) {
        eai_config_free(&cfg); FAIL("load industrial-gateway failed"); return;
    }
    if (cfg.connector_count != 3 || strcmp(cfg.connectors[1], "opcua") != 0 ||
        cfg.runtime_multi.provider_count != 2 ||
        strcmp(cfg.runtime_multi.providers[1], "llama.cpp") != 0) {
        eai_config_free(&cfg); FAIL("wrong connectors or providers"); return;
    }
    eai_config_free(&cfg);
    PASS();
}

int main(void)
{
    printf("=== EAI Common Tests ===\n\n");

    test_config_init();
    test_bci_assistive_profile();
    test_config_file_list_sections();
    test_profile_with_connectors_free();
    test_tool_registry();
    test_tool_exec();
    test_security_ctx();
    test_status_strings();
    test_new_error_codes();

    printf("\n=== Results: %d/%d passed, %d failed ===\n",
           tests_passed, tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
