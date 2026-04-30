// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// Extended format tests — covers numeric KV types, tensor info, format_load,
// ONNX with multiple fields, and more GGUF edge cases

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "eai/gguf.h"
#include "eai/onnx.h"

static int tests_run = 0, tests_passed = 0, tests_failed = 0;
#define TEST(name) do { tests_run++; printf("  TEST %-44s ", #name); } while(0)
#define PASS() do { tests_passed++; printf("[PASS]\n"); } while(0)
#define FAIL(msg) do { tests_failed++; printf("[FAIL] %s\n", msg); } while(0)

/* format_registry.c externs */
typedef enum { EAI_FORMAT_UNKNOWN = 0, EAI_FORMAT_GGUF = 1, EAI_FORMAT_ONNX = 2 } eai_format_t;
extern eai_format_t eai_format_detect(const char *path);
extern eai_status_t eai_format_load(const char *path, void *ctx, eai_format_t *out_format);

/* Helper: write a GGUF string to file (uint64_t len + bytes) */
static void write_gguf_string(FILE *fp, const char *str)
{
    uint64_t len = strlen(str);
    fwrite(&len, 8, 1, fp);
    fwrite(str, 1, (size_t)len, fp);
}

/* ========== GGUF: numeric KV types ========== */

static const char *create_gguf_with_int_kv(void)
{
    static const char *path = "test_gguf_int_kv.gguf";
    FILE *fp = fopen(path, "wb");
    if (!fp) return NULL;

    uint32_t magic = 0x46554747;
    uint32_t version = 3;
    uint64_t n_tensors = 0;
    uint64_t n_kv = 2;
    fwrite(&magic, 4, 1, fp);
    fwrite(&version, 4, 1, fp);
    fwrite(&n_tensors, 8, 1, fp);
    fwrite(&n_kv, 8, 1, fp);

    /* KV 1: INT32 key */
    write_gguf_string(fp, "model.layers");
    uint32_t type_i32 = 5; /* GGUF_TYPE_INT32 */
    fwrite(&type_i32, 4, 1, fp);
    int32_t layers = 32;
    fwrite(&layers, 4, 1, fp);

    /* KV 2: UINT32 key */
    write_gguf_string(fp, "model.vocab_size");
    uint32_t type_u32 = 4; /* GGUF_TYPE_UINT32 */
    fwrite(&type_u32, 4, 1, fp);
    uint32_t vocab = 32000;
    fwrite(&vocab, 4, 1, fp);

    fclose(fp);
    return path;
}

static void test_gguf_get_int_i32(void)
{
    TEST(gguf_get_int_i32);
    const char *path = create_gguf_with_int_kv();
    if (!path) { FAIL("cannot create GGUF"); return; }

    gguf_context_t ctx;
    eai_status_t st = eai_gguf_load(path, &ctx);
    if (st != EAI_OK) { FAIL("load failed"); return; }

    int val = eai_gguf_get_int(&ctx, "model.layers", -1);
    if (val != 32) {
        char msg[64];
        snprintf(msg, sizeof(msg), "expected 32, got %d", val);
        FAIL(msg);
        eai_gguf_free(&ctx);
        return;
    }
    eai_gguf_free(&ctx);
    PASS();
}

static void test_gguf_get_int_u32(void)
{
    TEST(gguf_get_int_u32);
    const char *path = create_gguf_with_int_kv();
    if (!path) { FAIL("cannot create GGUF"); return; }

    gguf_context_t ctx;
    eai_gguf_load(path, &ctx);

    int val = eai_gguf_get_int(&ctx, "model.vocab_size", -1);
    if (val != 32000) {
        char msg[64];
        snprintf(msg, sizeof(msg), "expected 32000, got %d", val);
        FAIL(msg);
        eai_gguf_free(&ctx);
        return;
    }
    eai_gguf_free(&ctx);
    PASS();
}

static void test_gguf_get_int_wrong_type(void)
{
    TEST(gguf_get_int_wrong_type);
    /* Load the original string-based GGUF — key is type STRING */
    static const char *path = "test_gguf_str_tmp.gguf";
    FILE *fp = fopen(path, "wb");
    if (!fp) { FAIL("create failed"); return; }

    uint32_t magic = 0x46554747;
    uint32_t version = 3;
    uint64_t n_tensors = 0, n_kv = 1;
    fwrite(&magic, 4, 1, fp);
    fwrite(&version, 4, 1, fp);
    fwrite(&n_tensors, 8, 1, fp);
    fwrite(&n_kv, 8, 1, fp);
    write_gguf_string(fp, "some.string_key");
    uint32_t type_str = 8;
    fwrite(&type_str, 4, 1, fp);
    write_gguf_string(fp, "hello");
    fclose(fp);

    gguf_context_t ctx;
    eai_gguf_load(path, &ctx);

    /* Asking get_int for a STRING key should return default */
    int val = eai_gguf_get_int(&ctx, "some.string_key", 42);
    if (val != 42) { FAIL("should return default for STRING type"); eai_gguf_free(&ctx); return; }

    eai_gguf_free(&ctx);
    remove(path);
    PASS();
}

/* ========== GGUF: tensor info + data ========== */

static const char *create_gguf_with_tensor(void)
{
    static const char *path = "test_gguf_tensor.gguf";
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

    /* Tensor info: name, ndim, shape[], dtype, offset */
    write_gguf_string(fp, "weight.0");
    uint32_t ndim = 2;
    fwrite(&ndim, 4, 1, fp);
    uint64_t shape0 = 4, shape1 = 3;
    fwrite(&shape0, 8, 1, fp);
    fwrite(&shape1, 8, 1, fp);
    uint32_t dtype = 0; /* F32 in GGML = 0 */
    fwrite(&dtype, 4, 1, fp);
    uint64_t offset = 0;
    fwrite(&offset, 8, 1, fp);

    /* Align to 32 bytes */
    long pos = ftell(fp);
    long aligned = (pos + 31) & ~31L;
    for (long i = pos; i < aligned; i++) {
        uint8_t zero = 0;
        fwrite(&zero, 1, 1, fp);
    }

    /* Tensor data: 4x3 = 12 floats */
    for (int i = 0; i < 12; i++) {
        float val = (float)(i + 1);
        fwrite(&val, 4, 1, fp);
    }

    fclose(fp);
    return path;
}

static void test_gguf_load_with_tensor(void)
{
    TEST(gguf_load_with_tensor);
    const char *path = create_gguf_with_tensor();
    if (!path) { FAIL("cannot create GGUF with tensor"); return; }

    gguf_context_t ctx;
    eai_status_t st = eai_gguf_load(path, &ctx);
    if (st != EAI_OK) { FAIL("load failed"); return; }

    if (ctx.n_tensors != 1) { FAIL("n_tensors != 1"); eai_gguf_free(&ctx); return; }
    if (!ctx.tensors) { FAIL("tensors array is NULL"); eai_gguf_free(&ctx); return; }
    if (!ctx.tensors[0].name) { FAIL("tensor name is NULL"); eai_gguf_free(&ctx); return; }
    if (strcmp(ctx.tensors[0].name, "weight.0") != 0) { FAIL("wrong tensor name"); eai_gguf_free(&ctx); return; }
    if (ctx.tensors[0].ndim != 2) { FAIL("ndim != 2"); eai_gguf_free(&ctx); return; }
    if (ctx.tensors[0].shape[0] != 4) { FAIL("shape[0] != 4"); eai_gguf_free(&ctx); return; }
    if (ctx.tensors[0].shape[1] != 3) { FAIL("shape[1] != 3"); eai_gguf_free(&ctx); return; }

    eai_gguf_free(&ctx);
    PASS();
}

static void test_gguf_get_tensor_by_name(void)
{
    TEST(gguf_get_tensor_by_name);
    const char *path = create_gguf_with_tensor();
    if (!path) { FAIL("cannot create GGUF"); return; }

    gguf_context_t ctx;
    eai_gguf_load(path, &ctx);

    eai_tensor_t tensor;
    eai_status_t st = eai_gguf_get_tensor(&ctx, "weight.0", &tensor);
    if (st != EAI_OK) { FAIL("get_tensor failed"); eai_gguf_free(&ctx); return; }
    if (tensor.ndim != 2) { FAIL("ndim != 2"); eai_gguf_free(&ctx); return; }
    if (tensor.shape[0] != 4) { FAIL("shape[0] != 4"); eai_gguf_free(&ctx); return; }
    if (tensor.shape[1] != 3) { FAIL("shape[1] != 3"); eai_gguf_free(&ctx); return; }
    if (!tensor.data) { FAIL("data is NULL"); eai_gguf_free(&ctx); return; }
    if (tensor.owns_data) { FAIL("should not own data (view into GGUF)"); eai_gguf_free(&ctx); return; }

    /* Verify data content: first float should be 1.0 */
    float *fdata = (float *)tensor.data;
    if (fabsf(fdata[0] - 1.0f) > 0.01f) {
        char msg[64];
        snprintf(msg, sizeof(msg), "data[0] = %f, expected 1.0", fdata[0]);
        FAIL(msg);
        eai_gguf_free(&ctx);
        return;
    }

    eai_gguf_free(&ctx);
    PASS();
}

static void test_gguf_get_tensor_data_values(void)
{
    TEST(gguf_get_tensor_data_values);
    const char *path = create_gguf_with_tensor();
    if (!path) { FAIL("cannot create GGUF"); return; }

    gguf_context_t ctx;
    eai_gguf_load(path, &ctx);

    eai_tensor_t tensor;
    eai_gguf_get_tensor(&ctx, "weight.0", &tensor);

    float *fdata = (float *)tensor.data;
    /* We wrote 1.0, 2.0, ..., 12.0 */
    for (int i = 0; i < 12; i++) {
        float expected = (float)(i + 1);
        if (fabsf(fdata[i] - expected) > 0.01f) {
            char msg[64];
            snprintf(msg, sizeof(msg), "data[%d] = %f, expected %f", i, fdata[i], expected);
            FAIL(msg);
            eai_gguf_free(&ctx);
            return;
        }
    }

    eai_gguf_free(&ctx);
    PASS();
}

/* ========== GGUF: multiple KV types in single file ========== */

static const char *create_gguf_multi_kv(void)
{
    static const char *path = "test_gguf_multi_kv.gguf";
    FILE *fp = fopen(path, "wb");
    if (!fp) return NULL;

    uint32_t magic = 0x46554747;
    uint32_t version = 3;
    uint64_t n_tensors = 0;
    uint64_t n_kv = 3;
    fwrite(&magic, 4, 1, fp);
    fwrite(&version, 4, 1, fp);
    fwrite(&n_tensors, 8, 1, fp);
    fwrite(&n_kv, 8, 1, fp);

    /* KV 1: FLOAT32 */
    write_gguf_string(fp, "training.lr");
    uint32_t type_f32 = 6; /* GGUF_TYPE_FLOAT32 */
    fwrite(&type_f32, 4, 1, fp);
    float lr = 0.001f;
    fwrite(&lr, 4, 1, fp);

    /* KV 2: BOOL */
    write_gguf_string(fp, "model.is_finetuned");
    uint32_t type_bool = 7; /* GGUF_TYPE_BOOL */
    fwrite(&type_bool, 4, 1, fp);
    uint8_t is_ft = 1;
    fwrite(&is_ft, 1, 1, fp);

    /* KV 3: STRING */
    write_gguf_string(fp, "model.name");
    uint32_t type_str = 8;
    fwrite(&type_str, 4, 1, fp);
    write_gguf_string(fp, "test-model");

    fclose(fp);
    return path;
}

static void test_gguf_multi_kv_types(void)
{
    TEST(gguf_multi_kv_types);
    const char *path = create_gguf_multi_kv();
    if (!path) { FAIL("create failed"); return; }

    gguf_context_t ctx;
    eai_status_t st = eai_gguf_load(path, &ctx);
    if (st != EAI_OK) { FAIL("load failed"); return; }
    if (ctx.n_kv != 3) { FAIL("n_kv != 3"); eai_gguf_free(&ctx); return; }

    /* Read string KV */
    const char *name = eai_gguf_get_str(&ctx, "model.name");
    if (!name || strcmp(name, "test-model") != 0) { FAIL("string KV mismatch"); eai_gguf_free(&ctx); return; }

    /* INT for float type should return default */
    int int_val = eai_gguf_get_int(&ctx, "training.lr", -1);
    if (int_val != -1) { FAIL("float key should return default for get_int"); eai_gguf_free(&ctx); return; }

    eai_gguf_free(&ctx);
    PASS();
}

/* ========== ONNX: richer protobuf ========== */

static const char *create_onnx_multi_field(void)
{
    static const char *path = "test_onnx_multi.onnx";
    FILE *fp = fopen(path, "wb");
    if (!fp) return NULL;

    /* Field 1 (ir_version) = 8: tag = (1<<3)|0 = 0x08, value = 8 */
    uint8_t f1[] = { 0x08, 0x08 };
    fwrite(f1, 1, sizeof(f1), fp);

    /* Field 2 (opset_import) — LEN-prefixed, tag = (8<<3)|2 = 0x42 */
    /* Just a dummy 2-byte nested message */
    uint8_t f8[] = { 0x42, 0x02, 0x08, 0x0D }; /* nested: field1=varint(13) */
    fwrite(f8, 1, sizeof(f8), fp);

    /* Field 7 (graph) — LEN-prefixed, tag = (7<<3)|2 = 0x3A */
    uint8_t f7[] = { 0x3A, 0x04, 0x0A, 0x02, 0x67, 0x31 }; /* nested: field1=string("g1") */
    fwrite(f7, 1, sizeof(f7), fp);

    fclose(fp);
    return path;
}

static void test_onnx_multi_field(void)
{
    TEST(onnx_multi_field);
    const char *path = create_onnx_multi_field();
    if (!path) { FAIL("create failed"); return; }

    onnx_context_t ctx;
    eai_status_t st = eai_onnx_load(path, &ctx);
    if (st != EAI_OK) { FAIL("load failed"); eai_onnx_free(&ctx); return; }

    if (ctx.ir_version != 8) {
        char msg[64];
        snprintf(msg, sizeof(msg), "ir_version = %ld, expected 8", (long)ctx.ir_version);
        FAIL(msg);
        eai_onnx_free(&ctx);
        return;
    }

    eai_onnx_free(&ctx);
    PASS();
}

static void test_onnx_empty_file(void)
{
    TEST(onnx_empty_file);
    const char *path = "test_onnx_empty.onnx";
    FILE *fp = fopen(path, "wb");
    if (!fp) { FAIL("create failed"); return; }
    fclose(fp);

    onnx_context_t ctx;
    eai_status_t st = eai_onnx_load(path, &ctx);
    /* Empty file (0 bytes) should fail — file_size <= 0 */
    if (st != EAI_ERR_FORMAT) { FAIL("expected FORMAT error for empty file"); remove(path); return; }

    remove(path);
    PASS();
}

/* ========== format_load() end-to-end ========== */

static void test_format_load_gguf(void)
{
    TEST(format_load_gguf);
    const char *path = create_gguf_with_int_kv();
    if (!path) { FAIL("create failed"); return; }

    gguf_context_t ctx;
    eai_format_t fmt = EAI_FORMAT_UNKNOWN;
    eai_status_t st = eai_format_load(path, &ctx, &fmt);
    if (st != EAI_OK) { FAIL("format_load failed"); return; }
    if (fmt != EAI_FORMAT_GGUF) { FAIL("format not detected as GGUF"); eai_gguf_free(&ctx); return; }
    if (ctx.magic != 0x46554747) { FAIL("magic wrong after format_load"); eai_gguf_free(&ctx); return; }

    eai_gguf_free(&ctx);
    PASS();
}

static void test_format_load_onnx(void)
{
    TEST(format_load_onnx);
    const char *path = create_onnx_multi_field();
    if (!path) { FAIL("create failed"); return; }

    onnx_context_t ctx;
    eai_format_t fmt = EAI_FORMAT_UNKNOWN;
    eai_status_t st = eai_format_load(path, &ctx, &fmt);
    if (st != EAI_OK) { FAIL("format_load failed"); eai_onnx_free(&ctx); return; }
    if (fmt != EAI_FORMAT_ONNX) { FAIL("format not detected as ONNX"); eai_onnx_free(&ctx); return; }
    if (ctx.ir_version != 8) { FAIL("ir_version wrong"); eai_onnx_free(&ctx); return; }

    eai_onnx_free(&ctx);
    PASS();
}

static void test_format_load_unknown(void)
{
    TEST(format_load_unknown);
    const char *path = "test_unknown_fmt.bin";
    FILE *fp = fopen(path, "wb");
    if (!fp) { FAIL("create failed"); return; }
    uint8_t data[] = {0xFF, 0xFE, 0xFD, 0xFC, 0xFB};
    fwrite(data, 1, sizeof(data), fp);
    fclose(fp);

    gguf_context_t ctx;
    eai_format_t fmt = EAI_FORMAT_UNKNOWN;
    eai_status_t st = eai_format_load(path, &ctx, &fmt);
    if (st != EAI_ERR_FORMAT) { FAIL("expected FORMAT error"); remove(path); return; }
    if (fmt != EAI_FORMAT_UNKNOWN) { FAIL("format should be UNKNOWN"); remove(path); return; }

    remove(path);
    PASS();
}

static void test_format_load_null_format_ptr(void)
{
    TEST(format_load_null_format_ptr);
    const char *path = create_gguf_with_int_kv();
    if (!path) { FAIL("create failed"); return; }

    gguf_context_t ctx;
    /* Pass NULL for out_format — should still work */
    eai_status_t st = eai_format_load(path, &ctx, NULL);
    if (st != EAI_OK) { FAIL("format_load with NULL format ptr failed"); return; }

    eai_gguf_free(&ctx);
    PASS();
}

/* ========== Cleanup ========== */

static void cleanup(void)
{
    remove("test_gguf_int_kv.gguf");
    remove("test_gguf_tensor.gguf");
    remove("test_gguf_multi_kv.gguf");
    remove("test_gguf_str_tmp.gguf");
    remove("test_onnx_multi.onnx");
    remove("test_onnx_empty.onnx");
    remove("test_unknown_fmt.bin");
}

int main(void)
{
    printf("=== EAI Extended Format Tests ===\n\n");

    printf("--- GGUF numeric KV ---\n");
    test_gguf_get_int_i32();
    test_gguf_get_int_u32();
    test_gguf_get_int_wrong_type();

    printf("\n--- GGUF tensors ---\n");
    test_gguf_load_with_tensor();
    test_gguf_get_tensor_by_name();
    test_gguf_get_tensor_data_values();

    printf("\n--- GGUF multi KV types ---\n");
    test_gguf_multi_kv_types();

    printf("\n--- ONNX extended ---\n");
    test_onnx_multi_field();
    test_onnx_empty_file();

    printf("\n--- format_load() ---\n");
    test_format_load_gguf();
    test_format_load_onnx();
    test_format_load_unknown();
    test_format_load_null_format_ptr();

    cleanup();

    printf("\n=== Results: %d/%d passed, %d failed ===\n",
           tests_passed, tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
