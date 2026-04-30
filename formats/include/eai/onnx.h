// SPDX-License-Identifier: MIT
// ONNX model format loader

#ifndef EAI_ONNX_H
#define EAI_ONNX_H

#include "eai/types.h"
#include "eai/accel.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char      *name;
    int        op_type;     /* mapped to eai_op_type_t */
    char     **input_names;
    int        input_count;
    char     **output_names;
    int        output_count;
} onnx_node_t;

typedef struct {
    char      *name;
    int        dtype;
    int        ndim;
    int64_t    shape[EAI_TENSOR_MAX_DIMS];
    void      *data;
    size_t     data_size;
} onnx_tensor_t;

typedef struct {
    char         *model_name;
    int64_t       ir_version;
    int64_t       opset_version;
    onnx_node_t  *nodes;
    int           node_count;
    onnx_tensor_t *initializers;
    int            initializer_count;
} onnx_context_t;

eai_status_t eai_onnx_load(const char *path, onnx_context_t *ctx);
void         eai_onnx_free(onnx_context_t *ctx);
eai_status_t eai_onnx_to_graph(const onnx_context_t *ctx, eai_compute_graph_t *graph);

#ifdef __cplusplus
}
#endif

#endif /* EAI_ONNX_H */
