// SPDX-License-Identifier: MIT
// Qualcomm QNN backend stub

#include "eai/accel.h"

#if defined(EAI_ACCEL_QNN)

#include "eai/log.h"
#include <string.h>
#define LOG_MOD "accel-qnn"

static eai_status_t qnn_init(eai_accel_backend_t *b, const eai_kv_t *cfg, int n)
{
    (void)b; (void)cfg; (void)n;
    EAI_LOG_INFO(LOG_MOD, "QNN backend initialized (stub)");
    return EAI_OK;
}

static bool qnn_can_handle(eai_accel_backend_t *b, const eai_op_t *op)
{
    (void)b;
    return (op && (op->type == EAI_OP_MATMUL || op->type == EAI_OP_CONV2D ||
                   op->type == EAI_OP_ATTENTION));
}

static eai_status_t qnn_prepare(eai_accel_backend_t *b, const eai_compute_graph_t *g)
{
    (void)b; (void)g;
    return EAI_ERR_NOT_IMPLEMENTED;
}

static eai_status_t qnn_execute(eai_accel_backend_t *b, const eai_compute_graph_t *g)
{
    (void)b; (void)g;
    return EAI_ERR_NOT_IMPLEMENTED;
}

static eai_status_t qnn_get_info(eai_accel_backend_t *b, eai_accel_backend_info_t *info)
{
    (void)b;
    if (!info) return EAI_ERR_INVALID;
    strncpy(info->name, "Qualcomm QNN", sizeof(info->name) - 1);
    info->type = EAI_BACKEND_QNN;
    return EAI_OK;
}

static void qnn_shutdown(eai_accel_backend_t *b) { (void)b; }

const eai_accel_backend_ops_t eai_accel_qnn_ops = {
    .name = "qnn", .type = EAI_BACKEND_QNN,
    .init = qnn_init, .can_handle = qnn_can_handle,
    .prepare = qnn_prepare, .execute = qnn_execute,
    .get_info = qnn_get_info, .shutdown = qnn_shutdown,
};

#endif /* EAI_ACCEL_QNN */
