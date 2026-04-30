// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// Stable C API implementation — version & status only (no platform dependency)

#include "eai/eai_api.h"
#include "eai/common.h"
#include <string.h>

/* ========== Version ========== */

const char *eai_version(void)
{
    return EAI_VERSION_STRING;
}

int eai_version_major(void)
{
    return EAI_VERSION_MAJOR;
}

int eai_version_minor(void)
{
    return EAI_VERSION_MINOR;
}

int eai_version_patch(void)
{
    return EAI_VERSION_PATCH;
}

/* ========== Status ========== */

const char *eai_api_status_str(eai_status_t status)
{
    return eai_status_str(status);
}
