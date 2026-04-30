// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// HAL GPIO sub-vtable — nullable (desktops/containers omit this)

#ifndef EAI_HAL_GPIO_H
#define EAI_HAL_GPIO_H

#include "eai/types.h"

struct eai_platform_s;

typedef enum {
    EAI_GPIO_INPUT  = 0,
    EAI_GPIO_OUTPUT = 1,
} eai_gpio_dir_t;

typedef enum {
    EAI_GPIO_EDGE_NONE    = 0,
    EAI_GPIO_EDGE_RISING  = 1,
    EAI_GPIO_EDGE_FALLING = 2,
    EAI_GPIO_EDGE_BOTH    = 3,
} eai_gpio_edge_t;

typedef void (*eai_gpio_isr_t)(int pin, void *user_data);

typedef struct {
    eai_status_t (*configure)(struct eai_platform_s *plat, int pin, eai_gpio_dir_t dir);
    eai_status_t (*read)(struct eai_platform_s *plat, int pin, int *value);
    eai_status_t (*write)(struct eai_platform_s *plat, int pin, int value);
    eai_status_t (*set_interrupt)(struct eai_platform_s *plat, int pin, eai_gpio_edge_t edge,
                                  eai_gpio_isr_t isr, void *user_data);
    uint32_t     (*get_capabilities)(struct eai_platform_s *plat);
} eai_hal_gpio_ops_t;

#endif /* EAI_HAL_GPIO_H */
