// SPDX-License-Identifier: MIT
// Vulkan Compute backend stub

#include "eai/accel.h"

#if defined(EAI_ACCEL_VULKAN)

#include "eai/log.h"
#define LOG_MOD "accel-vulkan"

static eai_status_t vulkan_init(eai_accel_backend_t *b, const eai_kv_t *cfg, int n)
{
    (void)b; (void)cfg; (void)n;
    EAI_LOG_INFO(LOG_MOD, "Vulkan backend initialized (stub)");
    return EAI_OK;
}

static bool vulkan_can_handle(eai_accel_backend_t *b, const eai_op_t *op)
{
    (void)b;
    /* Vulkan can accelerate matmul, conv2d, attention */
    return (op && (op->type == EAI_OP_MATMUL || op->type == EAI_OP_CONV2D ||
                   op->type == EAI_OP_ATTENTION));
}

static eai_status_t vulkan_prepare(eai_accel_backend_t *b, const eai_compute_graph_t *g)
{
    (void)b; (void)g;
    return EAI_ERR_NOT_IMPLEMENTED;
}

static eai_status_t vulkan_execute(eai_accel_backend_t *b, const eai_compute_graph_t *g)
{
    (void)b; (void)g;
    return EAI_ERR_NOT_IMPLEMENTED;
}

static eai_status_t vulkan_get_info(eai_accel_backend_t *b, eai_accel_backend_info_t *info)
{
    (void)b;
    if (!info) return EAI_ERR_INVALID;
    strncpy(info->name, "Vulkan Compute", sizeof(info->name) - 1);
    info->type = EAI_BACKEND_VULKAN;
    return EAI_OK;
}

static void vulkan_shutdown(eai_accel_backend_t *b) { (void)b; }

const eai_accel_backend_ops_t eai_accel_vulkan_ops = {
    .name = "vulkan", .type = EAI_BACKEND_VULKAN,
    .init = vulkan_init, .can_handle = vulkan_can_handle,
    .prepare = vulkan_prepare, .execute = vulkan_execute,
    .get_info = vulkan_get_info, .shutdown = vulkan_shutdown,
};

#endif /* EAI_ACCEL_VULKAN */
