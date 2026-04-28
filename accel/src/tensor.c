// SPDX-License-Identifier: MIT
// Tensor library implementation

#include "eai/accel.h"
#include <stdlib.h>
#include <string.h>

size_t eai_dtype_size(eai_dtype_t dtype)
{
    switch (dtype) {
        case EAI_DTYPE_F32:   return 4;
        case EAI_DTYPE_F16:   return 2;
        case EAI_DTYPE_BF16:  return 2;
        case EAI_DTYPE_INT8:  return 1;
        case EAI_DTYPE_INT4:  return 1; /* packed 2 per byte, but element-level = 1 */
        case EAI_DTYPE_Q4_0:  return 1;
        case EAI_DTYPE_Q4_1:  return 1;
        case EAI_DTYPE_Q8_0:  return 1;
        case EAI_DTYPE_INT32: return 4;
        default:              return 0;
    }
}

int64_t eai_tensor_numel(const eai_tensor_t *t)
{
    if (!t || t->ndim <= 0) return 0;
    int64_t n = 1;
    for (int i = 0; i < t->ndim; i++) {
        n *= t->shape[i];
    }
    return n;
}

eai_status_t eai_tensor_create(eai_tensor_t *t, eai_dtype_t dtype,
                                const int64_t *shape, int ndim)
{
    if (!t || !shape || ndim <= 0 || ndim > EAI_TENSOR_MAX_DIMS)
        return EAI_ERR_INVALID;

    memset(t, 0, sizeof(*t));
    t->dtype = dtype;
    t->ndim = ndim;

    int64_t numel = 1;
    for (int i = 0; i < ndim; i++) {
        t->shape[i] = shape[i];
        numel *= shape[i];
    }

    /* Compute row-major strides */
    t->strides[ndim - 1] = 1;
    for (int i = ndim - 2; i >= 0; i--) {
        t->strides[i] = t->strides[i + 1] * t->shape[i + 1];
    }

    size_t elem_size = eai_dtype_size(dtype);
    t->data_size = (size_t)numel * elem_size;
    t->data = calloc(1, t->data_size);
    if (!t->data) return EAI_ERR_NOMEM;
    t->owns_data = true;

    return EAI_OK;
}

eai_status_t eai_tensor_create_view(eai_tensor_t *t, eai_tensor_t *src,
                                     const int64_t *offset, const int64_t *shape, int ndim)
{
    if (!t || !src || !shape || ndim <= 0 || ndim > EAI_TENSOR_MAX_DIMS)
        return EAI_ERR_INVALID;

    memset(t, 0, sizeof(*t));
    t->dtype = src->dtype;
    t->ndim = ndim;

    size_t elem_size = eai_dtype_size(src->dtype);
    size_t byte_offset = 0;
    for (int i = 0; i < ndim && i < src->ndim; i++) {
        t->shape[i] = shape[i];
        t->strides[i] = src->strides[i];
        if (offset) byte_offset += (size_t)(offset[i] * src->strides[i]) * elem_size;
    }

    t->data = (uint8_t *)src->data + byte_offset;
    t->data_size = 0; /* view doesn't own data */
    t->owns_data = false;

    return EAI_OK;
}

void eai_tensor_destroy(eai_tensor_t *t)
{
    if (t && t->owns_data && t->data) {
        free(t->data);
    }
    if (t) memset(t, 0, sizeof(*t));
}
