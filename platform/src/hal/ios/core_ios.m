// SPDX-License-Identifier: MIT
// HAL Core implementation for iOS

#ifndef EAI_HAL_IOS_CORE_H
#define EAI_HAL_IOS_CORE_H

#include "eai/platform.h"
#include "eai/log.h"

#if defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE

#import <UIKit/UIKit.h>
#import <sys/sysctl.h>

#define LOG_MOD "hal-ios-core"

static eai_status_t ios_hal_init(eai_platform_t *plat)
{
    EAI_LOG_INFO(LOG_MOD, "iOS HAL core initialized");
    return EAI_OK;
}

static void ios_hal_shutdown(eai_platform_t *plat)
{
    EAI_LOG_INFO(LOG_MOD, "iOS HAL core shut down");
}

static eai_status_t ios_hal_get_device_info(eai_platform_t *plat, char *buf, size_t buf_size)
{
    @autoreleasepool {
        NSString *model = [[UIDevice currentDevice] model];
        NSString *sysVer = [[UIDevice currentDevice] systemVersion];
        snprintf(buf, buf_size, "iOS %s (%s)",
                 [sysVer UTF8String], [model UTF8String]);
    }
    return EAI_OK;
}

static eai_status_t ios_hal_get_os_name(eai_platform_t *plat, char *buf, size_t buf_size)
{
    snprintf(buf, buf_size, "iOS");
    return EAI_OK;
}

static eai_status_t ios_hal_get_cpu_temp(eai_platform_t *plat, float *temp_c)
{
    (void)plat; (void)temp_c;
    return EAI_ERR_UNSUPPORTED;
}

static int ios_hal_get_cpu_count(eai_platform_t *plat)
{
    int count = 0;
    size_t len = sizeof(count);
    if (sysctlbyname("hw.ncpu", &count, &len, NULL, 0) != 0) return 1;
    return (count > 0) ? count : 1;
}

const eai_hal_core_ops_t eai_hal_ios_core_ops = {
    .init            = ios_hal_init,
    .shutdown        = ios_hal_shutdown,
    .get_device_info = ios_hal_get_device_info,
    .get_os_name     = ios_hal_get_os_name,
    .get_cpu_temp    = ios_hal_get_cpu_temp,
    .get_cpu_count   = ios_hal_get_cpu_count,
};

#endif /* TARGET_OS_IPHONE */
#endif /* __APPLE__ */
#endif /* EAI_HAL_IOS_CORE_H */
