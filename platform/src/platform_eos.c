// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// ISO/IEC 25000 | ISO/IEC/IEEE 15288:2023

#include "eai/platform.h"
#include "eai/log.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#define LOG_MOD "plat-eos"

#if defined(EAI_PLATFORM_EOS_ENABLED)

/* Real EoS SDK integration when available */
#if defined(EOS_SDK_AVAILABLE)
#include <eos/system.h>
#include <eos/gpio.h>
#include <eos/memory.h>
#include <eos/thermal.h>

static eai_status_t eos_init(eai_platform_t *plat) {
    int rc = eos_system_init();
    if (rc != 0) {
        EAI_LOG_ERROR(LOG_MOD, "EoS SDK init failed: %d", rc);
        return EAI_ERR_PLATFORM;
    }
    EAI_LOG_INFO(LOG_MOD, "EoS platform adapter initialized (SDK)");
    return EAI_OK;
}

static eai_status_t eos_get_device_info(eai_platform_t *plat, char *buf, size_t buf_size) {
    const char *model = eos_system_get_model();
    const char *fw_ver = eos_system_get_firmware_version();
    snprintf(buf, buf_size, "EoS %s (fw %s)", model ? model : "unknown", fw_ver ? fw_ver : "?");
    return EAI_OK;
}

static eai_status_t eos_read_gpio(eai_platform_t *plat, int pin, int *value) {
    if (!value) return EAI_ERR_INVALID;
    int rc = eos_gpio_read(pin, value);
    return (rc == 0) ? EAI_OK : EAI_ERR_IO;
}

static eai_status_t eos_write_gpio(eai_platform_t *plat, int pin, int value) {
    int rc = eos_gpio_write(pin, value);
    return (rc == 0) ? EAI_OK : EAI_ERR_IO;
}

static eai_status_t eos_get_memory_info(eai_platform_t *plat, uint64_t *total, uint64_t *available) {
    eos_memory_info_t info;
    int rc = eos_memory_get_info(&info);
    if (rc != 0) return EAI_ERR_IO;
    if (total)     *total     = info.total_bytes;
    if (available) *available = info.available_bytes;
    return EAI_OK;
}

static eai_status_t eos_get_cpu_temp(eai_platform_t *plat, float *temp_c) {
    if (!temp_c) return EAI_ERR_INVALID;
    int rc = eos_thermal_get_cpu_temp(temp_c);
    return (rc == 0) ? EAI_OK : EAI_ERR_IO;
}

static void eos_shutdown(eai_platform_t *plat) {
    eos_system_shutdown();
    EAI_LOG_INFO(LOG_MOD, "EoS platform shut down (SDK)");
}

#else /* No EoS SDK — stub implementation with best-effort values */

static eai_status_t eos_init(eai_platform_t *plat) {
    (void)plat;
    EAI_LOG_INFO(LOG_MOD, "EoS platform adapter initialized (stub)");
    return EAI_OK;
}

static eai_status_t eos_get_device_info(eai_platform_t *plat, char *buf, size_t buf_size) {
    (void)plat;
    snprintf(buf, buf_size, "EoS Device (embedded, stub)");
    return EAI_OK;
}

static eai_status_t eos_read_gpio(eai_platform_t *plat, int pin, int *value) {
    (void)plat; (void)pin;
    if (!value) return EAI_ERR_INVALID;
    *value = 0;
    return EAI_ERR_NOT_IMPLEMENTED;
}

static eai_status_t eos_write_gpio(eai_platform_t *plat, int pin, int value) {
    (void)plat; (void)pin; (void)value;
    return EAI_ERR_NOT_IMPLEMENTED;
}

static eai_status_t eos_get_memory_info(eai_platform_t *plat, uint64_t *total, uint64_t *available) {
    (void)plat;
    /* Default embedded memory profile — override with real SDK */
    if (total) *total = 256 * 1024 * 1024ULL;
    if (available) *available = 128 * 1024 * 1024ULL;
    return EAI_OK;
}

static eai_status_t eos_get_cpu_temp(eai_platform_t *plat, float *temp_c) {
    (void)plat;
    if (!temp_c) return EAI_ERR_INVALID;
    *temp_c = 0.0f;
    return EAI_ERR_NOT_IMPLEMENTED;
}

static void eos_shutdown(eai_platform_t *plat) {
    (void)plat;
    EAI_LOG_INFO(LOG_MOD, "EoS platform shut down (stub)");
}

#endif /* EOS_SDK_AVAILABLE */

const eai_platform_ops_t eai_platform_eos_ops = {
    .name            = "eos",
    .init            = eos_init,
    .get_device_info = eos_get_device_info,
    .read_gpio       = eos_read_gpio,
    .write_gpio      = eos_write_gpio,
    .get_memory_info = eos_get_memory_info,
    .get_cpu_temp    = eos_get_cpu_temp,
    .shutdown        = eos_shutdown,
};

#endif /* EAI_PLATFORM_EOS_ENABLED */
