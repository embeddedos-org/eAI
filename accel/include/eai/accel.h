// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// Accelerator backend API — ExecuTorch-style delegate pattern

#ifndef EAI_ACCEL_H
#define EAI_ACCEL_H

#include "eai/types.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== Data Types ========== */

typedef enum {
    EAI_DTYPE_F32  = 0,
    EAI_DTYPE_F16  = 1,
    EAI_DTYPE_BF16 = 2,
    EAI_DTYPE_INT8 = 3,
    EAI_DTYPE_INT4 = 4,
    EAI_DTYPE_Q4_0 = 5,
    EAI_DTYPE_Q4_1 = 6,
    EAI_DTYPE_Q8_0 = 7,
    EAI_DTYPE_INT32 = 8,
} eai_dtype_t;

/* ========== Tensor ========== */

#define EAI_TENSOR_MAX_DIMS 8

typedef struct {
    eai_dtype_t dtype;
    int         ndim;
    int64_t     shape[EAI_TENSOR_MAX_DIMS];
    int64_t     strides[EAI_TENSOR_MAX_DIMS];
    void       *data;
    size_t      data_size;
    bool        owns_data;
} eai_tensor_t;

eai_status_t eai_tensor_create(eai_tensor_t *t, eai_dtype_t dtype,
                                const int64_t *shape, int ndim);
eai_status_t eai_tensor_create_view(eai_tensor_t *t, eai_tensor_t *src,
                                     const int64_t *offset, const int64_t *shape, int ndim);
void         eai_tensor_destroy(eai_tensor_t *t);
size_t       eai_dtype_size(eai_dtype_t dtype);
int64_t      eai_tensor_numel(const eai_tensor_t *t);

/* ========== Operations ========== */

typedef enum {
    EAI_OP_MATMUL = 0,
    EAI_OP_CONV2D,
    EAI_OP_RELU,
    EAI_OP_SOFTMAX,
    EAI_OP_LAYERNORM,
    EAI_OP_EMBEDDING,
    EAI_OP_ATTENTION,
    EAI_OP_ADD,
    EAI_OP_MUL,
    EAI_OP_RESHAPE,
    EAI_OP_TRANSPOSE,
    EAI_OP_CUSTOM,
} eai_op_type_t;

typedef struct {
    eai_op_type_t type;
    int           input_count;
    int           output_count;
    int          *input_indices;
    int          *output_indices;
    const eai_kv_t *attrs;
    int             attr_count;
} eai_op_t;

/* ========== Compute Graph ========== */

typedef struct {
    eai_op_t     *ops;
    int           op_count;
    eai_tensor_t *tensors;
    int           tensor_count;
} eai_compute_graph_t;

void eai_compute_graph_destroy(eai_compute_graph_t *graph);

/* ========== Accelerator Backend ========== */

typedef enum {
    EAI_BACKEND_CPU    = 0,
    EAI_BACKEND_VULKAN = 1,
    EAI_BACKEND_COREML = 2,
    EAI_BACKEND_QNN    = 3,
    EAI_BACKEND_CUDA   = 4,
} eai_backend_type_t;

typedef struct {
    char               name[64];
    eai_backend_type_t type;
    uint64_t           memory_bytes;
    uint32_t           compute_units;
} eai_accel_backend_info_t;

typedef struct eai_accel_backend_s eai_accel_backend_t;

typedef struct {
    const char        *name;
    eai_backend_type_t type;

    eai_status_t (*init)(eai_accel_backend_t *backend, const eai_kv_t *config, int config_count);
    bool         (*can_handle)(eai_accel_backend_t *backend, const eai_op_t *op);
    eai_status_t (*prepare)(eai_accel_backend_t *backend, const eai_compute_graph_t *graph);
    eai_status_t (*execute)(eai_accel_backend_t *backend, const eai_compute_graph_t *graph);
    eai_status_t (*get_info)(eai_accel_backend_t *backend, eai_accel_backend_info_t *info);
    void         (*shutdown)(eai_accel_backend_t *backend);
} eai_accel_backend_ops_t;

struct eai_accel_backend_s {
    const eai_accel_backend_ops_t *ops;
    void                          *ctx;
    bool                           initialized;
};

/* ========== Backend Registry ========== */

#define EAI_MAX_ACCEL_BACKENDS 8

eai_status_t eai_accel_register(const eai_accel_backend_ops_t *ops);
const eai_accel_backend_ops_t *eai_accel_find(const char *name);
const eai_accel_backend_ops_t *eai_accel_find_by_type(eai_backend_type_t type);
int          eai_accel_list(const eai_accel_backend_ops_t **out, int max);
eai_status_t eai_accel_dispatch(eai_compute_graph_t *graph);

/* Reset the backend registry (for testing only) */
void eai_accel_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* EAI_ACCEL_H */
