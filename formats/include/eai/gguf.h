// SPDX-License-Identifier: MIT
// GGUF model format loader

#ifndef EAI_GGUF_H
#define EAI_GGUF_H

#include "eai/types.h"
#include "eai/accel.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GGUF_MAGIC 0x46554747 /* "GGUF" in little-endian */

typedef enum {
    GGUF_TYPE_UINT8   = 0,
    GGUF_TYPE_INT8    = 1,
    GGUF_TYPE_UINT16  = 2,
    GGUF_TYPE_INT16   = 3,
    GGUF_TYPE_UINT32  = 4,
    GGUF_TYPE_INT32   = 5,
    GGUF_TYPE_FLOAT32 = 6,
    GGUF_TYPE_BOOL    = 7,
    GGUF_TYPE_STRING  = 8,
    GGUF_TYPE_ARRAY   = 9,
    GGUF_TYPE_UINT64  = 10,
    GGUF_TYPE_INT64   = 11,
    GGUF_TYPE_FLOAT64 = 12,
} gguf_value_type_t;

typedef struct {
    char    *key;
    int      type;
    union {
        uint8_t   u8;
        int8_t    i8;
        uint16_t  u16;
        int16_t   i16;
        uint32_t  u32;
        int32_t   i32;
        uint64_t  u64;
        int64_t   i64;
        float     f32;
        double    f64;
        bool      b;
        char     *str;
    } value;
} gguf_kv_t;

/* GGML tensor element types, as recorded in a GGUF tensor-info record.
 *
 * This is upstream ggml's numbering and is part of the file format, so the
 * values are fixed and must not be renumbered. Listed in full - including the
 * types eai_dtype_t cannot describe - because the point of the list is to be
 * able to tell "a type we can express" from "a type we cannot", and a partial
 * list cannot do that. 4 and 5 are the withdrawn Q4_2/Q4_3 and are absent
 * from the format. */
typedef enum {
    GGML_TYPE_F32     = 0,
    GGML_TYPE_F16     = 1,
    GGML_TYPE_Q4_0    = 2,
    GGML_TYPE_Q4_1    = 3,
    GGML_TYPE_Q5_0    = 6,
    GGML_TYPE_Q5_1    = 7,
    GGML_TYPE_Q8_0    = 8,
    GGML_TYPE_Q8_1    = 9,
    GGML_TYPE_Q2_K    = 10,
    GGML_TYPE_Q3_K    = 11,
    GGML_TYPE_Q4_K    = 12,
    GGML_TYPE_Q5_K    = 13,
    GGML_TYPE_Q6_K    = 14,
    GGML_TYPE_Q8_K    = 15,
    GGML_TYPE_IQ2_XXS = 16,
    GGML_TYPE_IQ2_XS  = 17,
    GGML_TYPE_IQ3_XXS = 18,
    GGML_TYPE_IQ1_S   = 19,
    GGML_TYPE_IQ4_NL  = 20,
    GGML_TYPE_IQ3_S   = 21,
    GGML_TYPE_IQ2_S   = 22,
    GGML_TYPE_IQ4_XS  = 23,
    GGML_TYPE_I8      = 24,
    GGML_TYPE_I16     = 25,
    GGML_TYPE_I32     = 26,
    GGML_TYPE_I64     = 27,
    GGML_TYPE_F64     = 28,
    GGML_TYPE_IQ1_M   = 29,
    GGML_TYPE_BF16    = 30,
} ggml_type_t;

typedef struct {
    char      *name;
    int        ndim;
    uint64_t   shape[EAI_TENSOR_MAX_DIMS];
    int        dtype;      /* GGML type */
    uint64_t   offset;     /* byte offset in data section */
    uint64_t   size;       /* total bytes */
} gguf_tensor_info_t;

typedef struct {
    uint32_t           magic;
    uint32_t           version;
    uint64_t           n_tensors;
    uint64_t           n_kv;
    gguf_kv_t         *kv;
    gguf_tensor_info_t *tensors;
    uint8_t           *data;        /* raw tensor data (mmap'd or loaded) */
    size_t             data_size;
} gguf_context_t;

eai_status_t eai_gguf_load(const char *path, gguf_context_t *ctx);
void         eai_gguf_free(gguf_context_t *ctx);
const char  *eai_gguf_get_str(const gguf_context_t *ctx, const char *key);
int          eai_gguf_get_int(const gguf_context_t *ctx, const char *key, int default_val);
eai_status_t eai_gguf_get_tensor(const gguf_context_t *ctx, const char *name, eai_tensor_t *tensor);

#ifdef __cplusplus
}
#endif

#endif /* EAI_GGUF_H */
