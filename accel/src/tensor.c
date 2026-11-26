// SPDX-License-Identifier: MIT
// Tensor library implementation

#include "eai/accel.h"
#include <stdint.h>
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

    /* numel and the byte size derived from it must both be representable, or
     * the allocation silently wraps: a 2^62+1 element F32 tensor multiplied
     * out to 4 bytes, calloc succeeded, and the tensor then reported a
     * data_size of 4 for a shape promising exabytes. Every bounds check
     * downstream trusts data_size, so the wrap defeated all of them. */
    int64_t numel = 1;
    for (int i = 0; i < ndim; i++) {
        if (shape[i] <= 0) return EAI_ERR_INVALID;
        if (numel > INT64_MAX / shape[i]) return EAI_ERR_INVALID;
        t->shape[i] = shape[i];
        numel *= shape[i];
    }

    /* Compute row-major strides */
    t->strides[ndim - 1] = 1;
    for (int i = ndim - 2; i >= 0; i--) {
        t->strides[i] = t->strides[i + 1] * t->shape[i + 1];
    }

    size_t elem_size = eai_dtype_size(dtype);
    if (elem_size == 0) return EAI_ERR_INVALID;
    if ((uint64_t)numel > (uint64_t)SIZE_MAX / elem_size) return EAI_ERR_INVALID;

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

    /* A view borrows the source's strides, so it cannot describe more
     * dimensions than the source has. The old loop ran to the smaller of the
     * two and left the remaining shape and stride entries zero, producing a
     * tensor that described neither. */
    if (ndim > src->ndim || !src->data)
        return EAI_ERR_INVALID;

    size_t elem_size = eai_dtype_size(src->dtype);
    if (elem_size == 0) return EAI_ERR_INVALID;

    /* Element offset of the view's first element, and the distance from there
     * to its last addressable element. Computed before *t is touched, so a
     * refused view leaves the caller's tensor alone.
     *
     * The per-dimension bound is the one that matters: a view was previously
     * free to name a shape larger than its source (a [8] view over a [4]
     * tensor), which handed every consumer a tensor whose shape promised
     * twice the memory that exists. */
    int64_t first = 0, last = 0;
    for (int i = 0; i < ndim; i++) {
        int64_t off = offset ? offset[i] : 0;
        if (shape[i] <= 0 || off < 0) return EAI_ERR_INVALID;
        if (off > src->shape[i] - shape[i]) return EAI_ERR_INVALID;
        first += off * src->strides[i];
        last  += (shape[i] - 1) * src->strides[i];
    }

    /* Both products must fit a size_t before they are formed. A source whose
     * own extent wrapped (or one built by hand) can carry a shape whose span
     * multiplies past SIZE_MAX, and the wrapped result would then compare
     * small enough to pass the bound below. */
    if ((uint64_t)first > (uint64_t)SIZE_MAX / elem_size) return EAI_ERR_INVALID;
    if ((uint64_t)(last + 1) > (uint64_t)SIZE_MAX / elem_size) return EAI_ERR_INVALID;

    size_t byte_offset = (size_t)first * elem_size;
    size_t span_bytes  = (size_t)(last + 1) * elem_size;

    /* The source's own buffer is the last word on what exists. */
    if (src->data_size < byte_offset || src->data_size - byte_offset < span_bytes)
        return EAI_ERR_INVALID;

    memset(t, 0, sizeof(*t));
    t->dtype = src->dtype;
    t->ndim = ndim;
    for (int i = 0; i < ndim; i++) {
        t->shape[i] = shape[i];
        t->strides[i] = src->strides[i];
    }

    t->data = (uint8_t *)src->data + byte_offset;
    /* The view's own extent in bytes, from its base pointer to its last
     * addressable element. A view used to report 0 here, which told every
     * bounds check downstream that the buffer behind it was empty - so no
     * bound derived from data_size could ever fire on a view. */
    t->data_size = span_bytes;
    t->owns_data = false; /* a view does not own what it points at */

    return EAI_OK;
}

void eai_tensor_destroy(eai_tensor_t *t)
{
    if (t && t->owns_data && t->data) {
        free(t->data);
    }
    if (t) memset(t, 0, sizeof(*t));
}
