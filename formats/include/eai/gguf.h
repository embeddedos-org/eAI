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
