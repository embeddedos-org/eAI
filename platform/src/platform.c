// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// ISO/IEC 25000 | ISO/IEC/IEEE 15288:2023

#include "eai/platform.h"
#include "eai/log.h"
#include <string.h>

#define LOG_MOD "platform"

eai_status_t eai_platform_init(eai_platform_t *plat, const eai_platform_ops_t *ops)
{
    if (!plat || !ops) return EAI_ERR_INVALID;
    memset(plat, 0, sizeof(*plat));
    plat->ops = ops;
    plat->hal = NULL;
    plat->initialized = false;

    eai_status_t s = ops->init(plat);
    if (s == EAI_OK) {
        plat->initialized = true;
        EAI_LOG_INFO(LOG_MOD, "initialized platform: %s", ops->name);
    }
    return s;
}

eai_status_t eai_platform_init_hal(eai_platform_t *plat, const eai_platform_hal_t *hal)
{
    if (!plat || !hal) return EAI_ERR_INVALID;
    if (!hal->core || !hal->memory) return EAI_ERR_INVALID;

    memset(plat, 0, sizeof(*plat));
    plat->hal = hal;
    plat->ops = NULL;
    plat->initialized = false;

    if (hal->core->init) {
        eai_status_t s = hal->core->init(plat);
        if (s != EAI_OK) return s;
    }

    plat->initialized = true;
    EAI_LOG_INFO(LOG_MOD, "initialized HAL platform: %s", hal->name ? hal->name : "unknown");
    return EAI_OK;
}

eai_status_t eai_platform_detect(eai_platform_t *plat)
{
    if (!plat) return EAI_ERR_INVALID;

#ifdef _WIN32
    return eai_platform_init(plat, &eai_platform_windows_ops);
#elif defined(__APPLE__)
    /* macOS / iOS — for now fall back to linux ops until macOS adapter exists */
    return eai_platform_init(plat, &eai_platform_linux_ops);
#elif defined(__ANDROID__)
    /* Android — for now fall back to linux ops until Android adapter exists */
    return eai_platform_init(plat, &eai_platform_linux_ops);
#elif defined(EAI_PLATFORM_EOS_ENABLED)
    return eai_platform_init(plat, &eai_platform_eos_ops);
#else
    /* Linux: check if running inside a container first */
    {
        if (eai_platform_is_container()) {
            return eai_platform_init(plat, &eai_platform_container_ops);
        }
    }
    return eai_platform_init(plat, &eai_platform_linux_ops);
#endif
}

void eai_platform_shutdown(eai_platform_t *plat)
{
    if (!plat || !plat->initialized) return;

    if (plat->ops && plat->ops->shutdown) {
        plat->ops->shutdown(plat);
    } else if (plat->hal && plat->hal->core && plat->hal->core->shutdown) {
        plat->hal->core->shutdown(plat);
    }

    plat->initialized = false;
    EAI_LOG_INFO(LOG_MOD, "platform shut down");
}
