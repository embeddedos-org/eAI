// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// Platform-aware eAI API implementation

#include "eai/eai_api.h"
#include "eai/platform.h"
#include <string.h>

eai_status_t eai_api_platform_detect(eai_platform_t *plat)
{
    return eai_platform_detect(plat);
}

eai_status_t eai_api_platform_get_info(eai_platform_t *plat, char *buf, size_t buf_size)
{
    if (!plat || !buf || buf_size == 0) return EAI_ERR_INVALID;
    if (!plat->initialized) return EAI_ERR_RUNTIME;
    if (plat->ops && plat->ops->get_device_info) {
        return plat->ops->get_device_info(plat, buf, buf_size);
    }
    if (plat->hal && plat->hal->core && plat->hal->core->get_device_info) {
        return plat->hal->core->get_device_info(plat, buf, buf_size);
    }
    return EAI_ERR_UNSUPPORTED;
}

eai_status_t eai_api_platform_get_memory(eai_platform_t *plat,
                                          uint64_t *total, uint64_t *available)
{
    if (!plat) return EAI_ERR_INVALID;
    if (!plat->initialized) return EAI_ERR_RUNTIME;
    if (plat->ops && plat->ops->get_memory_info) {
        return plat->ops->get_memory_info(plat, total, available);
    }
    if (plat->hal && plat->hal->memory && plat->hal->memory->get_memory_info) {
        return plat->hal->memory->get_memory_info(plat, total, available);
    }
    return EAI_ERR_UNSUPPORTED;
}

void eai_api_platform_shutdown(eai_platform_t *plat)
{
    eai_platform_shutdown(plat);
}

int eai_api_accel_count(void)
{
    return 0; /* Extended when accel module is linked */
}

eai_status_t eai_api_accel_get_name(int index, char *buf, size_t buf_size)
{
    (void)index;
    if (!buf || buf_size == 0) return EAI_ERR_INVALID;
    buf[0] = '\0';
    return EAI_ERR_NOT_IMPLEMENTED;
}
