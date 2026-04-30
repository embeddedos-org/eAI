// SPDX-License-Identifier: MIT
// HAL Core implementation for bare-metal targets

#include "eai/platform.h"
#include <string.h>
#include <stdio.h>

#if defined(EAI_PLATFORM_BAREMETAL)

static eai_status_t baremetal_hal_init(eai_platform_t *plat)
{
    return EAI_OK;
}

static void baremetal_hal_shutdown(eai_platform_t *plat)
{
    /* Nothing to do */
}

static eai_status_t baremetal_hal_get_device_info(eai_platform_t *plat, char *buf, size_t buf_size)
{
    snprintf(buf, buf_size, "Bare-metal MCU");
    return EAI_OK;
}

static eai_status_t baremetal_hal_get_os_name(eai_platform_t *plat, char *buf, size_t buf_size)
{
    snprintf(buf, buf_size, "bare-metal");
    return EAI_OK;
}

static eai_status_t baremetal_hal_get_cpu_temp(eai_platform_t *plat, float *temp_c)
{
    (void)plat; (void)temp_c;
    return EAI_ERR_NOT_IMPLEMENTED;
}

static int baremetal_hal_get_cpu_count(eai_platform_t *plat)
{
    return 1;
}

const eai_hal_core_ops_t eai_hal_baremetal_core_ops = {
    .init            = baremetal_hal_init,
    .shutdown        = baremetal_hal_shutdown,
    .get_device_info = baremetal_hal_get_device_info,
    .get_os_name     = baremetal_hal_get_os_name,
    .get_cpu_temp    = baremetal_hal_get_cpu_temp,
    .get_cpu_count   = baremetal_hal_get_cpu_count,
};

#endif /* EAI_PLATFORM_BAREMETAL */
