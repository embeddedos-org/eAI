// SPDX-License-Identifier: MIT
// CoreML / Metal backend stub for Apple platforms

#include "eai/accel.h"

#if defined(EAI_ACCEL_COREML) && defined(__APPLE__)

#include "eai/log.h"
#import <Foundation/Foundation.h>

#define LOG_MOD "accel-coreml"

static eai_status_t coreml_init(eai_accel_backend_t *b, const eai_kv_t *cfg, int n)
{
    (void)b; (void)cfg; (void)n;
    EAI_LOG_INFO(LOG_MOD, "CoreML backend initialized (stub)");
    return EAI_OK;
}

static bool coreml_can_handle(eai_accel_backend_t *b, const eai_op_t *op)
{
    (void)b;
    return (op && (op->type == EAI_OP_MATMUL || op->type == EAI_OP_CONV2D));
}

static eai_status_t coreml_prepare(eai_accel_backend_t *b, const eai_compute_graph_t *g)
{
    (void)b; (void)g;
    return EAI_ERR_NOT_IMPLEMENTED;
}

static eai_status_t coreml_execute(eai_accel_backend_t *b, const eai_compute_graph_t *g)
{
    (void)b; (void)g;
    return EAI_ERR_NOT_IMPLEMENTED;
}

static eai_status_t coreml_get_info(eai_accel_backend_t *b, eai_accel_backend_info_t *info)
{
    (void)b;
    if (!info) return EAI_ERR_INVALID;
    strncpy(info->name, "CoreML/Metal", sizeof(info->name) - 1);
    info->type = EAI_BACKEND_COREML;
    return EAI_OK;
}

static void coreml_shutdown(eai_accel_backend_t *b) { (void)b; }

const eai_accel_backend_ops_t eai_accel_coreml_ops = {
    .name = "coreml", .type = EAI_BACKEND_COREML,
    .init = coreml_init, .can_handle = coreml_can_handle,
    .prepare = coreml_prepare, .execute = coreml_execute,
    .get_info = coreml_get_info, .shutdown = coreml_shutdown,
};

#endif
