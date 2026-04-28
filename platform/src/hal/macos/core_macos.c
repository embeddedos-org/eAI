// SPDX-License-Identifier: MIT
// HAL Core implementation for macOS/Darwin

#include "eai/platform.h"
#include "eai/log.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define LOG_MOD "hal-macos-core"

#if defined(__APPLE__)
#include <TargetConditionals.h>
#if !TARGET_OS_IPHONE

#include <sys/sysctl.h>
#include <mach/mach.h>

static eai_status_t macos_hal_init(eai_platform_t *plat)
{
    EAI_LOG_INFO(LOG_MOD, "macOS HAL core initialized");
    return EAI_OK;
}

static void macos_hal_shutdown(eai_platform_t *plat)
{
    EAI_LOG_INFO(LOG_MOD, "macOS HAL core shut down");
}

static eai_status_t macos_hal_get_device_info(eai_platform_t *plat, char *buf, size_t buf_size)
{
    char model[128] = {0};
    size_t model_len = sizeof(model);
    if (sysctlbyname("hw.model", model, &model_len, NULL, 0) != 0) {
        strncpy(model, "Mac", sizeof(model) - 1);
    }

    char osver[64] = {0};
    size_t ver_len = sizeof(osver);
    if (sysctlbyname("kern.osrelease", osver, &ver_len, NULL, 0) != 0) {
        strncpy(osver, "unknown", sizeof(osver) - 1);
    }

    snprintf(buf, buf_size, "macOS %s (%s)", osver, model);
    return EAI_OK;
}

static eai_status_t macos_hal_get_os_name(eai_platform_t *plat, char *buf, size_t buf_size)
{
    snprintf(buf, buf_size, "macOS");
    return EAI_OK;
}

static eai_status_t macos_hal_get_cpu_temp(eai_platform_t *plat, float *temp_c)
{
    /* SMC access requires IOKit — not implemented without framework dependency */
    (void)plat; (void)temp_c;
    return EAI_ERR_NOT_IMPLEMENTED;
}

static int macos_hal_get_cpu_count(eai_platform_t *plat)
{
    int count = 0;
    size_t len = sizeof(count);
    if (sysctlbyname("hw.ncpu", &count, &len, NULL, 0) != 0) return 1;
    return (count > 0) ? count : 1;
}

const eai_hal_core_ops_t eai_hal_macos_core_ops = {
    .init            = macos_hal_init,
    .shutdown        = macos_hal_shutdown,
    .get_device_info = macos_hal_get_device_info,
    .get_os_name     = macos_hal_get_os_name,
    .get_cpu_temp    = macos_hal_get_cpu_temp,
    .get_cpu_count   = macos_hal_get_cpu_count,
};

#endif /* !TARGET_OS_IPHONE */
#endif /* __APPLE__ */
