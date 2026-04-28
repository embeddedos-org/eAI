// SPDX-License-Identifier: MIT
// Accelerator backend registry and dispatch

#include "eai/accel.h"
#include "eai/log.h"
#include <string.h>
#include <stdlib.h>

#define LOG_MOD "accel"

static const eai_accel_backend_ops_t *registry[EAI_MAX_ACCEL_BACKENDS];
static int registry_count = 0;

eai_status_t eai_accel_register(const eai_accel_backend_ops_t *ops)
{
    if (!ops || !ops->name) return EAI_ERR_INVALID;
    if (registry_count >= EAI_MAX_ACCEL_BACKENDS) return EAI_ERR_NOMEM;

    /* Check for duplicates */
    for (int i = 0; i < registry_count; i++) {
        if (strcmp(registry[i]->name, ops->name) == 0)
            return EAI_ERR_INVALID;
    }

    registry[registry_count++] = ops;
    EAI_LOG_INFO(LOG_MOD, "registered accelerator backend: %s", ops->name);
    return EAI_OK;
}

const eai_accel_backend_ops_t *eai_accel_find(const char *name)
{
    if (!name) return NULL;
    for (int i = 0; i < registry_count; i++) {
        if (strcmp(registry[i]->name, name) == 0)
            return registry[i];
    }
    return NULL;
}

const eai_accel_backend_ops_t *eai_accel_find_by_type(eai_backend_type_t type)
{
    for (int i = 0; i < registry_count; i++) {
        if (registry[i]->type == type)
            return registry[i];
    }
    return NULL;
}

int eai_accel_list(const eai_accel_backend_ops_t **out, int max)
{
    int count = (registry_count < max) ? registry_count : max;
    for (int i = 0; i < count; i++) {
        out[i] = registry[i];
    }
    return count;
}

eai_status_t eai_accel_dispatch(eai_compute_graph_t *graph)
{
    if (!graph || !graph->ops || graph->op_count <= 0)
        return EAI_ERR_INVALID;

    /* Find CPU backend as fallback */
    const eai_accel_backend_ops_t *cpu_ops = eai_accel_find_by_type(EAI_BACKEND_CPU);
    if (!cpu_ops) {
        EAI_LOG_ERROR(LOG_MOD, "no CPU backend registered — cannot dispatch");
        return EAI_ERR_DELEGATE;
    }

    /* For each op, try to find the best accelerator */
    eai_accel_backend_t cpu_backend;
    memset(&cpu_backend, 0, sizeof(cpu_backend));
    cpu_backend.ops = cpu_ops;

    eai_status_t st = cpu_ops->init(&cpu_backend, NULL, 0);
    if (st != EAI_OK) return st;

    /* Simple dispatch: try non-CPU backends first, fallback to CPU */
    for (int i = 0; i < graph->op_count; i++) {
        bool handled = false;

        for (int j = 0; j < registry_count; j++) {
            if (registry[j]->type == EAI_BACKEND_CPU) continue;

            eai_accel_backend_t backend;
            memset(&backend, 0, sizeof(backend));
            backend.ops = registry[j];

            if (registry[j]->can_handle && registry[j]->can_handle(&backend, &graph->ops[i])) {
                /* Accelerated path — for now just note it */
                handled = true;
                break;
            }
        }

        if (!handled) {
            /* CPU fallback */
            (void)cpu_ops->can_handle;
        }
    }

    /* Execute on CPU backend (full graph for now) */
    st = cpu_ops->execute(&cpu_backend, graph);

    cpu_ops->shutdown(&cpu_backend);
    return st;
}

void eai_compute_graph_destroy(eai_compute_graph_t *graph)
{
    if (!graph) return;
    if (graph->tensors) {
        for (int i = 0; i < graph->tensor_count; i++) {
            eai_tensor_destroy(&graph->tensors[i]);
        }
        free(graph->tensors);
    }
    if (graph->ops) {
        for (int i = 0; i < graph->op_count; i++) {
            free(graph->ops[i].input_indices);
            free(graph->ops[i].output_indices);
        }
        free(graph->ops);
    }
    memset(graph, 0, sizeof(*graph));
}

void eai_accel_reset(void)
{
    registry_count = 0;
    memset(registry, 0, sizeof(registry));
}
