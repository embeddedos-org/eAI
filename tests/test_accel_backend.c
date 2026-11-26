// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// Comprehensive tests for accelerator backend registry, dispatch, and CPU kernels

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "eai/accel.h"

static int tests_run = 0, tests_passed = 0, tests_failed = 0;
#define TEST(name) do { tests_run++; printf("  TEST %-44s ", #name); } while(0)
#define PASS() do { tests_passed++; printf("[PASS]\n"); } while(0)
#define FAIL(msg) do { tests_failed++; printf("[FAIL] %s\n", msg); } while(0)

extern const eai_accel_backend_ops_t eai_accel_cpu_ops;

/* Ensure CPU backend is registered exactly once */
static int backend_registered = 0;
static void ensure_cpu_registered(void)
{
    if (!backend_registered) {
        eai_accel_reset(); /* Clean slate */
        eai_accel_register(&eai_accel_cpu_ops);
        backend_registered = 1;
    }
}

/* ---- Registry tests ---- */

static void test_register_null_ops(void)
{
    TEST(register_null_ops);
    eai_status_t st = eai_accel_register(NULL);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for NULL ops"); return; }
    PASS();
}

static void test_register_null_name(void)
{
    TEST(register_null_name);
    eai_accel_backend_ops_t bad_ops;
    memset(&bad_ops, 0, sizeof(bad_ops));
    bad_ops.name = NULL;
    eai_status_t st = eai_accel_register(&bad_ops);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for NULL name"); return; }
    PASS();
}

static void test_register_duplicate(void)
{
    TEST(register_duplicate);
    ensure_cpu_registered();
    /* Registering "cpu" again should fail */
    eai_status_t st = eai_accel_register(&eai_accel_cpu_ops);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for duplicate"); return; }
    PASS();
}

static void test_find_null_name(void)
{
    TEST(find_null_name);
    const eai_accel_backend_ops_t *found = eai_accel_find(NULL);
    if (found != NULL) { FAIL("expected NULL for NULL name"); return; }
    PASS();
}

static void test_find_nonexistent(void)
{
    TEST(find_nonexistent);
    const eai_accel_backend_ops_t *found = eai_accel_find("nonexistent_backend_xyz");
    if (found != NULL) { FAIL("expected NULL for nonexistent backend"); return; }
    PASS();
}

static void test_find_by_type_cpu(void)
{
    TEST(find_by_type_cpu);
    ensure_cpu_registered();
    const eai_accel_backend_ops_t *found = eai_accel_find_by_type(EAI_BACKEND_CPU);
    if (!found) { FAIL("expected to find CPU backend"); return; }
    if (found->type != EAI_BACKEND_CPU) { FAIL("type mismatch"); return; }
    if (strcmp(found->name, "cpu") != 0) { FAIL("name mismatch"); return; }
    PASS();
}

static void test_find_by_type_nonexistent(void)
{
    TEST(find_by_type_nonexistent);
    const eai_accel_backend_ops_t *found = eai_accel_find_by_type(EAI_BACKEND_CUDA);
    if (found != NULL) { FAIL("expected NULL for unregistered type"); return; }
    PASS();
}

static void test_accel_list(void)
{
    TEST(accel_list);
    ensure_cpu_registered();
    const eai_accel_backend_ops_t *out[EAI_MAX_ACCEL_BACKENDS];
    int count = eai_accel_list(out, EAI_MAX_ACCEL_BACKENDS);
    if (count < 1) { FAIL("list returned 0 backends"); return; }

    /* Verify cpu is in the list */
    int found_cpu = 0;
    for (int i = 0; i < count; i++) {
        if (strcmp(out[i]->name, "cpu") == 0) { found_cpu = 1; break; }
    }
    if (!found_cpu) { FAIL("cpu not found in list"); return; }
    PASS();
}

static void test_accel_list_limited(void)
{
    TEST(accel_list_limited);
    ensure_cpu_registered();
    const eai_accel_backend_ops_t *out[1];
    int count = eai_accel_list(out, 1);
    if (count != 1) { FAIL("expected exactly 1"); return; }
    PASS();
}

/* ---- CPU backend info ---- */

static void test_cpu_backend_info(void)
{
    TEST(cpu_backend_info);
    ensure_cpu_registered();
    eai_accel_backend_t backend;
    memset(&backend, 0, sizeof(backend));
    backend.ops = &eai_accel_cpu_ops;

    eai_accel_backend_info_t info;
    memset(&info, 0, sizeof(info));
    eai_status_t st = eai_accel_cpu_ops.get_info(&backend, &info);
    if (st != EAI_OK) { FAIL("get_info failed"); return; }
    if (strlen(info.name) == 0) { FAIL("info name is empty"); return; }
    if (info.type != EAI_BACKEND_CPU) { FAIL("info type not CPU"); return; }
    printf("(%s) ", info.name);
    PASS();
}

static void test_cpu_backend_info_null(void)
{
    TEST(cpu_backend_info_null);
    eai_accel_backend_t backend;
    memset(&backend, 0, sizeof(backend));
    eai_status_t st = eai_accel_cpu_ops.get_info(&backend, NULL);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for NULL info"); return; }
    PASS();
}

static void test_cpu_can_handle_null_op(void)
{
    TEST(cpu_can_handle_null_op);
    eai_accel_backend_t backend;
    memset(&backend, 0, sizeof(backend));
    bool can = eai_accel_cpu_ops.can_handle(&backend, NULL);
    if (can) { FAIL("should not handle NULL op"); return; }
    PASS();
}

static void test_cpu_can_handle_valid_op(void)
{
    TEST(cpu_can_handle_valid_op);
    eai_accel_backend_t backend;
    memset(&backend, 0, sizeof(backend));
    eai_op_t op = { .type = EAI_OP_RELU };
    bool can = eai_accel_cpu_ops.can_handle(&backend, &op);
    if (!can) { FAIL("CPU should handle all valid ops"); return; }
    PASS();
}

/* ---- Dispatch edge cases ---- */

static void test_dispatch_null_graph(void)
{
    TEST(dispatch_null_graph);
    ensure_cpu_registered();
    eai_status_t st = eai_accel_dispatch(NULL);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for NULL graph"); return; }
    PASS();
}

static void test_dispatch_null_ops_in_graph(void)
{
    TEST(dispatch_null_ops_in_graph);
    ensure_cpu_registered();
    eai_compute_graph_t graph;
    memset(&graph, 0, sizeof(graph));
    graph.ops = NULL;
    graph.op_count = 1;
    eai_status_t st = eai_accel_dispatch(&graph);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for NULL ops"); return; }
    PASS();
}

static void test_dispatch_zero_op_count(void)
{
    TEST(dispatch_zero_op_count);
    ensure_cpu_registered();
    eai_op_t op = { .type = EAI_OP_RELU };
    eai_compute_graph_t graph = { .ops = &op, .op_count = 0 };
    eai_status_t st = eai_accel_dispatch(&graph);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for op_count=0"); return; }
    PASS();
}

/* ---- CPU kernel: ReLU ---- */

static void test_cpu_relu(void)
{
    TEST(cpu_relu);
    ensure_cpu_registered();

    int64_t shape[] = {6};
    eai_tensor_t in, out;
    eai_tensor_create(&in, EAI_DTYPE_F32, shape, 1);
    eai_tensor_create(&out, EAI_DTYPE_F32, shape, 1);

    float *fi = (float *)in.data;
    fi[0] = -3.0f; fi[1] = -0.5f; fi[2] = 0.0f;
    fi[3] = 0.5f;  fi[4] = 3.0f;  fi[5] = -100.0f;

    int in_idx[] = {0};
    int out_idx[] = {1};
    eai_op_t op = {
        .type = EAI_OP_RELU, .input_count = 1, .output_count = 1,
        .input_indices = in_idx, .output_indices = out_idx,
    };
    eai_tensor_t tensors[] = {in, out};
    eai_compute_graph_t graph = { .ops = &op, .op_count = 1, .tensors = tensors, .tensor_count = 2 };

    eai_status_t st = eai_accel_dispatch(&graph);
    if (st != EAI_OK) { FAIL("dispatch failed"); goto cleanup; }

    float *fo = (float *)tensors[1].data;
    if (fo[0] != 0.0f)  { FAIL("relu(-3) should be 0"); goto cleanup; }
    if (fo[1] != 0.0f)  { FAIL("relu(-0.5) should be 0"); goto cleanup; }
    if (fo[2] != 0.0f)  { FAIL("relu(0) should be 0"); goto cleanup; }
    if (fabsf(fo[3] - 0.5f) > 1e-6f) { FAIL("relu(0.5) should be 0.5"); goto cleanup; }
    if (fabsf(fo[4] - 3.0f) > 1e-6f) { FAIL("relu(3) should be 3"); goto cleanup; }
    if (fo[5] != 0.0f)  { FAIL("relu(-100) should be 0"); goto cleanup; }
    PASS();

cleanup:
    eai_tensor_destroy(&in);
    eai_tensor_destroy(&out);
}

/* ---- CPU kernel: Add ---- */

static void test_cpu_add(void)
{
    TEST(cpu_add);
    ensure_cpu_registered();

    int64_t shape[] = {4};
    eai_tensor_t a, b, c;
    eai_tensor_create(&a, EAI_DTYPE_F32, shape, 1);
    eai_tensor_create(&b, EAI_DTYPE_F32, shape, 1);
    eai_tensor_create(&c, EAI_DTYPE_F32, shape, 1);

    float *fa = (float *)a.data;
    float *fb = (float *)b.data;
    fa[0] = 1.0f; fa[1] = -2.0f; fa[2] = 0.0f; fa[3] = 100.5f;
    fb[0] = 3.0f; fb[1] = 2.0f;  fb[2] = 0.0f; fb[3] = -0.5f;

    int in_idx[] = {0, 1};
    int out_idx[] = {2};
    eai_op_t op = {
        .type = EAI_OP_ADD, .input_count = 2, .output_count = 1,
        .input_indices = in_idx, .output_indices = out_idx,
    };
    eai_tensor_t tensors[] = {a, b, c};
    eai_compute_graph_t graph = { .ops = &op, .op_count = 1, .tensors = tensors, .tensor_count = 3 };

    eai_status_t st = eai_accel_dispatch(&graph);
    if (st != EAI_OK) { FAIL("dispatch failed"); goto cleanup; }

    float *fc = (float *)tensors[2].data;
    if (fabsf(fc[0] - 4.0f) > 1e-6f) { FAIL("1+3 != 4"); goto cleanup; }
    if (fabsf(fc[1] - 0.0f) > 1e-6f) { FAIL("-2+2 != 0"); goto cleanup; }
    if (fabsf(fc[2] - 0.0f) > 1e-6f) { FAIL("0+0 != 0"); goto cleanup; }
    if (fabsf(fc[3] - 100.0f) > 1e-6f) { FAIL("100.5+(-0.5) != 100"); goto cleanup; }
    PASS();

cleanup:
    eai_tensor_destroy(&a);
    eai_tensor_destroy(&b);
    eai_tensor_destroy(&c);
}

/* ---- CPU kernel: Mul ---- */

static void test_cpu_mul(void)
{
    TEST(cpu_mul);
    ensure_cpu_registered();

    int64_t shape[] = {4};
    eai_tensor_t a, b, c;
    eai_tensor_create(&a, EAI_DTYPE_F32, shape, 1);
    eai_tensor_create(&b, EAI_DTYPE_F32, shape, 1);
    eai_tensor_create(&c, EAI_DTYPE_F32, shape, 1);

    float *fa = (float *)a.data;
    float *fb = (float *)b.data;
    fa[0] = 2.0f; fa[1] = -3.0f; fa[2] = 0.0f; fa[3] = 0.5f;
    fb[0] = 3.0f; fb[1] = -2.0f; fb[2] = 99.0f; fb[3] = 4.0f;

    int in_idx[] = {0, 1};
    int out_idx[] = {2};
    eai_op_t op = {
        .type = EAI_OP_MUL, .input_count = 2, .output_count = 1,
        .input_indices = in_idx, .output_indices = out_idx,
    };
    eai_tensor_t tensors[] = {a, b, c};
    eai_compute_graph_t graph = { .ops = &op, .op_count = 1, .tensors = tensors, .tensor_count = 3 };

    eai_status_t st = eai_accel_dispatch(&graph);
    if (st != EAI_OK) { FAIL("dispatch failed"); goto cleanup; }

    float *fc = (float *)tensors[2].data;
    if (fabsf(fc[0] - 6.0f) > 1e-6f) { FAIL("2*3 != 6"); goto cleanup; }
    if (fabsf(fc[1] - 6.0f) > 1e-6f) { FAIL("-3*-2 != 6"); goto cleanup; }
    if (fabsf(fc[2] - 0.0f) > 1e-6f) { FAIL("0*99 != 0"); goto cleanup; }
    if (fabsf(fc[3] - 2.0f) > 1e-6f) { FAIL("0.5*4 != 2"); goto cleanup; }
    PASS();

cleanup:
    eai_tensor_destroy(&a);
    eai_tensor_destroy(&b);
    eai_tensor_destroy(&c);
}

/* ---- CPU kernel: LayerNorm ---- */

static void test_cpu_layernorm(void)
{
    TEST(cpu_layernorm);
    ensure_cpu_registered();

    int64_t shape[] = {4};
    eai_tensor_t in, out;
    eai_tensor_create(&in, EAI_DTYPE_F32, shape, 1);
    eai_tensor_create(&out, EAI_DTYPE_F32, shape, 1);

    /* Set all values equal — normalized should be ~0 */
    float *fi = (float *)in.data;
    fi[0] = 5.0f; fi[1] = 5.0f; fi[2] = 5.0f; fi[3] = 5.0f;

    int in_idx[] = {0};
    int out_idx[] = {1};
    eai_op_t op = {
        .type = EAI_OP_LAYERNORM, .input_count = 1, .output_count = 1,
        .input_indices = in_idx, .output_indices = out_idx,
    };
    eai_tensor_t tensors[] = {in, out};
    eai_compute_graph_t graph = { .ops = &op, .op_count = 1, .tensors = tensors, .tensor_count = 2 };

    eai_status_t st = eai_accel_dispatch(&graph);
    if (st != EAI_OK) { FAIL("dispatch failed"); goto cleanup; }

    float *fo = (float *)tensors[1].data;
    /* Constant input → normalized values should be near 0 */
    for (int i = 0; i < 4; i++) {
        if (fabsf(fo[i]) > 0.1f) { FAIL("layernorm of constant != ~0"); goto cleanup; }
    }
    PASS();

cleanup:
    eai_tensor_destroy(&in);
    eai_tensor_destroy(&out);
}

/* ---- CPU kernel: Softmax numerical stability ---- */

static void test_cpu_softmax_large_values(void)
{
    TEST(cpu_softmax_large_values);
    ensure_cpu_registered();

    int64_t shape[] = {3};
    eai_tensor_t in, out;
    eai_tensor_create(&in, EAI_DTYPE_F32, shape, 1);
    eai_tensor_create(&out, EAI_DTYPE_F32, shape, 1);

    /* Large values should not overflow due to max-subtraction trick */
    float *fi = (float *)in.data;
    fi[0] = 1000.0f; fi[1] = 1001.0f; fi[2] = 1002.0f;

    int in_idx[] = {0};
    int out_idx[] = {1};
    eai_op_t op = {
        .type = EAI_OP_SOFTMAX, .input_count = 1, .output_count = 1,
        .input_indices = in_idx, .output_indices = out_idx,
    };
    eai_tensor_t tensors[] = {in, out};
    eai_compute_graph_t graph = { .ops = &op, .op_count = 1, .tensors = tensors, .tensor_count = 2 };

    eai_status_t st = eai_accel_dispatch(&graph);
    if (st != EAI_OK) { FAIL("dispatch failed"); goto cleanup; }

    float *fo = (float *)tensors[1].data;
    float sum = fo[0] + fo[1] + fo[2];
    if (fabsf(sum - 1.0f) > 0.01f) { FAIL("softmax sum != 1.0 for large values"); goto cleanup; }
    /* Check no NaN/Inf */
    for (int i = 0; i < 3; i++) {
        if (fo[i] != fo[i]) { FAIL("NaN in output"); goto cleanup; }  /* NaN check */
        if (fo[i] < 0.0f || fo[i] > 1.0f) { FAIL("output out of [0,1]"); goto cleanup; }
    }
    /* Largest input should have largest probability */
    if (fo[2] <= fo[0]) { FAIL("softmax ordering wrong"); goto cleanup; }
    PASS();

cleanup:
    eai_tensor_destroy(&in);
    eai_tensor_destroy(&out);
}

/* ---- CPU kernel: Identity matmul ---- */

static void test_cpu_matmul_identity(void)
{
    TEST(cpu_matmul_identity);
    ensure_cpu_registered();

    int64_t shape[] = {3, 3};
    eai_tensor_t a, eye, c;
    eai_tensor_create(&a, EAI_DTYPE_F32, shape, 2);
    eai_tensor_create(&eye, EAI_DTYPE_F32, shape, 2);
    eai_tensor_create(&c, EAI_DTYPE_F32, shape, 2);

    /* A = [1..9], eye = identity */
    float *fa = (float *)a.data;
    for (int i = 0; i < 9; i++) fa[i] = (float)(i + 1);
    float *fe = (float *)eye.data;
    memset(fe, 0, 9 * sizeof(float));
    fe[0] = 1.0f; fe[4] = 1.0f; fe[8] = 1.0f;

    int in_idx[] = {0, 1};
    int out_idx[] = {2};
    eai_op_t op = {
        .type = EAI_OP_MATMUL, .input_count = 2, .output_count = 1,
        .input_indices = in_idx, .output_indices = out_idx,
    };
    eai_tensor_t tensors[] = {a, eye, c};
    eai_compute_graph_t graph = { .ops = &op, .op_count = 1, .tensors = tensors, .tensor_count = 3 };

    eai_status_t st = eai_accel_dispatch(&graph);
    if (st != EAI_OK) { FAIL("dispatch failed"); goto cleanup; }

    /* A * I = A */
    float *fc = (float *)tensors[2].data;
    for (int i = 0; i < 9; i++) {
        if (fabsf(fc[i] - fa[i]) > 1e-5f) {
            char msg[64];
            snprintf(msg, sizeof(msg), "A*I[%d]: expected %f got %f", i, fa[i], fc[i]);
            FAIL(msg);
            goto cleanup;
        }
    }
    PASS();

cleanup:
    eai_tensor_destroy(&a);
    eai_tensor_destroy(&eye);
    eai_tensor_destroy(&c);
}

/* ---- compute_graph_destroy safety ---- */

static void test_graph_destroy_null(void)
{
    TEST(graph_destroy_null);
    eai_compute_graph_destroy(NULL); /* should not crash */
    PASS();
}

static void test_graph_destroy_empty(void)
{
    TEST(graph_destroy_empty);
    eai_compute_graph_t graph;
    memset(&graph, 0, sizeof(graph));
    eai_compute_graph_destroy(&graph); /* should not crash */
    if (graph.ops != NULL) { FAIL("ops should be NULL after destroy"); return; }
    if (graph.tensors != NULL) { FAIL("tensors should be NULL after destroy"); return; }
    PASS();
}

/* ---- What the CPU backend does not implement, it refuses ---- */

static void test_cpu_can_handle_only_implemented_ops(void)
{
    TEST(cpu_can_handle_only_implemented_ops);
    eai_accel_backend_t backend;
    memset(&backend, 0, sizeof(backend));
    eai_op_t conv = { .type = EAI_OP_CONV2D };
    if (eai_accel_cpu_ops.can_handle(&backend, &conv)) {
        FAIL("CPU backend has no CONV2D kernel"); return;
    }
    PASS();
}

/* An op with no kernel used to be logged and skipped, and the graph reported
 * EAI_OK with its output never written. */
static void test_cpu_refuses_unimplemented_op(void)
{
    TEST(cpu_refuses_unimplemented_op);
    ensure_cpu_registered();

    int64_t shape[] = {4};
    eai_tensor_t in, out;
    eai_tensor_create(&in, EAI_DTYPE_F32, shape, 1);
    eai_tensor_create(&out, EAI_DTYPE_F32, shape, 1);
    for (int i = 0; i < 4; i++) {
        ((float *)in.data)[i] = 1.0f;
        ((float *)out.data)[i] = -7.0f;
    }

    int in_idx[] = {0};
    int out_idx[] = {1};
    eai_op_t op = {
        .type = EAI_OP_CONV2D, .input_count = 1, .output_count = 1,
        .input_indices = in_idx, .output_indices = out_idx,
    };
    eai_tensor_t tensors[] = {in, out};
    eai_compute_graph_t graph = { .ops = &op, .op_count = 1, .tensors = tensors, .tensor_count = 2 };

    eai_status_t st = eai_accel_dispatch(&graph);
    if (st != EAI_ERR_UNSUPPORTED) { FAIL("expected UNSUPPORTED for CONV2D"); goto cleanup; }
    for (int i = 0; i < 4; i++) {
        if (((float *)tensors[1].data)[i] != -7.0f) { FAIL("output was written"); goto cleanup; }
    }
    PASS();
cleanup:
    eai_tensor_destroy(&tensors[0]);
    eai_tensor_destroy(&tensors[1]);
}

/* Every kernel reads its tensors as float32. A 2x2 INT8 tensor holds 4
 * bytes; the float kernel read and wrote 16, past the end of each buffer. */
static void test_cpu_refuses_non_f32_tensor(void)
{
    TEST(cpu_refuses_non_f32_tensor);
    ensure_cpu_registered();

    int64_t shape[] = {2, 2};
    eai_tensor_t a, b, c;
    eai_tensor_create(&a, EAI_DTYPE_INT8, shape, 2);
    eai_tensor_create(&b, EAI_DTYPE_INT8, shape, 2);
    eai_tensor_create(&c, EAI_DTYPE_INT8, shape, 2);
    memset(a.data, 1, 4);
    memset(b.data, 1, 4);
    memset(c.data, 0x5A, 4);

    int in_idx[] = {0, 1};
    int out_idx[] = {2};
    eai_op_t op = {
        .type = EAI_OP_MATMUL, .input_count = 2, .output_count = 1,
        .input_indices = in_idx, .output_indices = out_idx,
    };
    eai_tensor_t tensors[] = {a, b, c};
    eai_compute_graph_t graph = { .ops = &op, .op_count = 1, .tensors = tensors, .tensor_count = 3 };

    eai_status_t st = eai_accel_dispatch(&graph);
    if (st != EAI_ERR_UNSUPPORTED) { FAIL("expected UNSUPPORTED for INT8 tensors"); goto cleanup; }
    for (int i = 0; i < 4; i++) {
        if (((uint8_t *)tensors[2].data)[i] != 0x5A) { FAIL("output was written"); goto cleanup; }
    }
    PASS();
cleanup:
    eai_tensor_destroy(&tensors[0]);
    eai_tensor_destroy(&tensors[1]);
    eai_tensor_destroy(&tensors[2]);
}

static void test_cpu_refuses_tensor_index_out_of_range(void)
{
    TEST(cpu_refuses_tensor_index_out_of_range);
    ensure_cpu_registered();

    int64_t shape[] = {4};
    eai_tensor_t in, out;
    eai_tensor_create(&in, EAI_DTYPE_F32, shape, 1);
    eai_tensor_create(&out, EAI_DTYPE_F32, shape, 1);

    int in_idx[] = {5};
    int out_idx[] = {1};
    eai_op_t op = {
        .type = EAI_OP_RELU, .input_count = 1, .output_count = 1,
        .input_indices = in_idx, .output_indices = out_idx,
    };
    /* On the heap, so that reading tensors[5] or tensors[-1] is an error a
     * sanitizer build reports rather than a read of whatever is on the stack. */
    eai_tensor_t *tensors = malloc(2 * sizeof(*tensors));
    if (!tensors) { FAIL("malloc"); return; }
    tensors[0] = in;
    tensors[1] = out;
    eai_compute_graph_t graph = { .ops = &op, .op_count = 1, .tensors = tensors, .tensor_count = 2 };

    eai_status_t st = eai_accel_dispatch(&graph);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for tensor index 5 of 2"); goto cleanup; }

    in_idx[0] = -1;
    st = eai_accel_dispatch(&graph);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for tensor index -1"); goto cleanup; }
    PASS();
cleanup:
    eai_tensor_destroy(&tensors[0]);
    eai_tensor_destroy(&tensors[1]);
    free(tensors);
}

/* A tensor that was never given a buffer (or was destroyed) is refused, not
 * dereferenced. */
static void test_cpu_refuses_tensor_without_data(void)
{
    TEST(cpu_refuses_tensor_without_data);
    ensure_cpu_registered();

    int64_t shape[] = {4};
    eai_tensor_t in, out;
    eai_tensor_create(&in, EAI_DTYPE_F32, shape, 1);
    memset(&out, 0, sizeof(out));
    out.dtype = EAI_DTYPE_F32;
    out.ndim = 1;
    out.shape[0] = 4;

    int in_idx[] = {0};
    int out_idx[] = {1};
    eai_op_t op = {
        .type = EAI_OP_RELU, .input_count = 1, .output_count = 1,
        .input_indices = in_idx, .output_indices = out_idx,
    };
    eai_tensor_t tensors[] = {in, out};
    eai_compute_graph_t graph = { .ops = &op, .op_count = 1, .tensors = tensors, .tensor_count = 2 };

    eai_status_t st = eai_accel_dispatch(&graph);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for a tensor with no data"); goto cleanup; }
    PASS();
cleanup:
    eai_tensor_destroy(&tensors[0]);
}

/* The graph is checked as a whole before any kernel runs: a graph whose
 * second op cannot be executed does not leave its first op's output behind. */
static void test_cpu_runs_no_op_of_a_graph_it_refuses(void)
{
    TEST(cpu_runs_no_op_of_a_graph_it_refuses);
    ensure_cpu_registered();

    int64_t shape[] = {4};
    eai_tensor_t in, mid, out;
    eai_tensor_create(&in, EAI_DTYPE_F32, shape, 1);
    eai_tensor_create(&mid, EAI_DTYPE_F32, shape, 1);
    eai_tensor_create(&out, EAI_DTYPE_F32, shape, 1);
    for (int i = 0; i < 4; i++) {
        ((float *)in.data)[i] = 2.0f;
        ((float *)mid.data)[i] = -7.0f;
    }

    int relu_in[] = {0};
    int relu_out[] = {1};
    int conv_in[] = {1};
    int conv_out[] = {2};
    eai_op_t ops[] = {
        { .type = EAI_OP_RELU, .input_count = 1, .output_count = 1,
          .input_indices = relu_in, .output_indices = relu_out },
        { .type = EAI_OP_CONV2D, .input_count = 1, .output_count = 1,
          .input_indices = conv_in, .output_indices = conv_out },
    };
    eai_tensor_t tensors[] = {in, mid, out};
    eai_compute_graph_t graph = { .ops = ops, .op_count = 2, .tensors = tensors, .tensor_count = 3 };

    eai_status_t st = eai_accel_dispatch(&graph);
    if (st != EAI_ERR_UNSUPPORTED) { FAIL("expected UNSUPPORTED"); goto cleanup; }
    for (int i = 0; i < 4; i++) {
        if (((float *)tensors[1].data)[i] != -7.0f) { FAIL("RELU ran before the refusal"); goto cleanup; }
    }
    PASS();
cleanup:
    eai_tensor_destroy(&tensors[0]);
    eai_tensor_destroy(&tensors[1]);
    eai_tensor_destroy(&tensors[2]);
}

/* ---- The CPU kernels do not trust tensor shapes (#94) ----
 *
 * cpu_execute() took an element count or a dimension from one operand and
 * applied it to the others. Each test below is one row of the table in #94;
 * every one of them wrote or read past a heap buffer before the fix, which
 * `-fsanitize=address` reports as a heap-buffer-overflow in the kernel.
 *
 * These assert on the returned status, so they fail on the unfixed backend
 * without needing a sanitizer: it returned EAI_OK for every one of them.
 */

/* Build a float32 tensor whose buffer is deliberately larger than the shape
 * it reports, with a known pattern past the end. data_size stays honest, so
 * the backend sees the real size while an overrun lands somewhere observable
 * instead of in the allocator's metadata. */
#define CANARY_BYTE 0xC7

static eai_status_t dispatch_binary(eai_op_type_t type, eai_tensor_t *tensors,
                                    int tensor_count, int a, int b, int out)
{
    int in_idx[2] = {a, b};
    int out_idx[1] = {out};
    eai_op_t op = {
        .type = type, .input_count = 2, .output_count = 1,
        .input_indices = in_idx, .output_indices = out_idx,
    };
    eai_compute_graph_t graph = {
        .ops = &op, .op_count = 1,
        .tensors = tensors, .tensor_count = tensor_count,
    };
    return eai_accel_dispatch(&graph);
}

static eai_status_t dispatch_unary(eai_op_type_t type, eai_tensor_t *tensors,
                                   int tensor_count, int in, int out)
{
    int in_idx[1] = {in};
    int out_idx[1] = {out};
    eai_op_t op = {
        .type = type, .input_count = 1, .output_count = 1,
        .input_indices = in_idx, .output_indices = out_idx,
    };
    eai_compute_graph_t graph = {
        .ops = &op, .op_count = 1,
        .tensors = tensors, .tensor_count = tensor_count,
    };
    return eai_accel_dispatch(&graph);
}

/* ADD over a[8], b[4], c[4]: the loop ran numel(a) = 8 times over buffers
 * holding 4 floats. The headline reproduction in #94. */
static void test_cpu_refuses_elementwise_shape_mismatch(void)
{
    TEST(cpu_refuses_elementwise_shape_mismatch);
    ensure_cpu_registered();

    int64_t s8[] = {8}, s4[] = {4};
    eai_tensor_t a, b, c;
    eai_tensor_create(&a, EAI_DTYPE_F32, s8, 1);
    eai_tensor_create(&b, EAI_DTYPE_F32, s4, 1);
    eai_tensor_create(&c, EAI_DTYPE_F32, s4, 1);
    eai_tensor_t tensors[] = {a, b, c};

    eai_status_t st = dispatch_binary(EAI_OP_ADD, tensors, 3, 0, 1, 2);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for ADD a[8]+b[4]"); goto cleanup; }

    /* MUL walks the same loop. */
    st = dispatch_binary(EAI_OP_MUL, tensors, 3, 0, 1, 2);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for MUL a[8]*b[4]"); goto cleanup; }

    /* An output too small for its inputs is the same defect. */
    eai_tensor_t t2[] = {tensors[1], tensors[1], tensors[0]};
    st = dispatch_binary(EAI_OP_ADD, t2, 3, 0, 1, 2);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for ADD b[4]+b[4] into c[8]"); goto cleanup; }
    PASS();
cleanup:
    eai_tensor_destroy(&tensors[0]);
    eai_tensor_destroy(&tensors[1]);
    eai_tensor_destroy(&tensors[2]);
}

/* The same overflow caught without a sanitizer: the output buffer is longer
 * than the tensor says it is, and the bytes past the reported end carry a
 * known pattern. The unfixed kernel wrote 8 floats into a 4-float tensor and
 * destroyed it. */
static void test_cpu_elementwise_mismatch_leaves_canary_intact(void)
{
    TEST(cpu_elementwise_mismatch_leaves_canary_intact);
    ensure_cpu_registered();

    enum { REPORTED = 4, ALLOCATED = 16 };
    float *buf = malloc(ALLOCATED * sizeof(float));
    if (!buf) { FAIL("malloc"); return; }
    memset(buf, CANARY_BYTE, ALLOCATED * sizeof(float));

    int64_t s8[] = {8};
    eai_tensor_t a, b;
    eai_tensor_create(&a, EAI_DTYPE_F32, s8, 1);
    eai_tensor_create(&b, EAI_DTYPE_F32, s8, 1);
    for (int i = 0; i < 8; i++) {
        ((float *)a.data)[i] = 1.0f;
        ((float *)b.data)[i] = 2.0f;
    }

    /* c reports 4 elements; its buffer really holds 16. */
    eai_tensor_t c;
    memset(&c, 0, sizeof(c));
    c.dtype = EAI_DTYPE_F32;
    c.ndim = 1;
    c.shape[0] = REPORTED;
    c.strides[0] = 1;
    c.data = buf;
    c.data_size = REPORTED * sizeof(float);
    c.owns_data = false;

    eai_tensor_t tensors[] = {a, b, c};
    eai_status_t st = dispatch_binary(EAI_OP_ADD, tensors, 3, 0, 1, 2);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for ADD a[8]+b[8] into c[4]"); goto cleanup; }

    /* Nothing at all should have been written: the graph was refused. */
    const unsigned char *bytes = (const unsigned char *)buf;
    for (size_t i = 0; i < ALLOCATED * sizeof(float); i++) {
        if (bytes[i] != CANARY_BYTE) { FAIL("output buffer was written past its length"); goto cleanup; }
    }
    PASS();
cleanup:
    eai_tensor_destroy(&tensors[0]);
    eai_tensor_destroy(&tensors[1]);
    free(buf);
}

/* RELU in[8] -> out[4]: the count came from the input alone. */
static void test_cpu_refuses_unary_shape_mismatch(void)
{
    TEST(cpu_refuses_unary_shape_mismatch);
    ensure_cpu_registered();

    int64_t s8[] = {8}, s4[] = {4};
    eai_tensor_t in, out;
    eai_tensor_create(&in, EAI_DTYPE_F32, s8, 1);
    eai_tensor_create(&out, EAI_DTYPE_F32, s4, 1);
    eai_tensor_t tensors[] = {in, out};

    eai_status_t st = dispatch_unary(EAI_OP_RELU, tensors, 2, 0, 1);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for RELU in[8] -> out[4]"); goto cleanup; }
    st = dispatch_unary(EAI_OP_SOFTMAX, tensors, 2, 0, 1);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for SOFTMAX in[8] -> out[4]"); goto cleanup; }
    st = dispatch_unary(EAI_OP_LAYERNORM, tensors, 2, 0, 1);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for LAYERNORM in[8] -> out[4]"); goto cleanup; }
    PASS();
cleanup:
    eai_tensor_destroy(&tensors[0]);
    eai_tensor_destroy(&tensors[1]);
}

/* MATMUL a[2,3] x b[2,2]: K came from a, so the kernel read a row of b that
 * does not exist. */
static void test_cpu_refuses_matmul_inner_dimension_mismatch(void)
{
    TEST(cpu_refuses_matmul_inner_dimension_mismatch);
    ensure_cpu_registered();

    int64_t s23[] = {2, 3}, s22[] = {2, 2};
    eai_tensor_t a, b, c;
    eai_tensor_create(&a, EAI_DTYPE_F32, s23, 2);
    eai_tensor_create(&b, EAI_DTYPE_F32, s22, 2);
    eai_tensor_create(&c, EAI_DTYPE_F32, s22, 2);
    eai_tensor_t tensors[] = {a, b, c};

    eai_status_t st = dispatch_binary(EAI_OP_MATMUL, tensors, 3, 0, 1, 2);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for a[2,3] x b[2,2]"); goto cleanup; }
    PASS();
cleanup:
    eai_tensor_destroy(&tensors[0]);
    eai_tensor_destroy(&tensors[1]);
    eai_tensor_destroy(&tensors[2]);
}

/* MATMUL 2x2 inputs into a [1,1] output: four floats written into one. A
 * 1-D operand is refused too - the kernel indexes shape[1] unconditionally. */
static void test_cpu_refuses_matmul_output_shape_mismatch(void)
{
    TEST(cpu_refuses_matmul_output_shape_mismatch);
    ensure_cpu_registered();

    int64_t s22[] = {2, 2}, s11[] = {1, 1}, s4[] = {4};
    eai_tensor_t a, b, small, flat;
    eai_tensor_create(&a, EAI_DTYPE_F32, s22, 2);
    eai_tensor_create(&b, EAI_DTYPE_F32, s22, 2);
    eai_tensor_create(&small, EAI_DTYPE_F32, s11, 2);
    eai_tensor_create(&flat, EAI_DTYPE_F32, s4, 1);
    eai_tensor_t tensors[] = {a, b, small, flat};

    eai_status_t st = dispatch_binary(EAI_OP_MATMUL, tensors, 4, 0, 1, 2);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for 2x2 x 2x2 -> [1,1]"); goto cleanup; }

    st = dispatch_binary(EAI_OP_MATMUL, tensors, 4, 0, 1, 3);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for a 1-D matmul output"); goto cleanup; }

    st = dispatch_binary(EAI_OP_MATMUL, tensors, 4, 3, 1, 0);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for a 1-D matmul input"); goto cleanup; }
    PASS();
cleanup:
    eai_tensor_destroy(&tensors[0]);
    eai_tensor_destroy(&tensors[1]);
    eai_tensor_destroy(&tensors[2]);
    eai_tensor_destroy(&tensors[3]);
}

/* A tensor of shape {0} holds nothing, but cpu_softmax_f32 reads in[0]
 * before its loop starts. */
static void test_cpu_refuses_zero_element_tensor(void)
{
    TEST(cpu_refuses_zero_element_tensor);
    ensure_cpu_registered();

    int64_t s0[] = {0};
    eai_tensor_t in, out;
    eai_tensor_create(&in, EAI_DTYPE_F32, s0, 1);
    eai_tensor_create(&out, EAI_DTYPE_F32, s0, 1);
    eai_tensor_t tensors[] = {in, out};

    eai_status_t st = dispatch_unary(EAI_OP_SOFTMAX, tensors, 2, 0, 1);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for a tensor of shape {0}"); goto cleanup; }
    st = dispatch_unary(EAI_OP_LAYERNORM, tensors, 2, 0, 1);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for LAYERNORM on {0}"); goto cleanup; }
    PASS();
cleanup:
    eai_tensor_destroy(&tensors[0]);
    eai_tensor_destroy(&tensors[1]);
}

/* A negative dimension made every loop bound in the kernels meaningless and
 * still reported EAI_OK. */
static void test_cpu_refuses_negative_dimension(void)
{
    TEST(cpu_refuses_negative_dimension);
    ensure_cpu_registered();

    int64_t s4[] = {4};
    eai_tensor_t in, out;
    eai_tensor_create(&in, EAI_DTYPE_F32, s4, 1);
    eai_tensor_create(&out, EAI_DTYPE_F32, s4, 1);
    eai_tensor_t tensors[] = {in, out};
    tensors[0].shape[0] = -1;
    tensors[1].shape[0] = -1;

    eai_status_t st = dispatch_unary(EAI_OP_SOFTMAX, tensors, 2, 0, 1);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for shape[0] = -1"); goto cleanup; }
    PASS();
cleanup:
    tensors[0].shape[0] = 4;
    tensors[1].shape[0] = 4;
    eai_tensor_destroy(&tensors[0]);
    eai_tensor_destroy(&tensors[1]);
}

/* eai_tensor_numel() returns int64_t and every kernel call casts it to int.
 * An element count of 2^32 + 1000 truncated to 1000 and walked a buffer
 * sized for neither number. */
static void test_cpu_refuses_element_count_that_overflows_int(void)
{
    TEST(cpu_refuses_element_count_that_overflows_int);
    ensure_cpu_registered();

    int64_t s4[] = {4};
    eai_tensor_t in, out;
    eai_tensor_create(&in, EAI_DTYPE_F32, s4, 1);
    eai_tensor_create(&out, EAI_DTYPE_F32, s4, 1);
    eai_tensor_t tensors[] = {in, out};
    /* 2^32 + 1000, which truncates to 1000 in an int. */
    tensors[0].shape[0] = 4294968296LL;
    tensors[1].shape[0] = 4294968296LL;

    eai_status_t st = dispatch_unary(EAI_OP_RELU, tensors, 2, 0, 1);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for an element count past INT_MAX"); goto cleanup; }
    PASS();
cleanup:
    tensors[0].shape[0] = 4;
    tensors[1].shape[0] = 4;
    eai_tensor_destroy(&tensors[0]);
    eai_tensor_destroy(&tensors[1]);
}

/* A RELU whose input is a view claiming more elements than its source holds.
 * The view is now refused at creation; the dispatch is checked too, so this
 * stays a regression test for the backend if views ever change again. */
static void test_cpu_refuses_oversized_view(void)
{
    TEST(cpu_refuses_oversized_view);
    ensure_cpu_registered();

    int64_t s4[] = {4}, s8[] = {8};
    eai_tensor_t src, view, out;
    eai_tensor_create(&src, EAI_DTYPE_F32, s4, 1);
    eai_tensor_create(&out, EAI_DTYPE_F32, s8, 1);

    eai_status_t vst = eai_tensor_create_view(&view, &src, NULL, s8, 1);
    if (vst == EAI_OK) {
        /* If a view may still outgrow its source, the backend must refuse it. */
        eai_tensor_t tensors[] = {view, out};
        eai_status_t st = dispatch_unary(EAI_OP_RELU, tensors, 2, 0, 1);
        if (st != EAI_ERR_INVALID) { FAIL("expected INVALID for a view larger than its source"); goto cleanup; }
    }
    PASS();
cleanup:
    eai_tensor_destroy(&src);
    eai_tensor_destroy(&out);
}

/* MATMUL accumulates into c while still reading a and b. With c aliasing a,
 * the backend returned EAI_OK and the wrong numbers: 19 130 43 290 where
 * 19 22 43 50 was correct. */
static void test_cpu_refuses_matmul_output_aliasing_input(void)
{
    TEST(cpu_refuses_matmul_output_aliasing_input);
    ensure_cpu_registered();

    int64_t s22[] = {2, 2};
    eai_tensor_t a, b;
    eai_tensor_create(&a, EAI_DTYPE_F32, s22, 2);
    eai_tensor_create(&b, EAI_DTYPE_F32, s22, 2);
    const float av[] = {1, 2, 3, 4}, bv[] = {5, 6, 7, 8};
    memcpy(a.data, av, sizeof(av));
    memcpy(b.data, bv, sizeof(bv));
    eai_tensor_t tensors[] = {a, b};

    eai_status_t st = dispatch_binary(EAI_OP_MATMUL, tensors, 2, 0, 1, 0);
    if (st != EAI_ERR_INVALID) { FAIL("expected INVALID when the output aliases an input"); goto cleanup; }

    /* Refused means untouched. */
    for (int i = 0; i < 4; i++) {
        if (((float *)tensors[0].data)[i] != av[i]) { FAIL("input was overwritten"); goto cleanup; }
    }
    PASS();
cleanup:
    eai_tensor_destroy(&tensors[0]);
    eai_tensor_destroy(&tensors[1]);
}

/* Two views of one source can overlap while starting at different addresses,
 * so a guard comparing only base pointers lets the kernel write over the
 * input it is still reading. Views [0,0] and [1,0] of a [3,2] source share
 * its middle row: with a = [1,2,3,4] the unguarded run returned
 * [19,22,249,290] instead of [19,22,43,50]. */
static void test_cpu_refuses_matmul_output_overlapping_input_view(void)
{
    TEST(cpu_refuses_matmul_output_overlapping_input_view);
    ensure_cpu_registered();

    int64_t s32[] = {3, 2}, s22[] = {2, 2};
    int64_t off0[] = {0, 0}, off1[] = {1, 0};
    eai_tensor_t src, b, va, vc;
    eai_tensor_create(&src, EAI_DTYPE_F32, s32, 2);
    eai_tensor_create(&b, EAI_DTYPE_F32, s22, 2);
    const float sv[] = {1, 2, 3, 4, 0, 0}, bv[] = {5, 6, 7, 8};
    memcpy(src.data, sv, sizeof(sv));
    memcpy(b.data, bv, sizeof(bv));

    if (eai_tensor_create_view(&va, &src, off0, s22, 2) != EAI_OK) {
        FAIL("a [2,2] view at [0,0] of a [3,2] source should be valid"); goto cleanup;
    }
    if (eai_tensor_create_view(&vc, &src, off1, s22, 2) != EAI_OK) {
        FAIL("a [2,2] view at [1,0] of a [3,2] source should be valid"); goto cleanup;
    }
    if (va.data == vc.data) { FAIL("the views should start at different addresses"); goto cleanup; }

    eai_tensor_t tensors[] = {va, b, vc};
    eai_status_t st = dispatch_binary(EAI_OP_MATMUL, tensors, 3, 0, 1, 2);
    if (st != EAI_ERR_INVALID) {
        FAIL("expected INVALID when the output view overlaps an input view"); goto cleanup;
    }

    /* Refused means untouched: the source still holds its original rows. */
    for (int i = 0; i < 6; i++) {
        if (((float *)src.data)[i] != sv[i]) { FAIL("the source was overwritten"); goto cleanup; }
    }
    PASS();
cleanup:
    eai_tensor_destroy(&src);
    eai_tensor_destroy(&b);
}

/* Disjoint views of one source are legal and must keep working, or the
 * overlap check would have been a blanket ban on views. Rows 0 and 2 of a
 * [4,2] source do not intersect. */
static void test_cpu_allows_matmul_over_disjoint_views(void)
{
    TEST(cpu_allows_matmul_over_disjoint_views);
    ensure_cpu_registered();

    int64_t s42[] = {4, 2}, s22[] = {2, 2};
    int64_t off0[] = {0, 0}, off2[] = {2, 0};
    eai_tensor_t src, b, va, vc;
    eai_tensor_create(&src, EAI_DTYPE_F32, s42, 2);
    eai_tensor_create(&b, EAI_DTYPE_F32, s22, 2);
    const float sv[] = {1, 2, 3, 4, 0, 0, 0, 0}, bv[] = {5, 6, 7, 8};
    memcpy(src.data, sv, sizeof(sv));
    memcpy(b.data, bv, sizeof(bv));

    if (eai_tensor_create_view(&va, &src, off0, s22, 2) != EAI_OK ||
        eai_tensor_create_view(&vc, &src, off2, s22, 2) != EAI_OK) {
        FAIL("disjoint views of a [4,2] source should be valid"); goto cleanup;
    }

    eai_tensor_t tensors[] = {va, b, vc};
    if (dispatch_binary(EAI_OP_MATMUL, tensors, 3, 0, 1, 2) != EAI_OK) {
        FAIL("a matmul over disjoint views was refused"); goto cleanup;
    }

    /* [1,2;3,4] x [5,6;7,8] = [19,22;43,50], written to rows 2-3. */
    const float want[] = {19, 22, 43, 50};
    for (int i = 0; i < 4; i++) {
        if (((float *)src.data)[4 + i] != want[i]) { FAIL("wrong matmul result"); goto cleanup; }
    }
    /* The input rows are untouched. */
    for (int i = 0; i < 4; i++) {
        if (((float *)src.data)[i] != sv[i]) { FAIL("the input rows were modified"); goto cleanup; }
    }
    PASS();
cleanup:
    eai_tensor_destroy(&src);
    eai_tensor_destroy(&b);
}

/* The checks must not cost well-formed graphs anything: a matmul whose
 * output happens to be a separate buffer of the right shape still runs. */
static void test_cpu_still_runs_well_formed_graphs(void)
{
    TEST(cpu_still_runs_well_formed_graphs);
    ensure_cpu_registered();

    int64_t s22[] = {2, 2};
    eai_tensor_t a, b, c;
    eai_tensor_create(&a, EAI_DTYPE_F32, s22, 2);
    eai_tensor_create(&b, EAI_DTYPE_F32, s22, 2);
    eai_tensor_create(&c, EAI_DTYPE_F32, s22, 2);
    const float av[] = {1, 2, 3, 4}, bv[] = {5, 6, 7, 8};
    memcpy(a.data, av, sizeof(av));
    memcpy(b.data, bv, sizeof(bv));
    eai_tensor_t tensors[] = {a, b, c};

    eai_status_t st = dispatch_binary(EAI_OP_MATMUL, tensors, 3, 0, 1, 2);
    if (st != EAI_OK) { FAIL("a well-formed matmul was refused"); goto cleanup; }

    const float want[] = {19, 22, 43, 50};
    for (int i = 0; i < 4; i++) {
        if (fabsf(((float *)tensors[2].data)[i] - want[i]) > 1e-5f) {
            FAIL("well-formed matmul gave the wrong answer"); goto cleanup;
        }
    }
    PASS();
cleanup:
    eai_tensor_destroy(&tensors[0]);
    eai_tensor_destroy(&tensors[1]);
    eai_tensor_destroy(&tensors[2]);
}

int main(void)
{
    printf("=== EAI Accel Backend Tests (Comprehensive) ===\n\n");

    test_register_null_ops();
    test_register_null_name();
    test_register_duplicate();
    test_find_null_name();
    test_find_nonexistent();
    test_find_by_type_cpu();
    test_find_by_type_nonexistent();
    test_accel_list();
    test_accel_list_limited();
    test_cpu_backend_info();
    test_cpu_backend_info_null();
    test_cpu_can_handle_null_op();
    test_cpu_can_handle_valid_op();
    test_dispatch_null_graph();
    test_dispatch_null_ops_in_graph();
    test_dispatch_zero_op_count();
    test_cpu_relu();
    test_cpu_add();
    test_cpu_mul();
    test_cpu_layernorm();
    test_cpu_softmax_large_values();
    test_cpu_matmul_identity();
    test_cpu_can_handle_only_implemented_ops();
    test_cpu_refuses_unimplemented_op();
    test_cpu_refuses_non_f32_tensor();
    test_cpu_refuses_tensor_index_out_of_range();
    test_cpu_refuses_tensor_without_data();
    test_cpu_runs_no_op_of_a_graph_it_refuses();
    test_cpu_refuses_elementwise_shape_mismatch();
    test_cpu_elementwise_mismatch_leaves_canary_intact();
    test_cpu_refuses_unary_shape_mismatch();
    test_cpu_refuses_matmul_inner_dimension_mismatch();
    test_cpu_refuses_matmul_output_shape_mismatch();
    test_cpu_refuses_zero_element_tensor();
    test_cpu_refuses_negative_dimension();
    test_cpu_refuses_element_count_that_overflows_int();
    test_cpu_refuses_oversized_view();
    test_cpu_refuses_matmul_output_aliasing_input();
    test_cpu_refuses_matmul_output_overlapping_input_view();
    test_cpu_allows_matmul_over_disjoint_views();
    test_cpu_still_runs_well_formed_graphs();
    test_graph_destroy_null();
    test_graph_destroy_empty();

    printf("\n=== Results: %d/%d passed, %d failed ===\n",
           tests_passed, tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
