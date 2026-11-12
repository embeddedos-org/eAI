// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// ISO/IEC 25000 | ISO/IEC/IEEE 15288:2023

/**
 * @file test_sensor_fusion.c
 * @brief Tests for the sensor fusion groups in framework/src/sensor_fusion.c.
 */

#include "eai_fw/sensor_fusion.h"

#include <math.h>
#include <stdio.h>

static int tests_run = 0;
static int tests_passed = 0;

#define CHECK(cond, msg)                                                  \
    do {                                                                  \
        tests_run++;                                                      \
        if (!(cond)) {                                                    \
            printf("  [FAIL] %s (line %d)\n", (msg), __LINE__);           \
            return 1;                                                     \
        }                                                                 \
        tests_passed++;                                                   \
    } while (0)

/* Two sources in one group; only "a" has ever reported a value. */
static int setup_one_reported(eai_fw_sensor_fusion_t *sf, eai_fusion_algorithm_t algo)
{
    if (eai_fw_fusion_init(sf) != EAI_OK) return 0;
    if (eai_fw_fusion_create_group(sf, "temp", algo) != EAI_OK) return 0;
    if (eai_fw_fusion_add_source(sf, "temp", "a", 1.0f) != EAI_OK) return 0;
    if (eai_fw_fusion_add_source(sf, "temp", "b", 1.0f) != EAI_OK) return 0;
    if (eai_fw_fusion_update_source(sf, "temp", "a", 20.0f) != EAI_OK) return 0;
    return 1;
}

static int test_weighted_avg_ignores_unreported_source(void)
{
    eai_fw_sensor_fusion_t sf;
    float value = 0.0f, confidence = 0.0f;

    CHECK(setup_one_reported(&sf, EAI_FUSION_WEIGHTED_AVG), "setup");
    CHECK(eai_fw_fusion_fuse(&sf, "temp") == EAI_OK, "fuse succeeds");
    eai_fw_fusion_get(&sf, "temp", &value, &confidence);
    printf("  weighted_avg: value=%.2f confidence=%.2f\n", value, confidence);
    CHECK(fabsf(value - 20.0f) < 1e-4f, "weighted average is the one reported value");
    CHECK(fabsf(confidence - 0.5f) < 1e-4f, "confidence is 1 of 2 sources");
    CHECK(eai_fw_fusion_active_sources(&sf, "temp") == 1,
          "a source that never reported is not counted as active");
    return 0;
}

static int test_min_value_ignores_unreported_source(void)
{
    eai_fw_sensor_fusion_t sf;
    float value = 0.0f;

    CHECK(setup_one_reported(&sf, EAI_FUSION_MIN_VALUE), "setup");
    CHECK(eai_fw_fusion_fuse(&sf, "temp") == EAI_OK, "fuse succeeds");
    eai_fw_fusion_get(&sf, "temp", &value, NULL);
    printf("  min_value:    value=%.2f\n", value);
    CHECK(fabsf(value - 20.0f) < 1e-4f, "minimum is not the unreported source's 0");
    return 0;
}

static int test_no_reported_source_is_not_fused(void)
{
    eai_fw_sensor_fusion_t sf;
    float confidence = 1.0f;

    CHECK(eai_fw_fusion_init(&sf) == EAI_OK, "init");
    CHECK(eai_fw_fusion_create_group(&sf, "temp", EAI_FUSION_WEIGHTED_AVG) == EAI_OK, "group");
    CHECK(eai_fw_fusion_add_source(&sf, "temp", "a", 1.0f) == EAI_OK, "source a");
    CHECK(eai_fw_fusion_fuse(&sf, "temp") == EAI_ERR_NOT_FOUND,
          "a group with no reported value has nothing to fuse");
    eai_fw_fusion_get(&sf, "temp", NULL, &confidence);
    CHECK(confidence == 0.0f, "confidence is 0 with no data");
    return 0;
}

int main(void)
{
    int failed = 0;

    printf("=== Sensor fusion tests ===\n");
    failed |= test_weighted_avg_ignores_unreported_source();
    failed |= test_min_value_ignores_unreported_source();
    failed |= test_no_reported_source_is_not_fused();

    printf("=== Results: %d/%d checks passed ===\n", tests_passed, tests_run);
    return failed ? 1 : 0;
}
