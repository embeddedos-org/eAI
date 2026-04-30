// SPDX-License-Identifier: MIT
// Format registry — auto-detect and load model files

#include "eai/gguf.h"
#include "eai/onnx.h"
#include "eai/log.h"
#include <stdio.h>
#include <string.h>

#define LOG_MOD "format"

typedef enum {
    EAI_FORMAT_UNKNOWN = 0,
    EAI_FORMAT_GGUF    = 1,
    EAI_FORMAT_ONNX    = 2,
} eai_format_t;

eai_format_t eai_format_detect(const char *path)
{
    if (!path) return EAI_FORMAT_UNKNOWN;

    FILE *fp = fopen(path, "rb");
    if (!fp) return EAI_FORMAT_UNKNOWN;

    uint8_t magic[4];
    if (fread(magic, 1, 4, fp) != 4) { fclose(fp); return EAI_FORMAT_UNKNOWN; }
    fclose(fp);

    /* GGUF magic: "GGUF" = 0x47 0x47 0x55 0x46 */
    if (magic[0] == 0x47 && magic[1] == 0x47 && magic[2] == 0x55 && magic[3] == 0x46)
        return EAI_FORMAT_GGUF;

    /* ONNX protobuf: typically starts with field 1 varint (0x08) */
    if (magic[0] == 0x08)
        return EAI_FORMAT_ONNX;

    /* Check file extension as fallback */
    const char *ext = strrchr(path, '.');
    if (ext) {
        if (strcmp(ext, ".gguf") == 0) return EAI_FORMAT_GGUF;
        if (strcmp(ext, ".onnx") == 0) return EAI_FORMAT_ONNX;
    }

    return EAI_FORMAT_UNKNOWN;
}

eai_status_t eai_format_load(const char *path, void *ctx, eai_format_t *out_format)
{
    eai_format_t fmt = eai_format_detect(path);
    if (out_format) *out_format = fmt;

    switch (fmt) {
        case EAI_FORMAT_GGUF:
            EAI_LOG_INFO(LOG_MOD, "loading GGUF: %s", path);
            return eai_gguf_load(path, (gguf_context_t *)ctx);
        case EAI_FORMAT_ONNX:
            EAI_LOG_INFO(LOG_MOD, "loading ONNX: %s", path);
            return eai_onnx_load(path, (onnx_context_t *)ctx);
        default:
            EAI_LOG_ERROR(LOG_MOD, "unknown format: %s", path);
            return EAI_ERR_FORMAT;
    }
}
