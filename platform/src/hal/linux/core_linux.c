// SPDX-License-Identifier: MIT
// HAL Core implementation for Linux

#include "eai/platform.h"
#include "eai/log.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define LOG_MOD "hal-linux-core"

#ifndef _WIN32
#ifdef __linux__

static eai_status_t linux_hal_init(eai_platform_t *plat)
{
    EAI_LOG_INFO(LOG_MOD, "Linux HAL core initialized");
    return EAI_OK;
}

static void linux_hal_shutdown(eai_platform_t *plat)
{
    EAI_LOG_INFO(LOG_MOD, "Linux HAL core shut down");
}

static eai_status_t linux_hal_get_device_info(eai_platform_t *plat, char *buf, size_t buf_size)
{
    FILE *fp = fopen("/etc/os-release", "r");
    if (!fp) {
        snprintf(buf, buf_size, "Linux (unknown)");
        return EAI_OK;
    }
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
            char *start = strchr(line, '"');
            if (start) {
                start++;
                char *end = strchr(start, '"');
                if (end) *end = '\0';
                snprintf(buf, buf_size, "%s", start);
                fclose(fp);
                return EAI_OK;
            }
        }
    }
    fclose(fp);
    snprintf(buf, buf_size, "Linux");
    return EAI_OK;
}

static eai_status_t linux_hal_get_os_name(eai_platform_t *plat, char *buf, size_t buf_size)
{
    snprintf(buf, buf_size, "Linux");
    return EAI_OK;
}

static eai_status_t linux_hal_get_cpu_temp(eai_platform_t *plat, float *temp_c)
{
    FILE *fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (!fp) return EAI_ERR_IO;
    int millideg;
    if (fscanf(fp, "%d", &millideg) != 1) {
        fclose(fp);
        return EAI_ERR_IO;
    }
    fclose(fp);
    *temp_c = millideg / 1000.0f;
    return EAI_OK;
}

static int linux_hal_get_cpu_count(eai_platform_t *plat)
{
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return (n > 0) ? (int)n : 1;
}

const eai_hal_core_ops_t eai_hal_linux_core_ops = {
    .init            = linux_hal_init,
    .shutdown        = linux_hal_shutdown,
    .get_device_info = linux_hal_get_device_info,
    .get_os_name     = linux_hal_get_os_name,
    .get_cpu_temp    = linux_hal_get_cpu_temp,
    .get_cpu_count   = linux_hal_get_cpu_count,
};

#endif /* __linux__ */
#endif /* !_WIN32 */
