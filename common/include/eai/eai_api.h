// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// Stable C API surface for eAI — flat API with opaque handles

#ifndef EAI_API_H
#define EAI_API_H

#include "eai/types.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== DLL export/import macros ========== */
#ifdef _WIN32
    #ifdef EAI_BUILD_DLL
        #define EAI_API __declspec(dllexport)
    #elif defined(EAI_USE_DLL)
        #define EAI_API __declspec(dllimport)
    #else
        #define EAI_API
    #endif
#else
    #define EAI_API __attribute__((visibility("default")))
#endif

/* ========== Version ========== */
EAI_API const char *eai_version(void);
EAI_API int         eai_version_major(void);
EAI_API int         eai_version_minor(void);
EAI_API int         eai_version_patch(void);

/* ========== Platform ========== */
/* eai_platform_t is defined in eai/platform.h */
struct eai_platform_s;

EAI_API eai_status_t eai_api_platform_detect(struct eai_platform_s *plat);
EAI_API eai_status_t eai_api_platform_get_info(struct eai_platform_s *plat, char *buf, size_t buf_size);
EAI_API eai_status_t eai_api_platform_get_memory(struct eai_platform_s *plat,
                                                   uint64_t *total, uint64_t *available);
EAI_API void         eai_api_platform_shutdown(struct eai_platform_s *plat);

/* ========== Runtime ========== */
/* eai_runtime_t is defined in eai/runtime_contract.h */
struct eai_runtime_s;

EAI_API eai_status_t eai_api_runtime_create(struct eai_runtime_s **rt);
EAI_API eai_status_t eai_api_runtime_load_model(struct eai_runtime_s *rt, const char *path);
EAI_API eai_status_t eai_api_runtime_infer(struct eai_runtime_s *rt,
                                            const void *input, size_t input_size,
                                            void *output, size_t output_size);
EAI_API void         eai_api_runtime_destroy(struct eai_runtime_s *rt);

/* ========== Accelerator ========== */
EAI_API int          eai_api_accel_count(void);
EAI_API eai_status_t eai_api_accel_get_name(int index, char *buf, size_t buf_size);

/* ========== Status ========== */
EAI_API const char  *eai_api_status_str(eai_status_t status);

#ifdef __cplusplus
}
#endif

#endif /* EAI_API_H */
