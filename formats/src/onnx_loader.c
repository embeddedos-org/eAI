// SPDX-License-Identifier: MIT
// Minimal ONNX loader — parses protobuf wire format for ONNX ModelProto

#include "eai/onnx.h"
#include "eai/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_MOD "onnx"

/* Minimal protobuf wire format parser */
typedef enum {
    PB_WIRE_VARINT  = 0,
    PB_WIRE_64BIT   = 1,
    PB_WIRE_LEN     = 2,
    PB_WIRE_32BIT   = 5,
} pb_wire_type_t;

static uint64_t pb_read_varint(const uint8_t *data, size_t len, size_t *pos)
{
    uint64_t val = 0;
    int shift = 0;
    while (*pos < len) {
        uint8_t byte = data[(*pos)++];
        val |= ((uint64_t)(byte & 0x7F)) << shift;
        if (!(byte & 0x80)) break;
        shift += 7;
        if (shift >= 64) break;
    }
    return val;
}

static char *pb_read_string(const uint8_t *data, size_t len, size_t *pos)
{
    uint64_t slen = pb_read_varint(data, len, pos);
    if (*pos + slen > len || slen > 65536) return NULL;
    char *str = (char *)calloc(1, (size_t)slen + 1);
    if (str) memcpy(str, data + *pos, (size_t)slen);
    *pos += (size_t)slen;
    return str;
}

eai_status_t eai_onnx_load(const char *path, onnx_context_t *ctx)
{
    if (!path || !ctx) return EAI_ERR_INVALID;
    memset(ctx, 0, sizeof(*ctx));

    FILE *fp = fopen(path, "rb");
    if (!fp) return EAI_ERR_IO;

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size <= 0 || file_size > 500 * 1024 * 1024) {
        fclose(fp);
        return EAI_ERR_FORMAT;
    }

    uint8_t *data = (uint8_t *)malloc((size_t)file_size);
    if (!data) { fclose(fp); return EAI_ERR_NOMEM; }
    fread(data, 1, (size_t)file_size, fp);
    fclose(fp);

    /* Parse top-level ModelProto fields */
    size_t pos = 0;
    size_t len = (size_t)file_size;

    while (pos < len) {
        uint64_t tag = pb_read_varint(data, len, &pos);
        uint32_t field = (uint32_t)(tag >> 3);
        uint32_t wire = (uint32_t)(tag & 0x07);

        switch (wire) {
            case PB_WIRE_VARINT: {
                uint64_t val = pb_read_varint(data, len, &pos);
                if (field == 1) ctx->ir_version = (int64_t)val;
                break;
            }
            case PB_WIRE_LEN: {
                uint64_t slen = pb_read_varint(data, len, &pos);
                if (pos + slen > len) { free(data); return EAI_ERR_FORMAT; }

                if (field == 7) {
                    /* graph — nested message, skip for now */
                    EAI_LOG_INFO(LOG_MOD, "ONNX graph found at offset %lu, size %lu",
                                 (unsigned long)pos, (unsigned long)slen);
                }

                pos += (size_t)slen;
                break;
            }
            case PB_WIRE_32BIT: pos += 4; break;
            case PB_WIRE_64BIT: pos += 8; break;
            default: pos = len; break; /* unknown wire type, bail */
        }
    }

    free(data);
    EAI_LOG_INFO(LOG_MOD, "ONNX model loaded: IR version %ld", (long)ctx->ir_version);
    return EAI_OK;
}

void eai_onnx_free(onnx_context_t *ctx)
{
    if (!ctx) return;
    free(ctx->model_name);
    if (ctx->nodes) {
        for (int i = 0; i < ctx->node_count; i++) {
            free(ctx->nodes[i].name);
            for (int j = 0; j < ctx->nodes[i].input_count; j++)
                free(ctx->nodes[i].input_names[j]);
            free(ctx->nodes[i].input_names);
            for (int j = 0; j < ctx->nodes[i].output_count; j++)
                free(ctx->nodes[i].output_names[j]);
            free(ctx->nodes[i].output_names);
        }
        free(ctx->nodes);
    }
    if (ctx->initializers) {
        for (int i = 0; i < ctx->initializer_count; i++) {
            free(ctx->initializers[i].name);
            free(ctx->initializers[i].data);
        }
        free(ctx->initializers);
    }
    memset(ctx, 0, sizeof(*ctx));
}

eai_status_t eai_onnx_to_graph(const onnx_context_t *ctx, eai_compute_graph_t *graph)
{
    if (!ctx || !graph) return EAI_ERR_INVALID;
    memset(graph, 0, sizeof(*graph));
    /* Placeholder — full graph building requires deeper protobuf parsing */
    return EAI_ERR_NOT_IMPLEMENTED;
}
