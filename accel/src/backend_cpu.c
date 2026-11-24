// SPDX-License-Identifier: MIT
// CPU reference backend for accelerator dispatch

#include "eai/accel.h"
#include "eai/log.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define LOG_MOD "accel-cpu"

static eai_status_t cpu_init(eai_accel_backend_t *backend, const eai_kv_t *config, int config_count)
{
    (void)config; (void)config_count;
    EAI_LOG_INFO(LOG_MOD, "CPU reference backend initialized");
    return EAI_OK;
}

/* Number of input tensors the kernel for `type` reads, or 0 when this backend
 * has no kernel for it. Every kernel below writes one output. */
static int cpu_kernel_inputs(eai_op_type_t type)
{
    switch (type) {
        case EAI_OP_MATMUL:
        case EAI_OP_ADD:
        case EAI_OP_MUL:
            return 2;
        case EAI_OP_RELU:
        case EAI_OP_SOFTMAX:
        case EAI_OP_LAYERNORM:
            return 1;
        default:
            return 0;
    }
}

static bool cpu_can_handle(eai_accel_backend_t *backend, const eai_op_t *op)
{
    (void)backend;
    return (op != NULL) && cpu_kernel_inputs(op->type) > 0;
}

static eai_status_t cpu_prepare(eai_accel_backend_t *backend, const eai_compute_graph_t *graph)
{
    (void)backend; (void)graph;
    return EAI_OK;
}

/* ---- Kernel implementations ---- */

static void cpu_matmul_f32(const float *a, const float *b, float *c,
                           int M, int N, int K)
{
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            float sum = 0.0f;
            for (int k = 0; k < K; k++) {
                sum += a[i * K + k] * b[k * N + j];
            }
            c[i * N + j] = sum;
        }
    }
}

static void cpu_relu_f32(const float *in, float *out, int n)
{
    for (int i = 0; i < n; i++) {
        out[i] = (in[i] > 0.0f) ? in[i] : 0.0f;
    }
}

static void cpu_softmax_f32(const float *in, float *out, int n)
{
    float max_val = in[0];
    for (int i = 1; i < n; i++) {
        if (in[i] > max_val) max_val = in[i];
    }

    float sum = 0.0f;
    for (int i = 0; i < n; i++) {
        out[i] = expf(in[i] - max_val);
        sum += out[i];
    }

    float inv_sum = 1.0f / sum;
    for (int i = 0; i < n; i++) {
        out[i] *= inv_sum;
    }
}

static void cpu_layernorm_f32(const float *in, float *out, int n,
                               const float *gamma, const float *beta, float eps)
{
    float mean = 0.0f;
    for (int i = 0; i < n; i++) mean += in[i];
    mean /= (float)n;

    float var = 0.0f;
    for (int i = 0; i < n; i++) {
        float d = in[i] - mean;
        var += d * d;
    }
    var /= (float)n;

    float inv_std = 1.0f / sqrtf(var + eps);
    for (int i = 0; i < n; i++) {
        float norm = (in[i] - mean) * inv_std;
        out[i] = (gamma ? gamma[i] : 1.0f) * norm + (beta ? beta[i] : 0.0f);
    }
}

/* A tensor a kernel may touch: inside the graph, backed by data, float32.
 * The kernels cast `data` to float *, so any other dtype would be read and
 * written past the end of a buffer that eai_tensor_create() sized by dtype. */
static eai_status_t cpu_check_tensor(const eai_compute_graph_t *graph, int index)
{
    if (!graph->tensors || index < 0 || index >= graph->tensor_count)
        return EAI_ERR_INVALID;
    if (!graph->tensors[index].data)
        return EAI_ERR_INVALID;
    if (graph->tensors[index].dtype != EAI_DTYPE_F32)
        return EAI_ERR_UNSUPPORTED;
    return EAI_OK;
}

/* Check the whole graph before any kernel runs, so a graph that is refused
 * has had none of its tensors written. */
static eai_status_t cpu_check_graph(const eai_compute_graph_t *graph)
{
    for (int i = 0; i < graph->op_count; i++) {
        const eai_op_t *op = &graph->ops[i];
        int inputs = cpu_kernel_inputs(op->type);

        if (inputs == 0) {
            EAI_LOG_WARN(LOG_MOD, "no CPU kernel for op type: %d", op->type);
            return EAI_ERR_UNSUPPORTED;
        }
        if (op->input_count < inputs || op->output_count < 1 ||
            !op->input_indices || !op->output_indices)
            return EAI_ERR_INVALID;

        for (int j = 0; j < inputs; j++) {
            eai_status_t st = cpu_check_tensor(graph, op->input_indices[j]);
            if (st != EAI_OK) return st;
        }
        eai_status_t st = cpu_check_tensor(graph, op->output_indices[0]);
        if (st != EAI_OK) return st;
    }
    return EAI_OK;
}

static eai_status_t cpu_execute(eai_accel_backend_t *backend, const eai_compute_graph_t *graph)
{
    if (!graph || !graph->ops) return EAI_ERR_INVALID;
    (void)backend;

    eai_status_t check = cpu_check_graph(graph);
    if (check != EAI_OK) return check;

    for (int i = 0; i < graph->op_count; i++) {
        const eai_op_t *op = &graph->ops[i];

        switch (op->type) {
            case EAI_OP_MATMUL: {
                if (op->input_count < 2 || op->output_count < 1) return EAI_ERR_INVALID;
                eai_tensor_t *a = &graph->tensors[op->input_indices[0]];
                eai_tensor_t *b = &graph->tensors[op->input_indices[1]];
                eai_tensor_t *c = &graph->tensors[op->output_indices[0]];
                int M = (int)a->shape[0], K = (int)a->shape[1], N = (int)b->shape[1];
                cpu_matmul_f32((float *)a->data, (float *)b->data, (float *)c->data, M, N, K);
                break;
            }
            case EAI_OP_RELU: {
                if (op->input_count < 1 || op->output_count < 1) return EAI_ERR_INVALID;
                eai_tensor_t *in = &graph->tensors[op->input_indices[0]];
                eai_tensor_t *out = &graph->tensors[op->output_indices[0]];
                cpu_relu_f32((float *)in->data, (float *)out->data, (int)eai_tensor_numel(in));
                break;
            }
            case EAI_OP_SOFTMAX: {
                if (op->input_count < 1 || op->output_count < 1) return EAI_ERR_INVALID;
                eai_tensor_t *in = &graph->tensors[op->input_indices[0]];
                eai_tensor_t *out = &graph->tensors[op->output_indices[0]];
                cpu_softmax_f32((float *)in->data, (float *)out->data, (int)eai_tensor_numel(in));
                break;
            }
            case EAI_OP_LAYERNORM: {
                if (op->input_count < 1 || op->output_count < 1) return EAI_ERR_INVALID;
                eai_tensor_t *in = &graph->tensors[op->input_indices[0]];
                eai_tensor_t *out = &graph->tensors[op->output_indices[0]];
                cpu_layernorm_f32((float *)in->data, (float *)out->data,
                                  (int)eai_tensor_numel(in), NULL, NULL, 1e-5f);
                break;
            }
            case EAI_OP_ADD: {
                if (op->input_count < 2 || op->output_count < 1) return EAI_ERR_INVALID;
                eai_tensor_t *a = &graph->tensors[op->input_indices[0]];
                eai_tensor_t *b = &graph->tensors[op->input_indices[1]];
                eai_tensor_t *c = &graph->tensors[op->output_indices[0]];
                int n = (int)eai_tensor_numel(a);
                float *fa = (float *)a->data, *fb = (float *)b->data, *fc = (float *)c->data;
                for (int j = 0; j < n; j++) fc[j] = fa[j] + fb[j];
                break;
            }
            case EAI_OP_MUL: {
                if (op->input_count < 2 || op->output_count < 1) return EAI_ERR_INVALID;
                eai_tensor_t *a = &graph->tensors[op->input_indices[0]];
                eai_tensor_t *b = &graph->tensors[op->input_indices[1]];
                eai_tensor_t *c = &graph->tensors[op->output_indices[0]];
                int n = (int)eai_tensor_numel(a);
                float *fa = (float *)a->data, *fb = (float *)b->data, *fc = (float *)c->data;
                for (int j = 0; j < n; j++) fc[j] = fa[j] * fb[j];
                break;
            }
            default:
                /* Not reachable: cpu_check_graph() refused the graph. */
                return EAI_ERR_UNSUPPORTED;
        }
    }

    return EAI_OK;
}

static eai_status_t cpu_get_info(eai_accel_backend_t *backend, eai_accel_backend_info_t *info)
{
    (void)backend;
    if (!info) return EAI_ERR_INVALID;
    strncpy(info->name, "CPU Reference", sizeof(info->name) - 1);
    info->type = EAI_BACKEND_CPU;
    info->memory_bytes = 0;
    info->compute_units = 1;
    return EAI_OK;
}

static void cpu_shutdown(eai_accel_backend_t *backend)
{
    (void)backend;
    EAI_LOG_INFO(LOG_MOD, "CPU reference backend shut down");
}

const eai_accel_backend_ops_t eai_accel_cpu_ops = {
    .name       = "cpu",
    .type       = EAI_BACKEND_CPU,
    .init       = cpu_init,
    .can_handle = cpu_can_handle,
    .prepare    = cpu_prepare,
    .execute    = cpu_execute,
    .get_info   = cpu_get_info,
    .shutdown   = cpu_shutdown,
};
