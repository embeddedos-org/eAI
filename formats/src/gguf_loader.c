// SPDX-License-Identifier: MIT
// GGUF format loader — parses GGUF v3 files

#include "eai/gguf.h"
#include "eai/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define LOG_MOD "gguf"

static char *read_string(FILE *fp)
{
    uint64_t len = 0;
    if (fread(&len, sizeof(len), 1, fp) != 1) return NULL;
    if (len > 65536) return NULL; /* sanity limit */

    char *str = (char *)calloc(1, (size_t)len + 1);
    if (!str) return NULL;
    if (fread(str, 1, (size_t)len, fp) != (size_t)len) { free(str); return NULL; }
    return str;
}

static eai_status_t read_kv(FILE *fp, gguf_kv_t *kv)
{
    kv->key = read_string(fp);
    if (!kv->key) return EAI_ERR_FORMAT;

    uint32_t type;
    if (fread(&type, sizeof(type), 1, fp) != 1) return EAI_ERR_FORMAT;
    kv->type = (int)type;

    switch (type) {
        case GGUF_TYPE_UINT8:   fread(&kv->value.u8,  1, 1, fp); break;
        case GGUF_TYPE_INT8:    fread(&kv->value.i8,  1, 1, fp); break;
        case GGUF_TYPE_UINT32:  fread(&kv->value.u32, 4, 1, fp); break;
        case GGUF_TYPE_INT32:   fread(&kv->value.i32, 4, 1, fp); break;
        case GGUF_TYPE_UINT64:  fread(&kv->value.u64, 8, 1, fp); break;
        case GGUF_TYPE_INT64:   fread(&kv->value.i64, 8, 1, fp); break;
        case GGUF_TYPE_FLOAT32: fread(&kv->value.f32, 4, 1, fp); break;
        case GGUF_TYPE_FLOAT64: fread(&kv->value.f64, 8, 1, fp); break;
        case GGUF_TYPE_BOOL:    fread(&kv->value.b,   1, 1, fp); break;
        case GGUF_TYPE_STRING:  kv->value.str = read_string(fp); break;
        case GGUF_TYPE_ARRAY: {
            /* Skip arrays for now — read type + count + data */
            uint32_t arr_type;
            uint64_t arr_count;
            fread(&arr_type, 4, 1, fp);
            fread(&arr_count, 8, 1, fp);
            /* Skip array data based on type */
            for (uint64_t i = 0; i < arr_count && i < 1000000; i++) {
                switch (arr_type) {
                    case GGUF_TYPE_UINT8:
                    case GGUF_TYPE_INT8:
                    case GGUF_TYPE_BOOL:    fseek(fp, 1, SEEK_CUR); break;
                    case GGUF_TYPE_UINT32:
                    case GGUF_TYPE_INT32:
                    case GGUF_TYPE_FLOAT32: fseek(fp, 4, SEEK_CUR); break;
                    case GGUF_TYPE_UINT64:
                    case GGUF_TYPE_INT64:
                    case GGUF_TYPE_FLOAT64: fseek(fp, 8, SEEK_CUR); break;
                    case GGUF_TYPE_STRING:  { char *s = read_string(fp); free(s); break; }
                    default: break;
                }
            }
            break;
        }
        default:
            EAI_LOG_WARN(LOG_MOD, "unknown GGUF KV type: %u for key '%s'", type, kv->key);
            break;
    }

    return EAI_OK;
}

eai_status_t eai_gguf_load(const char *path, gguf_context_t *ctx)
{
    if (!path || !ctx) return EAI_ERR_INVALID;
    memset(ctx, 0, sizeof(*ctx));

    FILE *fp = fopen(path, "rb");
    if (!fp) return EAI_ERR_IO;

    /* Read header */
    if (fread(&ctx->magic, 4, 1, fp) != 1 || ctx->magic != GGUF_MAGIC) {
        EAI_LOG_ERROR(LOG_MOD, "not a GGUF file: bad magic 0x%08X", ctx->magic);
        fclose(fp);
        return EAI_ERR_FORMAT;
    }

    if (fread(&ctx->version, 4, 1, fp) != 1) { fclose(fp); return EAI_ERR_FORMAT; }
    if (fread(&ctx->n_tensors, 8, 1, fp) != 1) { fclose(fp); return EAI_ERR_FORMAT; }
    if (fread(&ctx->n_kv, 8, 1, fp) != 1) { fclose(fp); return EAI_ERR_FORMAT; }

    EAI_LOG_INFO(LOG_MOD, "GGUF v%u: %lu KV pairs, %lu tensors",
                 ctx->version, (unsigned long)ctx->n_kv, (unsigned long)ctx->n_tensors);

    /* Read KV pairs */
    if (ctx->n_kv > 0 && ctx->n_kv < 100000) {
        ctx->kv = (gguf_kv_t *)calloc((size_t)ctx->n_kv, sizeof(gguf_kv_t));
        if (!ctx->kv) { fclose(fp); return EAI_ERR_NOMEM; }

        for (uint64_t i = 0; i < ctx->n_kv; i++) {
            eai_status_t st = read_kv(fp, &ctx->kv[i]);
            if (st != EAI_OK) {
                EAI_LOG_WARN(LOG_MOD, "failed to read KV %lu", (unsigned long)i);
                break;
            }
        }
    }

    /* Read tensor info */
    if (ctx->n_tensors > 0 && ctx->n_tensors < 100000) {
        ctx->tensors = (gguf_tensor_info_t *)calloc((size_t)ctx->n_tensors, sizeof(gguf_tensor_info_t));
        if (!ctx->tensors) { fclose(fp); eai_gguf_free(ctx); return EAI_ERR_NOMEM; }

        for (uint64_t i = 0; i < ctx->n_tensors; i++) {
            gguf_tensor_info_t *ti = &ctx->tensors[i];
            ti->name = read_string(fp);

            uint32_t ndim;
            fread(&ndim, 4, 1, fp);
            ti->ndim = (int)ndim;

            for (int d = 0; d < ti->ndim && d < EAI_TENSOR_MAX_DIMS; d++) {
                fread(&ti->shape[d], 8, 1, fp);
            }

            uint32_t dtype;
            fread(&dtype, 4, 1, fp);
            ti->dtype = (int)dtype;

            fread(&ti->offset, 8, 1, fp);
        }
    }

    /* Position reached after the header/KV/tensor-info sections. */
    long ftell_after_metadata = ftell(fp);

    /* Establish the true end of file first, so the data section can be
     * bounds-checked against it. */
    if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); eai_gguf_free(ctx); return EAI_ERR_IO; }
    long file_end = ftell(fp);
    if (file_end < 0) { fclose(fp); eai_gguf_free(ctx); return EAI_ERR_IO; }

    /* Data section starts after alignment to 32 bytes */
    long pos = ftell_after_metadata;
    if (pos < 0) { fclose(fp); eai_gguf_free(ctx); return EAI_ERR_IO; }
    long aligned_pos = (pos > LONG_MAX - 31) ? file_end : ((pos + 31) & ~31L);

    /* Rounding up to the alignment boundary can land past the end of a
     * truncated file. Computing file_end - data_start unguarded then yielded a
     * negative value that became ~1.8e19 when cast to size_t, and that value
     * reached malloc() and stayed in ctx->data_size for every later consumer to
     * read as a length. Treat "starts at or past EOF" as an empty data section. */
    if (aligned_pos >= file_end) {
        ctx->data = NULL;
        ctx->data_size = 0;
        fclose(fp);
        return EAI_OK;
    }

    long data_start = aligned_pos;
    ctx->data_size = (size_t)(file_end - data_start);

    ctx->data = (uint8_t *)malloc(ctx->data_size);
    if (!ctx->data) { ctx->data_size = 0; fclose(fp); eai_gguf_free(ctx); return EAI_ERR_NOMEM; }

    if (fseek(fp, data_start, SEEK_SET) != 0) {
        free(ctx->data); ctx->data = NULL; ctx->data_size = 0;
        fclose(fp); eai_gguf_free(ctx); return EAI_ERR_IO;
    }

    /* Keep data_size consistent with what was actually read, so a short read
     * cannot leave a length longer than the buffer's valid contents. */
    size_t got = fread(ctx->data, 1, ctx->data_size, fp);
    ctx->data_size = got;

    fclose(fp);
    return EAI_OK;
}

void eai_gguf_free(gguf_context_t *ctx)
{
    if (!ctx) return;
    if (ctx->kv) {
        for (uint64_t i = 0; i < ctx->n_kv; i++) {
            free(ctx->kv[i].key);
            if (ctx->kv[i].type == GGUF_TYPE_STRING) free(ctx->kv[i].value.str);
        }
        free(ctx->kv);
    }
    if (ctx->tensors) {
        for (uint64_t i = 0; i < ctx->n_tensors; i++) {
            free(ctx->tensors[i].name);
        }
        free(ctx->tensors);
    }
    free(ctx->data);
    memset(ctx, 0, sizeof(*ctx));
}

const char *eai_gguf_get_str(const gguf_context_t *ctx, const char *key)
{
    if (!ctx || !ctx->kv || !key) return NULL;
    for (uint64_t i = 0; i < ctx->n_kv; i++) {
        if (ctx->kv[i].key && strcmp(ctx->kv[i].key, key) == 0 &&
            ctx->kv[i].type == GGUF_TYPE_STRING) {
            return ctx->kv[i].value.str;
        }
    }
    return NULL;
}

int eai_gguf_get_int(const gguf_context_t *ctx, const char *key, int default_val)
{
    if (!ctx || !ctx->kv || !key) return default_val;
    for (uint64_t i = 0; i < ctx->n_kv; i++) {
        if (ctx->kv[i].key && strcmp(ctx->kv[i].key, key) == 0) {
            switch (ctx->kv[i].type) {
                case GGUF_TYPE_INT32:  return ctx->kv[i].value.i32;
                case GGUF_TYPE_UINT32: return (int)ctx->kv[i].value.u32;
                default: return default_val;
            }
        }
    }
    return default_val;
}

eai_status_t eai_gguf_get_tensor(const gguf_context_t *ctx, const char *name, eai_tensor_t *tensor)
{
    if (!ctx || !name || !tensor) return EAI_ERR_INVALID;

    for (uint64_t i = 0; i < ctx->n_tensors; i++) {
        if (ctx->tensors[i].name && strcmp(ctx->tensors[i].name, name) == 0) {
            gguf_tensor_info_t *ti = &ctx->tensors[i];

            memset(tensor, 0, sizeof(*tensor));
            tensor->dtype = EAI_DTYPE_F32; /* TODO: map GGML dtype */
            tensor->ndim = ti->ndim;
            for (int d = 0; d < ti->ndim; d++) {
                tensor->shape[d] = (int64_t)ti->shape[d];
            }

            if (ctx->data && ti->offset < ctx->data_size) {
                tensor->data = ctx->data + ti->offset;
                tensor->owns_data = false;
            }

            return EAI_OK;
        }
    }
    return EAI_ERR_NOT_FOUND;
}
