// SPDX-License-Identifier: MIT
// HAL Core implementation for containers (extends POSIX)

#include "eai/platform.h"
#include "eai/log.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define LOG_MOD "hal-container-core"

#if !defined(_WIN32)

#include <sys/stat.h>

static eai_status_t container_hal_init(eai_platform_t *plat)
{
    EAI_LOG_INFO(LOG_MOD, "Container HAL core initialized");
    return EAI_OK;
}

static void container_hal_shutdown(eai_platform_t *plat)
{
    EAI_LOG_INFO(LOG_MOD, "Container HAL core shut down");
}

static eai_status_t container_hal_get_device_info(eai_platform_t *plat, char *buf, size_t buf_size)
{
    struct stat st;
    const char *runtime = "unknown";
    if (stat("/.dockerenv", &st) == 0)
        runtime = "Docker";
    else if (stat("/run/.containerenv", &st) == 0)
        runtime = "Podman/OCI";
    snprintf(buf, buf_size, "Container (%s)", runtime);
    return EAI_OK;
}

static eai_status_t container_hal_get_os_name(eai_platform_t *plat, char *buf, size_t buf_size)
{
    snprintf(buf, buf_size, "Linux (container)");
    return EAI_OK;
}

static eai_status_t container_hal_get_cpu_temp(eai_platform_t *plat, float *temp_c)
{
    (void)plat; (void)temp_c;
    return EAI_ERR_UNSUPPORTED;
}

static int container_hal_get_cpu_count(eai_platform_t *plat)
{
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return (n > 0) ? (int)n : 1;
}

const eai_hal_core_ops_t eai_hal_container_core_ops = {
    .init            = container_hal_init,
    .shutdown        = container_hal_shutdown,
    .get_device_info = container_hal_get_device_info,
    .get_os_name     = container_hal_get_os_name,
    .get_cpu_temp    = container_hal_get_cpu_temp,
    .get_cpu_count   = container_hal_get_cpu_count,
};

#endif /* !_WIN32 */
