// SPDX-License-Identifier: MIT
// HAL GPIO implementation for Linux (sysfs)

#include "eai/platform.h"
#include <stdio.h>
#include <string.h>

#ifndef _WIN32
#ifdef __linux__

static eai_status_t linux_hal_gpio_configure(eai_platform_t *plat, int pin, eai_gpio_dir_t dir)
{
    char path[128];
    /* Export the pin */
    FILE *fp = fopen("/sys/class/gpio/export", "w");
    if (!fp) return EAI_ERR_IO;
    fprintf(fp, "%d", pin);
    fclose(fp);

    /* Set direction */
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", pin);
    fp = fopen(path, "w");
    if (!fp) return EAI_ERR_IO;
    fprintf(fp, "%s", (dir == EAI_GPIO_OUTPUT) ? "out" : "in");
    fclose(fp);
    return EAI_OK;
}

static eai_status_t linux_hal_gpio_read(eai_platform_t *plat, int pin, int *value)
{
    char path[128];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);
    FILE *fp = fopen(path, "r");
    if (!fp) return EAI_ERR_IO;
    if (fscanf(fp, "%d", value) != 1) {
        fclose(fp);
        return EAI_ERR_IO;
    }
    fclose(fp);
    return EAI_OK;
}

static eai_status_t linux_hal_gpio_write(eai_platform_t *plat, int pin, int value)
{
    char path[128];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);
    FILE *fp = fopen(path, "w");
    if (!fp) return EAI_ERR_IO;
    fprintf(fp, "%d", value);
    fclose(fp);
    return EAI_OK;
}

static eai_status_t linux_hal_gpio_set_interrupt(eai_platform_t *plat, int pin,
                                                  eai_gpio_edge_t edge,
                                                  eai_gpio_isr_t isr, void *user_data)
{
    (void)plat; (void)pin; (void)edge; (void)isr; (void)user_data;
    return EAI_ERR_NOT_IMPLEMENTED;
}

static uint32_t linux_hal_gpio_get_capabilities(eai_platform_t *plat)
{
    (void)plat;
    return 0x01; /* basic read/write via sysfs */
}

const eai_hal_gpio_ops_t eai_hal_linux_gpio_ops = {
    .configure        = linux_hal_gpio_configure,
    .read             = linux_hal_gpio_read,
    .write            = linux_hal_gpio_write,
    .set_interrupt    = linux_hal_gpio_set_interrupt,
    .get_capabilities = linux_hal_gpio_get_capabilities,
};

#endif /* __linux__ */
#endif /* !_WIN32 */
