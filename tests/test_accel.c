// SPDX-License-Identifier: MIT
// Tests for tensor library and accelerator backend

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "eai/accel.h"

static int tests_run = 0, tests_passed = 0, tests_failed = 0;
#define TEST(name) do { tests_run++; printf("  TEST %-40s ", #name); } while(0)
#define PASS() do { tests_passed++; printf("[PASS]\n"); } while(0)
#define FAIL(msg) do { tests_failed++; printf("[FAIL] %s\n", msg); } while(0)

static void test_tensor_create(void)
{
    TEST(tensor_create);
    int64_t shape[] = {2, 3};
    eai_tensor_t t;
    eai_status_t st = eai_tensor_create(&t, EAI_DTYPE_F32, shape, 2);
    if (st != EAI_OK) { FAIL("create failed"); return; }
    if (t.ndim != 2) { FAIL("wrong ndim"); eai_tensor_destroy(&t); return; }
    if (t.shape[0] != 2 || t.shape[1] != 3) { FAIL("wrong shape"); eai_tensor_destroy(&t); return; }
    if (eai_tensor_numel(&t) != 6) { FAIL("wrong numel"); eai_tensor_destroy(&t); return; }
    if (!t.data) { FAIL("data is NULL"); eai_tensor_destroy(&t); return; }
    eai_tensor_destroy(&t);
    PASS();
}

static void test_tensor_view(void)
{
    TEST(tensor_view);
    int64_t shape[] = {4, 4};
    eai_tensor_t src, view;
    eai_tensor_create(&src, EAI_DTYPE_F32, shape, 2);

    int64_t offset[] = {1, 1};
    int64_t view_shape[] = {2, 2};
    eai_status_t st = eai_tensor_create_view(&view, &src, offset, view_shape, 2);
    if (st != EAI_OK) { FAIL("view failed"); eai_tensor_destroy(&src); return; }
    if (view.owns_data) { FAIL("view should not own data"); eai_tensor_destroy(&src); return; }

    eai_tensor_destroy(&view);
    eai_tensor_destroy(&src);
    PASS();
}

static void test_dtype_size(void)
{
    TEST(dtype_size);
    if (eai_dtype_size(EAI_DTYPE_F32) != 4) { FAIL("F32 != 4"); return; }
    if (eai_dtype_size(EAI_DTYPE_F16) != 2) { FAIL("F16 != 2"); return; }
    if (eai_dtype_size(EAI_DTYPE_INT8) != 1) { FAIL("INT8 != 1"); return; }
    if (eai_dtype_size(EAI_DTYPE_INT32) != 4) { FAIL("INT32 != 4"); return; }
    PASS();
}

static void test_backend_register(void)
{
    TEST(backend_register);
    extern const eai_accel_backend_ops_t eai_accel_cpu_ops;
    eai_accel_reset(); /* Clean slate for this test */
    eai_status_t st = eai_accel_register(&eai_accel_cpu_ops);
    if (st != EAI_OK) { FAIL("register failed"); return; }

    const eai_accel_backend_ops_t *found = eai_accel_find("cpu");
    if (!found) { FAIL("find returned NULL"); return; }
    if (strcmp(found->name, "cpu") != 0) { FAIL("wrong name"); return; }
    PASS();
}

static void test_cpu_matmul(void)
{
    TEST(cpu_matmul);

    /* 2x2 matmul: [1,2;3,4] * [5,6;7,8] = [19,22;43,50] */
    int64_t shape[] = {2, 2};
    eai_tensor_t a, b, c;
    eai_tensor_create(&a, EAI_DTYPE_F32, shape, 2);
    eai_tensor_create(&b, EAI_DTYPE_F32, shape, 2);
    eai_tensor_create(&c, EAI_DTYPE_F32, shape, 2);

    float *fa = (float *)a.data;
    fa[0] = 1; fa[1] = 2; fa[2] = 3; fa[3] = 4;
    float *fb = (float *)b.data;
    fb[0] = 5; fb[1] = 6; fb[2] = 7; fb[3] = 8;

    int in_idx[] = {0, 1};
    int out_idx[] = {2};
    eai_op_t op = {
        .type = EAI_OP_MATMUL, .input_count = 2, .output_count = 1,
        .input_indices = in_idx, .output_indices = out_idx,
    };

    eai_tensor_t tensors[] = {a, b, c};
    eai_compute_graph_t graph = {
        .ops = &op, .op_count = 1, .tensors = tensors, .tensor_count = 3,
    };

    eai_status_t st = eai_accel_dispatch(&graph);
    if (st != EAI_OK) { FAIL("dispatch failed"); goto cleanup; }

    float *fc = (float *)tensors[2].data;
    if (fabsf(fc[0] - 19.0f) > 0.01f) { FAIL("c[0,0] != 19"); goto cleanup; }
    if (fabsf(fc[1] - 22.0f) > 0.01f) { FAIL("c[0,1] != 22"); goto cleanup; }
    if (fabsf(fc[2] - 43.0f) > 0.01f) { FAIL("c[1,0] != 43"); goto cleanup; }
    if (fabsf(fc[3] - 50.0f) > 0.01f) { FAIL("c[1,1] != 50"); goto cleanup; }
    PASS();

cleanup:
    eai_tensor_destroy(&a);
    eai_tensor_destroy(&b);
    eai_tensor_destroy(&c);
}

static void test_cpu_softmax(void)
{
    TEST(cpu_softmax);

    int64_t shape[] = {4};
    eai_tensor_t in, out;
    eai_tensor_create(&in, EAI_DTYPE_F32, shape, 1);
    eai_tensor_create(&out, EAI_DTYPE_F32, shape, 1);

    float *fi = (float *)in.data;
    fi[0] = 1.0f; fi[1] = 2.0f; fi[2] = 3.0f; fi[3] = 4.0f;

    int in_idx[] = {0};
    int out_idx[] = {1};
    eai_op_t op = {
        .type = EAI_OP_SOFTMAX, .input_count = 1, .output_count = 1,
        .input_indices = in_idx, .output_indices = out_idx,
    };

    eai_tensor_t tensors[] = {in, out};
    eai_compute_graph_t graph = {
        .ops = &op, .op_count = 1, .tensors = tensors, .tensor_count = 2,
    };

    eai_status_t st = eai_accel_dispatch(&graph);
    if (st != EAI_OK) { FAIL("dispatch failed"); goto cleanup; }

    float *fo = (float *)tensors[1].data;
    /* Sum should be ~1.0 */
    float sum = fo[0] + fo[1] + fo[2] + fo[3];
    if (fabsf(sum - 1.0f) > 0.01f) { FAIL("softmax sum != 1.0"); goto cleanup; }
    /* Values should be monotonically increasing */
    if (fo[3] <= fo[0]) { FAIL("softmax not monotonic"); goto cleanup; }
    PASS();

cleanup:
    eai_tensor_destroy(&in);
    eai_tensor_destroy(&out);
}

int main(void)
{
    printf("=== EAI Accelerator Tests ===\n\n");

    test_tensor_create();
    test_tensor_view();
    test_dtype_size();
    test_backend_register();
    test_cpu_matmul();
    test_cpu_softmax();

    printf("\n=== Results: %d/%d passed, %d failed ===\n",
           tests_passed, tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
