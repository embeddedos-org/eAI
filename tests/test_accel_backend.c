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
    test_graph_destroy_null();
    test_graph_destroy_empty();

    printf("\n=== Results: %d/%d passed, %d failed ===\n",
           tests_passed, tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
