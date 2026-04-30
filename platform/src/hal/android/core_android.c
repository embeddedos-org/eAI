// SPDX-License-Identifier: MIT
// HAL Core implementation for Android

#include "eai/platform.h"
#include "eai/log.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define LOG_MOD "hal-android-core"

#if defined(__ANDROID__)

#include <sys/system_properties.h>

static eai_status_t android_hal_init(eai_platform_t *plat)
{
    EAI_LOG_INFO(LOG_MOD, "Android HAL core initialized");
    return EAI_OK;
}

static void android_hal_shutdown(eai_platform_t *plat)
{
    EAI_LOG_INFO(LOG_MOD, "Android HAL core shut down");
}

static eai_status_t android_hal_get_device_info(eai_platform_t *plat, char *buf, size_t buf_size)
{
    char model[128] = "unknown";
    char version[32] = "?";

    __system_property_get("ro.product.model", model);
    __system_property_get("ro.build.version.release", version);

    snprintf(buf, buf_size, "Android %s (%s)", version, model);
    return EAI_OK;
}

static eai_status_t android_hal_get_os_name(eai_platform_t *plat, char *buf, size_t buf_size)
{
    snprintf(buf, buf_size, "Android");
    return EAI_OK;
}

static eai_status_t android_hal_get_cpu_temp(eai_platform_t *plat, float *temp_c)
{
    FILE *fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (!fp) return EAI_ERR_IO;
    int millideg;
    if (fscanf(fp, "%d", &millideg) != 1) { fclose(fp); return EAI_ERR_IO; }
    fclose(fp);
    *temp_c = millideg / 1000.0f;
    return EAI_OK;
}

static int android_hal_get_cpu_count(eai_platform_t *plat)
{
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return (n > 0) ? (int)n : 1;
}

const eai_hal_core_ops_t eai_hal_android_core_ops = {
    .init            = android_hal_init,
    .shutdown        = android_hal_shutdown,
    .get_device_info = android_hal_get_device_info,
    .get_os_name     = android_hal_get_os_name,
    .get_cpu_temp    = android_hal_get_cpu_temp,
    .get_cpu_count   = android_hal_get_cpu_count,
};

#endif /* __ANDROID__ */
