// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// Comprehensive tests for tensor library — edge cases, all dtypes, multi-dim

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "eai/accel.h"

static int tests_run = 0, tests_passed = 0, tests_failed = 0;
#define TEST(name) do { tests_run++; printf("  TEST %-44s ", #name); } while(0)
#define PASS() do { tests_passed++; printf("[PASS]\n"); } while(0)
#define FAIL(msg) do { tests_failed++; printf("[FAIL] %s\n", msg); } while(0)

/* ---- eai_tensor_create edge cases ---- */

static void test_tensor_create_null_tensor(void)
{
    TEST(tensor_create_null_tensor);
    int64_t shape[] = {2, 3};
    eai_status_t st = eai_tensor_create(NULL, EAI_DTYPE_F32, shape, 2);
    if (st != EAI_ERR_INVALID) { FAIL("expected EAI_ERR_INVALID for NULL tensor"); return; }
    PASS();
}

static void test_tensor_create_null_shape(void)
{
    TEST(tensor_create_null_shape);
    eai_tensor_t t;
    eai_status_t st = eai_tensor_create(&t, EAI_DTYPE_F32, NULL, 2);
    if (st != EAI_ERR_INVALID) { FAIL("expected EAI_ERR_INVALID for NULL shape"); return; }
    PASS();
}

static void test_tensor_create_zero_ndim(void)
{
    TEST(tensor_create_zero_ndim);
    eai_tensor_t t;
    int64_t shape[] = {2};
    eai_status_t st = eai_tensor_create(&t, EAI_DTYPE_F32, shape, 0);
    if (st != EAI_ERR_INVALID) { FAIL("expected EAI_ERR_INVALID for ndim=0"); return; }
    PASS();
}

static void test_tensor_create_negative_ndim(void)
{
    TEST(tensor_create_negative_ndim);
    eai_tensor_t t;
    int64_t shape[] = {2};
    eai_status_t st = eai_tensor_create(&t, EAI_DTYPE_F32, shape, -1);
    if (st != EAI_ERR_INVALID) { FAIL("expected EAI_ERR_INVALID for ndim=-1"); return; }
    PASS();
}

static void test_tensor_create_exceeds_max_dims(void)
{
    TEST(tensor_create_exceeds_max_dims);
    eai_tensor_t t;
    int64_t shape[EAI_TENSOR_MAX_DIMS + 1];
    for (int i = 0; i <= EAI_TENSOR_MAX_DIMS; i++) shape[i] = 2;
    eai_status_t st = eai_tensor_create(&t, EAI_DTYPE_F32, shape, EAI_TENSOR_MAX_DIMS + 1);
    if (st != EAI_ERR_INVALID) { FAIL("expected EAI_ERR_INVALID for ndim > MAX"); return; }
    PASS();
}

static void test_tensor_create_1d(void)
{
    TEST(tensor_create_1d);
    int64_t shape[] = {10};
    eai_tensor_t t;
    eai_status_t st = eai_tensor_create(&t, EAI_DTYPE_F32, shape, 1);
    if (st != EAI_OK) { FAIL("create failed"); return; }
    if (t.ndim != 1) { FAIL("ndim != 1"); eai_tensor_destroy(&t); return; }
    if (t.shape[0] != 10) { FAIL("shape[0] != 10"); eai_tensor_destroy(&t); return; }
    if (eai_tensor_numel(&t) != 10) { FAIL("numel != 10"); eai_tensor_destroy(&t); return; }
    if (t.strides[0] != 1) { FAIL("strides[0] != 1 for 1D"); eai_tensor_destroy(&t); return; }
    if (t.data_size != 10 * 4) { FAIL("data_size != 40"); eai_tensor_destroy(&t); return; }
    eai_tensor_destroy(&t);
    PASS();
}

static void test_tensor_create_3d(void)
{
    TEST(tensor_create_3d);
    int64_t shape[] = {2, 3, 4};
    eai_tensor_t t;
    eai_status_t st = eai_tensor_create(&t, EAI_DTYPE_F32, shape, 3);
    if (st != EAI_OK) { FAIL("create failed"); return; }
    if (t.ndim != 3) { FAIL("ndim != 3"); eai_tensor_destroy(&t); return; }
    if (eai_tensor_numel(&t) != 24) { FAIL("numel != 24"); eai_tensor_destroy(&t); return; }
    /* Check row-major strides: [12, 4, 1] */
    if (t.strides[2] != 1) { FAIL("strides[2] != 1"); eai_tensor_destroy(&t); return; }
    if (t.strides[1] != 4) { FAIL("strides[1] != 4"); eai_tensor_destroy(&t); return; }
    if (t.strides[0] != 12) { FAIL("strides[0] != 12"); eai_tensor_destroy(&t); return; }
    eai_tensor_destroy(&t);
    PASS();
}

static void test_tensor_create_max_dims(void)
{
    TEST(tensor_create_max_dims);
    int64_t shape[EAI_TENSOR_MAX_DIMS];
    for (int i = 0; i < EAI_TENSOR_MAX_DIMS; i++) shape[i] = 2;
    eai_tensor_t t;
    eai_status_t st = eai_tensor_create(&t, EAI_DTYPE_F32, shape, EAI_TENSOR_MAX_DIMS);
    if (st != EAI_OK) { FAIL("create failed"); return; }
    /* numel = 2^8 = 256 */
    if (eai_tensor_numel(&t) != 256) { FAIL("numel != 256"); eai_tensor_destroy(&t); return; }
    eai_tensor_destroy(&t);
    PASS();
}

static void test_tensor_data_zero_initialized(void)
{
    TEST(tensor_data_zero_initialized);
    int64_t shape[] = {4};
    eai_tensor_t t;
    eai_tensor_create(&t, EAI_DTYPE_F32, shape, 1);
    float *data = (float *)t.data;
    for (int i = 0; i < 4; i++) {
        if (data[i] != 0.0f) { FAIL("data not zero-initialized"); eai_tensor_destroy(&t); return; }
    }
    eai_tensor_destroy(&t);
    PASS();
}

static void test_tensor_owns_data_flag(void)
{
    TEST(tensor_owns_data_flag);
    int64_t shape[] = {5};
    eai_tensor_t t;
    eai_tensor_create(&t, EAI_DTYPE_F32, shape, 1);
    if (!t.owns_data) { FAIL("created tensor should own data"); eai_tensor_destroy(&t); return; }
    eai_tensor_destroy(&t);
    /* After destroy, tensor is zeroed */
    if (t.data != NULL) { FAIL("data should be NULL after destroy"); return; }
    if (t.owns_data) { FAIL("owns_data should be false after destroy"); return; }
    PASS();
}

static void test_tensor_destroy_null_safe(void)
{
    TEST(tensor_destroy_null_safe);
    eai_tensor_destroy(NULL); /* should not crash */
    eai_tensor_t t;
    memset(&t, 0, sizeof(t));
    eai_tensor_destroy(&t); /* should not crash on zero tensor */
    PASS();
}

static void test_tensor_destroy_double_call(void)
{
    TEST(tensor_destroy_double_call);
    int64_t shape[] = {3};
    eai_tensor_t t;
    eai_tensor_create(&t, EAI_DTYPE_F32, shape, 1);
    eai_tensor_destroy(&t);
    eai_tensor_destroy(&t); /* Second call should be safe since memset zeroes it */
    PASS();
}

/* ---- eai_tensor_create_view edge cases ---- */

static void test_tensor_view_null_params(void)
{
    TEST(tensor_view_null_params);
    eai_tensor_t src, view;
    int64_t shape[] = {4, 4};
    int64_t vs[] = {2, 2};
    eai_tensor_create(&src, EAI_DTYPE_F32, shape, 2);

    eai_status_t st = eai_tensor_create_view(NULL, &src, NULL, vs, 2);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for NULL view"); eai_tensor_destroy(&src); return; }

    st = eai_tensor_create_view(&view, NULL, NULL, vs, 2);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for NULL src"); eai_tensor_destroy(&src); return; }

    st = eai_tensor_create_view(&view, &src, NULL, NULL, 2);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for NULL shape"); eai_tensor_destroy(&src); return; }

    eai_tensor_destroy(&src);
    PASS();
}

static void test_tensor_view_shares_data(void)
{
    TEST(tensor_view_shares_data);
    int64_t shape[] = {4, 4};
    eai_tensor_t src, view;
    eai_tensor_create(&src, EAI_DTYPE_F32, shape, 2);

    /* Write known value at [0,0] */
    float *data = (float *)src.data;
    data[0] = 42.0f;

    int64_t view_shape[] = {2, 2};
    eai_tensor_create_view(&view, &src, NULL, view_shape, 2);

    /* View without offset should point at same memory start */
    float *vdata = (float *)view.data;
    if (vdata[0] != 42.0f) { FAIL("view data[0] != 42.0 — data not shared"); goto cleanup; }

    /* Mutate via view should reflect in src */
    vdata[0] = 99.0f;
    if (data[0] != 99.0f) { FAIL("mutation via view not reflected in src"); goto cleanup; }
    PASS();

cleanup:
    eai_tensor_destroy(&view);
    eai_tensor_destroy(&src);
}

static void test_tensor_view_with_offset(void)
{
    TEST(tensor_view_with_offset);
    int64_t shape[] = {4, 4};
    eai_tensor_t src, view;
    eai_tensor_create(&src, EAI_DTYPE_F32, shape, 2);

    /* Fill src with index values */
    float *data = (float *)src.data;
    for (int i = 0; i < 16; i++) data[i] = (float)i;

    int64_t offset[] = {1, 2};
    int64_t vs[] = {2, 2};
    eai_tensor_create_view(&view, &src, offset, vs, 2);

    /* offset[1,2] with stride [4,1] → byte offset = (1*4 + 2*1)*4 = 24 bytes = 6 floats */
    float *vdata = (float *)view.data;
    if (vdata[0] != 6.0f) { FAIL("view[0] should be src[1][2]=6.0"); goto cleanup; }
    PASS();

cleanup:
    eai_tensor_destroy(&view);
    eai_tensor_destroy(&src);
}

/* ---- eai_dtype_size completeness ---- */

static void test_dtype_size_all(void)
{
    TEST(dtype_size_all);
    if (eai_dtype_size(EAI_DTYPE_F32)  != 4) { FAIL("F32"); return; }
    if (eai_dtype_size(EAI_DTYPE_F16)  != 2) { FAIL("F16"); return; }
    if (eai_dtype_size(EAI_DTYPE_BF16) != 2) { FAIL("BF16"); return; }
    if (eai_dtype_size(EAI_DTYPE_INT8) != 1) { FAIL("INT8"); return; }
    if (eai_dtype_size(EAI_DTYPE_INT4) != 1) { FAIL("INT4"); return; }
    if (eai_dtype_size(EAI_DTYPE_Q4_0) != 1) { FAIL("Q4_0"); return; }
    if (eai_dtype_size(EAI_DTYPE_Q4_1) != 1) { FAIL("Q4_1"); return; }
    if (eai_dtype_size(EAI_DTYPE_Q8_0) != 1) { FAIL("Q8_0"); return; }
    if (eai_dtype_size(EAI_DTYPE_INT32) != 4) { FAIL("INT32"); return; }
    /* Unknown dtype should return 0 */
    if (eai_dtype_size((eai_dtype_t)999) != 0) { FAIL("unknown dtype != 0"); return; }
    PASS();
}

/* ---- eai_tensor_numel edge cases ---- */

static void test_tensor_numel_null(void)
{
    TEST(tensor_numel_null);
    int64_t n = eai_tensor_numel(NULL);
    if (n != 0) { FAIL("numel(NULL) should be 0"); return; }
    PASS();
}

static void test_tensor_numel_zero_ndim(void)
{
    TEST(tensor_numel_zero_ndim);
    eai_tensor_t t;
    memset(&t, 0, sizeof(t));
    t.ndim = 0;
    int64_t n = eai_tensor_numel(&t);
    if (n != 0) { FAIL("numel with ndim=0 should be 0"); return; }
    PASS();
}

static void test_tensor_numel_negative_ndim(void)
{
    TEST(tensor_numel_negative_ndim);
    eai_tensor_t t;
    memset(&t, 0, sizeof(t));
    t.ndim = -1;
    int64_t n = eai_tensor_numel(&t);
    if (n != 0) { FAIL("numel with ndim=-1 should be 0"); return; }
    PASS();
}

/* ---- INT8 dtype tensor ---- */

static void test_tensor_int8_dtype(void)
{
    TEST(tensor_int8_dtype);
    int64_t shape[] = {8};
    eai_tensor_t t;
    eai_status_t st = eai_tensor_create(&t, EAI_DTYPE_INT8, shape, 1);
    if (st != EAI_OK) { FAIL("create failed"); return; }
    if (t.data_size != 8) { FAIL("data_size != 8 for INT8[8]"); eai_tensor_destroy(&t); return; }
    /* Write and read back INT8 values */
    int8_t *d = (int8_t *)t.data;
    d[0] = -128;
    d[7] = 127;
    if (d[0] != -128) { FAIL("INT8 write/read failed at [0]"); eai_tensor_destroy(&t); return; }
    if (d[7] != 127) { FAIL("INT8 write/read failed at [7]"); eai_tensor_destroy(&t); return; }
    eai_tensor_destroy(&t);
    PASS();
}

int main(void)
{
    printf("=== EAI Tensor Tests (Comprehensive) ===\n\n");

    test_tensor_create_null_tensor();
    test_tensor_create_null_shape();
    test_tensor_create_zero_ndim();
    test_tensor_create_negative_ndim();
    test_tensor_create_exceeds_max_dims();
    test_tensor_create_1d();
    test_tensor_create_3d();
    test_tensor_create_max_dims();
    test_tensor_data_zero_initialized();
    test_tensor_owns_data_flag();
    test_tensor_destroy_null_safe();
    test_tensor_destroy_double_call();
    test_tensor_view_null_params();
    test_tensor_view_shares_data();
    test_tensor_view_with_offset();
    test_dtype_size_all();
    test_tensor_numel_null();
    test_tensor_numel_zero_ndim();
    test_tensor_numel_negative_ndim();
    test_tensor_int8_dtype();

    printf("\n=== Results: %d/%d passed, %d failed ===\n",
           tests_passed, tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
