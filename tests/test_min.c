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

int main(void)
{
    printf("=== EAI Min Tests ===\n");

    test_memory_lite();
    test_router_mode();
    test_cloud_https_not_downgraded();
    test_load_unparseable_model_falls_back();
    test_load_missing_model_falls_back();
    test_load_empty_model_falls_back();
    test_load_mismatched_bias_model_falls_back();

    printf("\nResults: %d/%d passed, %d failed\n", tests_passed, tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
