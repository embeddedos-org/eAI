// SPDX-License-Identifier: MIT
// HAL Core implementation for FreeRTOS

#include "eai/platform.h"
#include "eai/log.h"
#include <string.h>
#include <stdio.h>

#define LOG_MOD "hal-freertos-core"

#if defined(EAI_PLATFORM_FREERTOS)

#include "FreeRTOS.h"
#include "task.h"

static eai_status_t freertos_hal_init(eai_platform_t *plat)
{
    EAI_LOG_INFO(LOG_MOD, "FreeRTOS HAL core initialized");
    return EAI_OK;
}

static void freertos_hal_shutdown(eai_platform_t *plat)
{
    EAI_LOG_INFO(LOG_MOD, "FreeRTOS HAL core shut down");
}

static eai_status_t freertos_hal_get_device_info(eai_platform_t *plat, char *buf, size_t buf_size)
{
    snprintf(buf, buf_size, "FreeRTOS v%s", tskKERNEL_VERSION_NUMBER);
    return EAI_OK;
}

static eai_status_t freertos_hal_get_os_name(eai_platform_t *plat, char *buf, size_t buf_size)
{
    snprintf(buf, buf_size, "FreeRTOS");
    return EAI_OK;
}

static eai_status_t freertos_hal_get_cpu_temp(eai_platform_t *plat, float *temp_c)
{
    (void)plat; (void)temp_c;
    return EAI_ERR_NOT_IMPLEMENTED;
}

static int freertos_hal_get_cpu_count(eai_platform_t *plat)
{
    (void)plat;
    return 1;
}

const eai_hal_core_ops_t eai_hal_freertos_core_ops = {
    .init            = freertos_hal_init,
    .shutdown        = freertos_hal_shutdown,
    .get_device_info = freertos_hal_get_device_info,
    .get_os_name     = freertos_hal_get_os_name,
    .get_cpu_temp    = freertos_hal_get_cpu_temp,
    .get_cpu_count   = freertos_hal_get_cpu_count,
};

#endif /* EAI_PLATFORM_FREERTOS */
