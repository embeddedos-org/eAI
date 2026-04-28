// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// ISO/IEC 25000 | ISO/IEC/IEEE 15288:2023

#include "eai/platform.h"
#include "eai/log.h"
#include <string.h>
#include <stdio.h>

#define LOG_MOD "plat-container"

#if !defined(_WIN32)

#include <unistd.h>
#include <sys/stat.h>
#ifdef __linux__
#include <sys/sysinfo.h>
#endif

bool eai_platform_is_container(void)
{
    struct stat st;
    /* Docker */
    if (stat("/.dockerenv", &st) == 0) return true;
    /* Podman / other OCI runtimes */
    if (stat("/run/.containerenv", &st) == 0) return true;
    /* cgroup-based detection: check for container cgroup paths */
    FILE *fp = fopen("/proc/1/cgroup", "r");
    if (fp) {
        char line[512];
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "docker") || strstr(line, "kubepods") ||
                strstr(line, "lxc") || strstr(line, "containerd")) {
                fclose(fp);
                return true;
            }
        }
        fclose(fp);
    }
    return false;
}

static eai_status_t container_init(eai_platform_t *plat)
{
    EAI_LOG_INFO(LOG_MOD, "Container platform adapter initialized");
    return EAI_OK;
}

static eai_status_t container_get_device_info(eai_platform_t *plat, char *buf, size_t buf_size)
{
    /* Try to identify the container runtime */
    struct stat st;
    const char *runtime = "unknown";
    if (stat("/.dockerenv", &st) == 0) {
        runtime = "Docker";
    } else if (stat("/run/.containerenv", &st) == 0) {
        runtime = "Podman/OCI";
    }
    snprintf(buf, buf_size, "Container (%s)", runtime);
    return EAI_OK;
}

static eai_status_t container_read_gpio(eai_platform_t *plat, int pin, int *value)
{
    (void)plat; (void)pin; (void)value;
    return EAI_ERR_UNSUPPORTED;
}

static eai_status_t container_write_gpio(eai_platform_t *plat, int pin, int value)
{
    (void)plat; (void)pin; (void)value;
    return EAI_ERR_UNSUPPORTED;
}

static eai_status_t container_get_memory_info(eai_platform_t *plat,
                                               uint64_t *total, uint64_t *available)
{
    /* Try cgroup v2 memory limits first */
    FILE *fp = fopen("/sys/fs/cgroup/memory.max", "r");
    if (fp) {
        char buf[64];
        if (fgets(buf, sizeof(buf), fp)) {
            if (strncmp(buf, "max", 3) != 0) {
                uint64_t limit = 0;
                if (sscanf(buf, "%lu", (unsigned long *)&limit) == 1) {
                    if (total) *total = limit;
                    fclose(fp);
                    /* Try to get current usage for "available" */
                    fp = fopen("/sys/fs/cgroup/memory.current", "r");
                    if (fp && fgets(buf, sizeof(buf), fp)) {
                        uint64_t used = 0;
                        if (sscanf(buf, "%lu", (unsigned long *)&used) == 1) {
                            if (available) *available = (limit > used) ? limit - used : 0;
                        }
                        fclose(fp);
                    } else {
                        if (available) *available = 0;
                        if (fp) fclose(fp);
                    }
                    return EAI_OK;
                }
            }
        }
        fclose(fp);
    }

    /* Fallback: cgroup v1 */
    fp = fopen("/sys/fs/cgroup/memory/memory.limit_in_bytes", "r");
    if (fp) {
        uint64_t limit = 0;
        if (fscanf(fp, "%lu", (unsigned long *)&limit) == 1) {
            if (total) *total = limit;
        }
        fclose(fp);

        fp = fopen("/sys/fs/cgroup/memory/memory.usage_in_bytes", "r");
        if (fp) {
            uint64_t used = 0;
            if (fscanf(fp, "%lu", (unsigned long *)&used) == 1) {
                if (available) *available = (limit > used) ? limit - used : 0;
            }
            fclose(fp);
        }
        return EAI_OK;
    }

    /* Last resort: use host sysinfo */
#ifdef __linux__
    {
        struct sysinfo si;
        if (sysinfo(&si) == 0) {
            if (total)     *total     = (uint64_t)si.totalram * si.mem_unit;
            if (available) *available = (uint64_t)si.freeram  * si.mem_unit;
            return EAI_OK;
        }
    }
#endif

    if (total)     *total     = 0;
    if (available) *available = 0;
    return EAI_ERR_IO;
}

static eai_status_t container_get_cpu_temp(eai_platform_t *plat, float *temp_c)
{
    (void)plat; (void)temp_c;
    return EAI_ERR_UNSUPPORTED;
}

static void container_shutdown(eai_platform_t *plat)
{
    EAI_LOG_INFO(LOG_MOD, "Container platform shut down");
}

const eai_platform_ops_t eai_platform_container_ops = {
    .name            = "container",
    .init            = container_init,
    .get_device_info = container_get_device_info,
    .read_gpio       = container_read_gpio,
    .write_gpio      = container_write_gpio,
    .get_memory_info = container_get_memory_info,
    .get_cpu_temp    = container_get_cpu_temp,
    .shutdown        = container_shutdown,
};

#else /* non-Linux stub */

const eai_platform_ops_t eai_platform_container_ops = {
    .name = "container-unavailable",
};

#endif
