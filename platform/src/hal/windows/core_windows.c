// SPDX-License-Identifier: MIT
// HAL Core implementation for Windows

#include "eai/platform.h"
#include "eai/log.h"
#include <stdio.h>
#include <string.h>

#define LOG_MOD "hal-win-core"

#ifdef _WIN32

#include <windows.h>
#include <winternl.h>

typedef NTSTATUS (WINAPI *RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

static eai_status_t win_hal_init(eai_platform_t *plat)
{
    EAI_LOG_INFO(LOG_MOD, "Windows HAL core initialized");
    return EAI_OK;
}

static void win_hal_shutdown(eai_platform_t *plat)
{
    EAI_LOG_INFO(LOG_MOD, "Windows HAL core shut down");
}

static eai_status_t win_hal_get_device_info(eai_platform_t *plat, char *buf, size_t buf_size)
{
    RTL_OSVERSIONINFOW vi;
    memset(&vi, 0, sizeof(vi));
    vi.dwOSVersionInfoSize = sizeof(vi);

    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (ntdll) {
        RtlGetVersionPtr fn = (RtlGetVersionPtr)GetProcAddress(ntdll, "RtlGetVersion");
        if (fn && fn(&vi) == 0) {
            snprintf(buf, buf_size, "Windows %lu.%lu (Build %lu)",
                     (unsigned long)vi.dwMajorVersion,
                     (unsigned long)vi.dwMinorVersion,
                     (unsigned long)vi.dwBuildNumber);
            return EAI_OK;
        }
    }
    snprintf(buf, buf_size, "Windows (version unknown)");
    return EAI_OK;
}

static eai_status_t win_hal_get_os_name(eai_platform_t *plat, char *buf, size_t buf_size)
{
    snprintf(buf, buf_size, "Windows");
    return EAI_OK;
}

static eai_status_t win_hal_get_cpu_temp(eai_platform_t *plat, float *temp_c)
{
    (void)plat; (void)temp_c;
    return EAI_ERR_UNSUPPORTED;
}

static int win_hal_get_cpu_count(eai_platform_t *plat)
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (int)si.dwNumberOfProcessors;
}

const eai_hal_core_ops_t eai_hal_windows_core_ops = {
    .init            = win_hal_init,
    .shutdown        = win_hal_shutdown,
    .get_device_info = win_hal_get_device_info,
    .get_os_name     = win_hal_get_os_name,
    .get_cpu_temp    = win_hal_get_cpu_temp,
    .get_cpu_count   = win_hal_get_cpu_count,
};

#endif /* _WIN32 */
