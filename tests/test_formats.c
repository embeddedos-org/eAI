// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// Comprehensive tests for GGUF/ONNX loaders and format registry

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "eai/gguf.h"
#include "eai/onnx.h"

static int tests_run = 0, tests_passed = 0, tests_failed = 0;
#define TEST(name) do { tests_run++; printf("  TEST %-44s ", #name); } while(0)
#define PASS() do { tests_passed++; printf("[PASS]\n"); } while(0)
#define FAIL(msg) do { tests_failed++; printf("[FAIL] %s\n", msg); } while(0)

/* Format registry functions (defined in format_registry.c) */
typedef enum { EAI_FORMAT_UNKNOWN = 0, EAI_FORMAT_GGUF = 1, EAI_FORMAT_ONNX = 2 } eai_format_t;
extern eai_format_t eai_format_detect(const char *path);

/* Helper: create a minimal GGUF file for testing */
static const char *create_test_gguf(void)
{
    static const char *path = "test_model.gguf";
    FILE *fp = fopen(path, "wb");
    if (!fp) return NULL;

    /* GGUF header: magic(4) + version(4) + n_tensors(8) + n_kv(8) */
    uint32_t magic = 0x46554747; /* "GGUF" little-endian */
    uint32_t version = 3;
    uint64_t n_tensors = 0;
    uint64_t n_kv = 1;

    fwrite(&magic, 4, 1, fp);
    fwrite(&version, 4, 1, fp);
    fwrite(&n_tensors, 8, 1, fp);
    fwrite(&n_kv, 8, 1, fp);

    /* Write one KV pair: key="general.architecture", type=STRING, value="test" */
    const char *key = "general.architecture";
    uint64_t key_len = strlen(key);
    fwrite(&key_len, 8, 1, fp);
    fwrite(key, 1, (size_t)key_len, fp);

    uint32_t kv_type = 8; /* GGUF_TYPE_STRING */
    fwrite(&kv_type, 4, 1, fp);

    const char *val = "test";
    uint64_t val_len = strlen(val);
    fwrite(&val_len, 8, 1, fp);
    fwrite(val, 1, (size_t)val_len, fp);

    fclose(fp);
    return path;
}

/* Helper: create a GGUF file holding exactly one 2x2 tensor of the given GGML
 * element type. Enough of the format to drive eai_gguf_get_tensor(): header,
 * zero KV pairs, one tensor-info record, then an aligned data section. */
static const char *create_gguf_with_tensor(int ggml_type)
{
    static const char *path = "test_dtype.gguf";
    FILE *fp = fopen(path, "wb");
    if (!fp) return NULL;

    uint32_t magic = 0x46554747;
    uint32_t version = 3;
    uint64_t n_tensors = 1;
    uint64_t n_kv = 0;
    fwrite(&magic, 4, 1, fp);
    fwrite(&version, 4, 1, fp);
    fwrite(&n_tensors, 8, 1, fp);
    fwrite(&n_kv, 8, 1, fp);

    const char *tname = "blk.0.weight";
    uint64_t name_len = strlen(tname);
    fwrite(&name_len, 8, 1, fp);
    fwrite(tname, 1, (size_t)name_len, fp);

    uint32_t ndim = 2;
    fwrite(&ndim, 4, 1, fp);
    uint64_t shape[2] = { 2, 2 };
    fwrite(shape, 8, 2, fp);

    uint32_t dtype = (uint32_t)ggml_type;
    fwrite(&dtype, 4, 1, fp);

    uint64_t offset = 0;
    fwrite(&offset, 8, 1, fp);

    /* Pad to the 32-byte data alignment, then write a data section. */
    long pos = ftell(fp);
    long aligned = (pos + 31) & ~31L;
    for (long i = pos; i < aligned; i++) fputc(0, fp);
    uint8_t payload[64];
    memset(payload, 0x5a, sizeof(payload));
    fwrite(payload, 1, sizeof(payload), fp);

    fclose(fp);
    return path;
}

/* Helper: create a minimal file that looks like ONNX (protobuf) */
static const char *create_test_onnx(void)
{
    static const char *path = "test_model.onnx";
    FILE *fp = fopen(path, "wb");
    if (!fp) return NULL;

    /* Minimal ONNX-like protobuf: field 1 (ir_version) varint = 7, padded to 4+ bytes */
    uint8_t data[] = { 0x08, 0x07, 0x10, 0x01 }; /* field 1=7, field 2=1 */
    fwrite(data, 1, sizeof(data), fp);

    fclose(fp);
    return path;
}

static void cleanup_test_files(void)
{
    remove("test_model.gguf");
    remove("test_dtype.gguf");
    remove("test_model.onnx");
    remove("test_bad_magic.bin");
}

/* ---- GGUF tensor dtype tests ---- */

/* eai_gguf_get_tensor() used to hardcode tensor->dtype = EAI_DTYPE_F32 and
 * discard the GGML type the tensor-info record carries. Almost every published
 * GGUF model is quantised, so that told callers a Q4_K or Q8_0 tensor was
 * float32 - which both misreads the bytes and overstates the tensor's length,
 * since a caller sizes the buffer from shape x sizeof(dtype). These pin that
 * the real type is reported, and that a type eai_dtype_t cannot describe is
 * refused rather than defaulted. */

struct dtype_case { int ggml; eai_dtype_t expect; const char *label; };

static void test_gguf_tensor_reports_mapped_dtypes(void)
{
    TEST(gguf_tensor_reports_mapped_dtypes);
    static const struct dtype_case cases[] = {
        { GGML_TYPE_F32,  EAI_DTYPE_F32,   "F32"  },
        { GGML_TYPE_F16,  EAI_DTYPE_F16,   "F16"  },
        { GGML_TYPE_BF16, EAI_DTYPE_BF16,  "BF16" },
        { GGML_TYPE_Q4_0, EAI_DTYPE_Q4_0,  "Q4_0" },
        { GGML_TYPE_Q4_1, EAI_DTYPE_Q4_1,  "Q4_1" },
        { GGML_TYPE_Q8_0, EAI_DTYPE_Q8_0,  "Q8_0" },
        { GGML_TYPE_I8,   EAI_DTYPE_INT8,  "I8"   },
        { GGML_TYPE_I32,  EAI_DTYPE_INT32, "I32"  },
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        const char *path = create_gguf_with_tensor(cases[i].ggml);
        if (!path) { FAIL("cannot create fixture"); return; }

        gguf_context_t ctx;
        if (eai_gguf_load(path, &ctx) != EAI_OK) { FAIL(cases[i].label); return; }

        eai_tensor_t t;
        eai_status_t st = eai_gguf_get_tensor(&ctx, "blk.0.weight", &t);
        if (st != EAI_OK) { FAIL(cases[i].label); eai_gguf_free(&ctx); return; }
        if (t.dtype != cases[i].expect) { FAIL(cases[i].label); eai_gguf_free(&ctx); return; }
        if (t.ndim != 2) { FAIL("ndim"); eai_gguf_free(&ctx); return; }
        eai_gguf_free(&ctx);
    }
    PASS();
}

static void test_gguf_tensor_refuses_types_it_cannot_describe(void)
{
    TEST(gguf_tensor_refuses_types_it_cannot_describe);
    /* Real GGUF element types with no eai_dtype_t equivalent. Q4_K and Q6_K in
     * particular are what mainstream quantised models actually ship as, so
     * this is the common case, not an exotic one. */
    static const int unsupported[] = {
        GGML_TYPE_Q5_0, GGML_TYPE_Q5_1, GGML_TYPE_Q2_K, GGML_TYPE_Q3_K,
        GGML_TYPE_Q4_K, GGML_TYPE_Q5_K, GGML_TYPE_Q6_K, GGML_TYPE_IQ4_XS,
        GGML_TYPE_I16,  GGML_TYPE_I64,  GGML_TYPE_F64,
    };

    for (size_t i = 0; i < sizeof(unsupported) / sizeof(unsupported[0]); i++) {
        const char *path = create_gguf_with_tensor(unsupported[i]);
        if (!path) { FAIL("cannot create fixture"); return; }

        gguf_context_t ctx;
        if (eai_gguf_load(path, &ctx) != EAI_OK) { FAIL("load"); return; }

        eai_tensor_t t;
        memset(&t, 0xA5, sizeof(t));
        eai_status_t st = eai_gguf_get_tensor(&ctx, "blk.0.weight", &t);
        if (st == EAI_OK) { FAIL("accepted an unmappable GGML type"); eai_gguf_free(&ctx); return; }
        if (st != EAI_ERR_UNSUPPORTED) { FAIL("wrong status"); eai_gguf_free(&ctx); return; }
        /* A refused tensor must not leave a data pointer behind. */
        if (t.data != NULL) { FAIL("data pointer set on refusal"); eai_gguf_free(&ctx); return; }
        eai_gguf_free(&ctx);
    }
    PASS();
}

static void test_gguf_quantised_tensor_is_not_reported_as_f32(void)
{
    /* The defect stated directly: before the fix every one of these came back
     * EAI_OK with dtype EAI_DTYPE_F32. */
    TEST(gguf_quantised_tensor_is_not_reported_as_f32);
    static const int quantised[] = { GGML_TYPE_Q4_0, GGML_TYPE_Q8_0, GGML_TYPE_Q4_K, GGML_TYPE_Q6_K };

    for (size_t i = 0; i < sizeof(quantised) / sizeof(quantised[0]); i++) {
        const char *path = create_gguf_with_tensor(quantised[i]);
        if (!path) { FAIL("cannot create fixture"); return; }

        gguf_context_t ctx;
        if (eai_gguf_load(path, &ctx) != EAI_OK) { FAIL("load"); return; }

        eai_tensor_t t;
        eai_status_t st = eai_gguf_get_tensor(&ctx, "blk.0.weight", &t);
        if (st == EAI_OK && t.dtype == EAI_DTYPE_F32) {
            FAIL("quantised tensor reported as F32");
            eai_gguf_free(&ctx);
            return;
        }
        eai_gguf_free(&ctx);
    }
    PASS();
}

/* ---- GGUF loader tests ---- */

static void test_gguf_load_null_path(void)
{
    TEST(gguf_load_null_path);
    gguf_context_t ctx;
    eai_status_t st = eai_gguf_load(NULL, &ctx);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for NULL path"); return; }
    PASS();
}

static void test_gguf_load_null_ctx(void)
{
    TEST(gguf_load_null_ctx);
    eai_status_t st = eai_gguf_load("some_file.gguf", NULL);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for NULL ctx"); return; }
    PASS();
}

static void test_gguf_load_nonexistent(void)
{
    TEST(gguf_load_nonexistent);
    gguf_context_t ctx;
    eai_status_t st = eai_gguf_load("nonexistent_file_xyz.gguf", &ctx);
    if (st != EAI_ERR_IO) { FAIL("expected IO error for missing file"); return; }
    PASS();
}

static void test_gguf_load_bad_magic(void)
{
    TEST(gguf_load_bad_magic);
    /* Create a file with wrong magic */
    FILE *fp = fopen("test_bad_magic.bin", "wb");
    if (!fp) { FAIL("cannot create test file"); return; }
    uint32_t bad_magic = 0xDEADBEEF;
    fwrite(&bad_magic, 4, 1, fp);
    fclose(fp);

    gguf_context_t ctx;
    eai_status_t st = eai_gguf_load("test_bad_magic.bin", &ctx);
    if (st != EAI_ERR_FORMAT) { FAIL("expected FORMAT error for bad magic"); return; }
    PASS();
}

static void test_gguf_load_valid(void)
{
    TEST(gguf_load_valid);
    const char *path = create_test_gguf();
    if (!path) { FAIL("cannot create test GGUF"); return; }

    gguf_context_t ctx;
    eai_status_t st = eai_gguf_load(path, &ctx);
    if (st != EAI_OK) { FAIL("load failed"); return; }

    if (ctx.magic != 0x46554747) { FAIL("magic mismatch"); eai_gguf_free(&ctx); return; }
    if (ctx.version != 3) { FAIL("version != 3"); eai_gguf_free(&ctx); return; }
    if (ctx.n_tensors != 0) { FAIL("n_tensors != 0"); eai_gguf_free(&ctx); return; }
    if (ctx.n_kv != 1) { FAIL("n_kv != 1"); eai_gguf_free(&ctx); return; }

    eai_gguf_free(&ctx);
    PASS();
}

static void test_gguf_get_str(void)
{
    TEST(gguf_get_str);
    const char *path = create_test_gguf();
    if (!path) { FAIL("cannot create test GGUF"); return; }

    gguf_context_t ctx;
    eai_gguf_load(path, &ctx);

    const char *arch = eai_gguf_get_str(&ctx, "general.architecture");
    if (!arch) { FAIL("get_str returned NULL"); eai_gguf_free(&ctx); return; }
    if (strcmp(arch, "test") != 0) { FAIL("expected 'test'"); eai_gguf_free(&ctx); return; }

    /* Non-existent key should return NULL */
    const char *missing = eai_gguf_get_str(&ctx, "nonexistent.key");
    if (missing != NULL) { FAIL("expected NULL for nonexistent key"); eai_gguf_free(&ctx); return; }

    eai_gguf_free(&ctx);
    PASS();
}

static void test_gguf_get_str_null_ctx(void)
{
    TEST(gguf_get_str_null_ctx);
    const char *result = eai_gguf_get_str(NULL, "any_key");
    if (result != NULL) { FAIL("expected NULL for NULL ctx"); return; }
    PASS();
}

static void test_gguf_get_str_null_key(void)
{
    TEST(gguf_get_str_null_key);
    gguf_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    const char *result = eai_gguf_get_str(&ctx, NULL);
    if (result != NULL) { FAIL("expected NULL for NULL key"); return; }
    PASS();
}

static void test_gguf_get_int_default(void)
{
    TEST(gguf_get_int_default);
    const char *path = create_test_gguf();
    if (!path) { FAIL("cannot create test GGUF"); return; }

    gguf_context_t ctx;
    eai_gguf_load(path, &ctx);

    /* Non-existent int key should return default */
    int val = eai_gguf_get_int(&ctx, "nonexistent.int", 42);
    if (val != 42) { FAIL("expected default value 42"); eai_gguf_free(&ctx); return; }

    /* NULL ctx should return default */
    val = eai_gguf_get_int(NULL, "any", 99);
    if (val != 99) { FAIL("expected 99 for NULL ctx"); eai_gguf_free(&ctx); return; }

    eai_gguf_free(&ctx);
    PASS();
}

static void test_gguf_get_tensor_nonexistent(void)
{
    TEST(gguf_get_tensor_nonexistent);
    const char *path = create_test_gguf();
    if (!path) { FAIL("cannot create test GGUF"); return; }

    gguf_context_t ctx;
    eai_gguf_load(path, &ctx);

    eai_tensor_t tensor;
    eai_status_t st = eai_gguf_get_tensor(&ctx, "nonexistent_tensor", &tensor);
    if (st != EAI_ERR_NOT_FOUND) { FAIL("expected NOT_FOUND"); eai_gguf_free(&ctx); return; }

    eai_gguf_free(&ctx);
    PASS();
}

static void test_gguf_get_tensor_null_params(void)
{
    TEST(gguf_get_tensor_null_params);
    eai_tensor_t tensor;
    eai_status_t st = eai_gguf_get_tensor(NULL, "name", &tensor);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for NULL ctx"); return; }

    gguf_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    st = eai_gguf_get_tensor(&ctx, NULL, &tensor);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for NULL name"); return; }

    st = eai_gguf_get_tensor(&ctx, "name", NULL);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for NULL tensor"); return; }

    PASS();
}

static void test_gguf_free_null(void)
{
    TEST(gguf_free_null);
    eai_gguf_free(NULL); /* should not crash */
    PASS();
}

static void test_gguf_free_double(void)
{
    TEST(gguf_free_double);
    const char *path = create_test_gguf();
    if (!path) { FAIL("cannot create test GGUF"); return; }

    gguf_context_t ctx;
    eai_gguf_load(path, &ctx);
    eai_gguf_free(&ctx);
    eai_gguf_free(&ctx); /* second free should be safe */
    PASS();
}

/* ---- ONNX loader tests ---- */

static void test_onnx_load_null_path(void)
{
    TEST(onnx_load_null_path);
    onnx_context_t ctx;
    eai_status_t st = eai_onnx_load(NULL, &ctx);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for NULL path"); return; }
    PASS();
}

static void test_onnx_load_null_ctx(void)
{
    TEST(onnx_load_null_ctx);
    eai_status_t st = eai_onnx_load("some_file.onnx", NULL);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for NULL ctx"); return; }
    PASS();
}

static void test_onnx_load_nonexistent(void)
{
    TEST(onnx_load_nonexistent);
    onnx_context_t ctx;
    eai_status_t st = eai_onnx_load("nonexistent_onnx_xyz.onnx", &ctx);
    if (st != EAI_ERR_IO) { FAIL("expected IO error"); return; }
    PASS();
}

static void test_onnx_load_valid(void)
{
    TEST(onnx_load_valid);
    const char *path = create_test_onnx();
    if (!path) { FAIL("cannot create test ONNX"); return; }

    onnx_context_t ctx;
    eai_status_t st = eai_onnx_load(path, &ctx);
    if (st != EAI_OK) { FAIL("load failed"); eai_onnx_free(&ctx); return; }

    /* The test file has ir_version = 7 */
    if (ctx.ir_version != 7) { FAIL("ir_version != 7"); eai_onnx_free(&ctx); return; }

    eai_onnx_free(&ctx);
    PASS();
}

static void test_onnx_to_graph_stub(void)
{
    TEST(onnx_to_graph_stub);
    onnx_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    eai_compute_graph_t graph;
    eai_status_t st = eai_onnx_to_graph(&ctx, &graph);
    /* Currently returns NOT_IMPLEMENTED */
    if (st != EAI_ERR_NOT_IMPLEMENTED) { FAIL("expected NOT_IMPLEMENTED"); return; }
    PASS();
}

static void test_onnx_to_graph_null(void)
{
    TEST(onnx_to_graph_null);
    eai_compute_graph_t graph;
    eai_status_t st = eai_onnx_to_graph(NULL, &graph);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for NULL ctx"); return; }

    onnx_context_t ctx;
    st = eai_onnx_to_graph(&ctx, NULL);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for NULL graph"); return; }
    PASS();
}

static void test_onnx_free_null(void)
{
    TEST(onnx_free_null);
    eai_onnx_free(NULL); /* should not crash */
    PASS();
}

/* ---- Format registry tests ---- */

static void test_format_detect_null(void)
{
    TEST(format_detect_null);
    eai_format_t fmt = eai_format_detect(NULL);
    if (fmt != EAI_FORMAT_UNKNOWN) { FAIL("expected UNKNOWN for NULL"); return; }
    PASS();
}

static void test_format_detect_nonexistent(void)
{
    TEST(format_detect_nonexistent);
    eai_format_t fmt = eai_format_detect("nonexistent_xyz_file.bin");
    if (fmt != EAI_FORMAT_UNKNOWN) { FAIL("expected UNKNOWN for nonexistent file"); return; }
    PASS();
}

static void test_format_detect_gguf_by_magic(void)
{
    TEST(format_detect_gguf_by_magic);
    const char *path = create_test_gguf();
    if (!path) { FAIL("cannot create test GGUF"); return; }
    eai_format_t fmt = eai_format_detect(path);
    if (fmt != EAI_FORMAT_GGUF) { FAIL("expected GGUF"); return; }
    PASS();
}

static void test_format_detect_onnx_by_magic(void)
{
    TEST(format_detect_onnx_by_magic);
    const char *path = create_test_onnx();
    if (!path) { FAIL("cannot create test ONNX"); return; }
    eai_format_t fmt = eai_format_detect(path);
    if (fmt != EAI_FORMAT_ONNX) { FAIL("expected ONNX"); return; }
    PASS();
}

static void test_format_detect_bad_magic_with_gguf_ext(void)
{
    TEST(format_detect_bad_magic_gguf_ext);
    /* Create a file with wrong magic but .gguf extension */
    const char *path = "test_fakename.gguf";
    FILE *fp = fopen(path, "wb");
    if (!fp) { FAIL("cannot create file"); return; }
    uint8_t data[] = {0x00, 0x00, 0x00, 0x00, 0x00};
    fwrite(data, 1, sizeof(data), fp);
    fclose(fp);

    eai_format_t fmt = eai_format_detect(path);
    /* Magic doesn't match GGUF or ONNX, but extension is .gguf → should detect as GGUF */
    if (fmt != EAI_FORMAT_GGUF) { FAIL("expected GGUF by extension fallback"); remove(path); return; }
    remove(path);
    PASS();
}

int main(void)
{
    printf("=== EAI Format Tests (Comprehensive) ===\n\n");

    test_gguf_load_null_path();
    test_gguf_load_null_ctx();
    test_gguf_load_nonexistent();
    test_gguf_load_bad_magic();
    test_gguf_load_valid();
    test_gguf_get_str();
    test_gguf_get_str_null_ctx();
    test_gguf_get_str_null_key();
    test_gguf_get_int_default();
    test_gguf_get_tensor_nonexistent();
    test_gguf_get_tensor_null_params();
    test_gguf_tensor_reports_mapped_dtypes();
    test_gguf_tensor_refuses_types_it_cannot_describe();
    test_gguf_quantised_tensor_is_not_reported_as_f32();
    test_gguf_free_null();
    test_gguf_free_double();
    test_onnx_load_null_path();
    test_onnx_load_null_ctx();
    test_onnx_load_nonexistent();
    test_onnx_load_valid();
    test_onnx_to_graph_stub();
    test_onnx_to_graph_null();
    test_onnx_free_null();
    test_format_detect_null();
    test_format_detect_nonexistent();
    test_format_detect_gguf_by_magic();
    test_format_detect_onnx_by_magic();
    test_format_detect_bad_magic_with_gguf_ext();

    cleanup_test_files();

    printf("\n=== Results: %d/%d passed, %d failed ===\n",
           tests_passed, tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
