// SPDX-License-Identifier: MIT
// HAL Core implementation for Zephyr RTOS

#include "eai/platform.h"
#include "eai/log.h"
#include <string.h>

#define LOG_MOD "hal-zephyr-core"

#if defined(EAI_PLATFORM_ZEPHYR)

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

static eai_status_t zephyr_hal_init(eai_platform_t *plat)
{
    EAI_LOG_INFO(LOG_MOD, "Zephyr HAL core initialized");
    return EAI_OK;
}

static void zephyr_hal_shutdown(eai_platform_t *plat)
{
    EAI_LOG_INFO(LOG_MOD, "Zephyr HAL core shut down");
}

static eai_status_t zephyr_hal_get_device_info(eai_platform_t *plat, char *buf, size_t buf_size)
{
    snprintf(buf, buf_size, "Zephyr RTOS " STRINGIFY(BUILD_VERSION));
    return EAI_OK;
}

static eai_status_t zephyr_hal_get_os_name(eai_platform_t *plat, char *buf, size_t buf_size)
{
    snprintf(buf, buf_size, "Zephyr");
    return EAI_OK;
}

static eai_status_t zephyr_hal_get_cpu_temp(eai_platform_t *plat, float *temp_c)
{
    (void)plat; (void)temp_c;
    return EAI_ERR_NOT_IMPLEMENTED;
}

static int zephyr_hal_get_cpu_count(eai_platform_t *plat)
{
    (void)plat;
    return 1; /* Typically single-core on MCU targets */
}

const eai_hal_core_ops_t eai_hal_zephyr_core_ops = {
    .init            = zephyr_hal_init,
    .shutdown        = zephyr_hal_shutdown,
    .get_device_info = zephyr_hal_get_device_info,
    .get_os_name     = zephyr_hal_get_os_name,
    .get_cpu_temp    = zephyr_hal_get_cpu_temp,
    .get_cpu_count   = zephyr_hal_get_cpu_count,
};

#endif /* EAI_PLATFORM_ZEPHYR */
