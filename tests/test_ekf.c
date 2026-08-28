// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// ISO/IEC 25000 | ISO/IEC/IEEE 15288:2023

/**
 * @file test_ekf.c
 * @brief Tests for the multi-state Kalman filter.
 *
 * The headline case is constant-velocity tracking: feed the filter noisy
 * *positions* only, and check it recovers *velocity*, which it never sees. That
 * is the capability the scalar filter in sensor_fusion.c cannot have — it holds
 * one float, so there is nowhere for a second state to live.
 */

#include "eai_fw/ekf.h"

#include <assert.h>
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

/* Deterministic pseudo-noise. A real RNG would make failures unreproducible,
 * and the point of these tests is that a failure can be re-run. */
static float noise(uint32_t i)
{
    const float table[8] = {
        0.42f, -0.31f, 0.18f, -0.55f, 0.27f, -0.12f, 0.61f, -0.38f
    };
    return table[i % 8u];
}

static int test_init_rejects_bad_args(void)
{
    printf("-- init argument validation --\n");
    eai_ekf_t ekf;

    CHECK(eai_ekf_init(NULL, 2, 1.0f) == EAI_ERR_INVALID, "NULL filter rejected");
    CHECK(eai_ekf_init(&ekf, 0, 1.0f) == EAI_ERR_INVALID, "zero dimension rejected");
    CHECK(eai_ekf_init(&ekf, EAI_EKF_MAX_STATE + 1, 1.0f) == EAI_ERR_INVALID,
          "oversized dimension rejected");
    CHECK(eai_ekf_init(&ekf, 2, 100.0f) == EAI_OK, "valid init accepted");
    CHECK(fabsf(eai_ekf_variance(&ekf, 0) - 100.0f) < 1e-6f, "P diagonal seeded");
    CHECK(fabsf(eai_ekf_state(&ekf, 0)) < 1e-6f, "state starts at zero");
    CHECK(fabsf(eai_ekf_state(&ekf, 9)) < 1e-6f, "out-of-range state reads 0");

    printf("  [PASS] init validation\n");
    return 0;
}

static int test_recovers_unobserved_velocity(void)
{
    printf("-- constant-velocity tracking from position alone --\n");

    /* Truth: starts at 0, moves at 2.0 units/second. dt = 0.1s. */
    const float true_velocity = 2.0f;
    const float dt = 0.1f;

    eai_ekf_t ekf;
    CHECK(eai_ekf_init(&ekf, 2, 500.0f) == EAI_OK, "init");

    float F[4];
    CHECK(eai_ekf_const_velocity_F(F, 2, dt) == EAI_OK, "build F");
    CHECK(fabsf(F[1] - dt) < 1e-6f, "F couples position to velocity");

    /* Small process noise: the model really is constant velocity. */
    const float Q[4] = { 1e-4f, 0.0f,
                         0.0f,  1e-4f };
    /* We observe position only. H picks state 0 out of the state vector. */
    const float H[2] = { 1.0f, 0.0f };
    const float R[1] = { 0.25f };   /* position sensor variance */

    for (uint32_t k = 1; k <= 200u; k++) {
        const float truth = true_velocity * dt * (float)k;
        const float z[1] = { truth + noise(k) * 0.5f };

        CHECK(eai_ekf_predict(&ekf, F, Q) == EAI_OK, "predict");
        CHECK(eai_ekf_update(&ekf, z, 1, H, R) == EAI_OK, "update");
    }

    const float est_pos = eai_ekf_state(&ekf, 0);
    const float est_vel = eai_ekf_state(&ekf, 1);
    const float true_pos = true_velocity * dt * 200.0f;

    printf("     true pos %.3f  est %.3f   |   true vel %.3f  est %.3f\n",
           (double)true_pos, (double)est_pos,
           (double)true_velocity, (double)est_vel);

    CHECK(fabsf(est_pos - true_pos) < 0.5f, "position tracked");

    /* The whole point: velocity was never measured. */
    CHECK(fabsf(est_vel - true_velocity) < 0.2f, "velocity recovered from position");

    /* Uncertainty must shrink as evidence accumulates. */
    CHECK(eai_ekf_variance(&ekf, 0) < 500.0f, "position variance fell");
    CHECK(eai_ekf_variance(&ekf, 1) < 500.0f, "velocity variance fell");

    printf("  [PASS] velocity recovered without ever being measured\n");
    return 0;
}

static int test_covariance_stays_symmetric(void)
{
    printf("-- covariance symmetry over a long run --\n");

    /* This is what the Joseph form buys. The short update form drifts
     * asymmetric over thousands of iterations and the filter then diverges
     * with nothing reporting an error. */
    eai_ekf_t ekf;
    CHECK(eai_ekf_init(&ekf, 4, 10.0f) == EAI_OK, "init 4-state");

    float F[16];
    CHECK(eai_ekf_const_velocity_F(F, 4, 0.05f) == EAI_OK, "build F");

    float Q[16] = { 0 };
    for (int i = 0; i < 4; i++) Q[i * 4 + i] = 1e-3f;

    /* Observe both positions. */
    const float H[8] = { 1.0f, 0.0f, 0.0f, 0.0f,
                         0.0f, 1.0f, 0.0f, 0.0f };
    const float R[4] = { 0.1f, 0.0f,
                         0.0f, 0.1f };

    for (uint32_t k = 1; k <= 5000u; k++) {
        const float z[2] = { 0.05f * (float)k + noise(k) * 0.2f,
                             0.02f * (float)k + noise(k + 3u) * 0.2f };
        CHECK(eai_ekf_predict(&ekf, F, Q) == EAI_OK, "predict");
        CHECK(eai_ekf_update(&ekf, z, 2, H, R) == EAI_OK, "update");
    }

    float worst = 0.0f;
    for (uint8_t i = 0; i < 4; i++) {
        for (uint8_t j = 0; j < 4; j++) {
            const float a = ekf.P[i * 4 + j];
            const float b = ekf.P[j * 4 + i];
            const float d = fabsf(a - b);
            if (d > worst) worst = d;
        }
    }
    printf("     worst |P[i][j] - P[j][i]| after 5000 updates: %.3e\n", (double)worst);
    CHECK(worst < 1e-6f, "P remained symmetric");

    for (uint8_t i = 0; i < 4; i++) {
        CHECK(eai_ekf_variance(&ekf, i) > 0.0f, "variance stayed positive");
        CHECK(isfinite(eai_ekf_variance(&ekf, i)), "variance stayed finite");
    }

    printf("  [PASS] covariance symmetric and positive after 5000 updates\n");
    return 0;
}

static int test_singular_innovation_is_refused(void)
{
    printf("-- singular innovation covariance --\n");

    eai_ekf_t ekf;
    CHECK(eai_ekf_init(&ekf, 2, 0.0f) == EAI_OK, "init with zero P");

    /* P is zero and R is zero, so S = H P Hᵀ + R is zero: the gain is
     * undefined. The filter must refuse rather than divide by zero. */
    const float H[2] = { 1.0f, 0.0f };
    const float R[1] = { 0.0f };
    const float z[1] = { 5.0f };

    CHECK(eai_ekf_update(&ekf, z, 1, H, R) == EAI_ERR_RUNTIME, "singular S refused");
    CHECK(fabsf(eai_ekf_state(&ekf, 0)) < 1e-6f, "state left untouched");

    printf("  [PASS] singular update refused without corrupting the estimate\n");
    return 0;
}

static int test_update_argument_validation(void)
{
    printf("-- update argument validation --\n");

    eai_ekf_t ekf;
    CHECK(eai_ekf_init(&ekf, 2, 1.0f) == EAI_OK, "init");

    const float H[2] = { 1.0f, 0.0f };
    const float R[1] = { 1.0f };
    const float z[1] = { 1.0f };

    CHECK(eai_ekf_update(NULL, z, 1, H, R) == EAI_ERR_INVALID, "NULL filter");
    CHECK(eai_ekf_update(&ekf, NULL, 1, H, R) == EAI_ERR_INVALID, "NULL z");
    CHECK(eai_ekf_update(&ekf, z, 0, H, R) == EAI_ERR_INVALID, "zero m");
    CHECK(eai_ekf_update(&ekf, z, EAI_EKF_MAX_MEAS + 1, H, R) == EAI_ERR_INVALID,
          "oversized m");
    CHECK(eai_ekf_predict(&ekf, NULL, NULL) == EAI_ERR_INVALID, "NULL F/Q");
    float scratch[EAI_EKF_MAX_STATE * EAI_EKF_MAX_STATE];
    CHECK(eai_ekf_const_velocity_F(NULL, 2, 0.1f) == EAI_ERR_INVALID, "NULL F out");
    CHECK(eai_ekf_const_velocity_F(scratch, 3, 0.1f) == EAI_ERR_INVALID,
          "odd dimension rejected — constant velocity needs paired pos/vel states");

    printf("  [PASS] update validation\n");
    return 0;
}

int main(void)
{
    printf("=== eAI EKF Tests ===\n");

    if (test_init_rejects_bad_args()) return 1;
    if (test_recovers_unobserved_velocity()) return 1;
    if (test_covariance_stays_symmetric()) return 1;
    if (test_singular_innovation_is_refused()) return 1;
    if (test_update_argument_validation()) return 1;

    printf("\n=== %d/%d checks passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
