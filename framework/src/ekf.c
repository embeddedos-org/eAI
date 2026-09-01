// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// ISO/IEC 25000 | ISO/IEC/IEEE 15288:2023

#include "eai_fw/ekf.h"

#include <string.h>

#define N_MAX EAI_EKF_MAX_STATE
#define M_MAX EAI_EKF_MAX_MEAS

/* Scratch is sized for the largest intermediate any routine below needs.
 * Everything is stack-local: the filter must be usable from more than one task
 * without a lock, so no static working buffers. Worst case here is three
 * N_MAX x N_MAX floats plus change — 460 bytes at N_MAX = 6. */

/* C = A * B, with A (ar x ac) and B (ac x bc), all row-major. */
static void mat_mul(float *C, const float *A, const float *B,
                    uint8_t ar, uint8_t ac, uint8_t bc)
{
    for (uint8_t i = 0; i < ar; i++) {
        for (uint8_t j = 0; j < bc; j++) {
            float sum = 0.0f;
            for (uint8_t k = 0; k < ac; k++) {
                sum += A[i * ac + k] * B[k * bc + j];
            }
            C[i * bc + j] = sum;
        }
    }
}

/* C = A * Bᵀ, with A (ar x ac) and B (br x ac). */
static void mat_mul_t(float *C, const float *A, const float *B,
                      uint8_t ar, uint8_t ac, uint8_t br)
{
    for (uint8_t i = 0; i < ar; i++) {
        for (uint8_t j = 0; j < br; j++) {
            float sum = 0.0f;
            for (uint8_t k = 0; k < ac; k++) {
                sum += A[i * ac + k] * B[j * ac + k];
            }
            C[i * br + j] = sum;
        }
    }
}

/* y = A * x, with A (ar x ac). */
static void mat_vec(float *y, const float *A, const float *x,
                    uint8_t ar, uint8_t ac)
{
    for (uint8_t i = 0; i < ar; i++) {
        float sum = 0.0f;
        for (uint8_t k = 0; k < ac; k++) {
            sum += A[i * ac + k] * x[k];
        }
        y[i] = sum;
    }
}

/* Invert a small symmetric positive-definite matrix by Gauss-Jordan with
 * partial pivoting. m is at most EAI_EKF_MAX_MEAS, so an O(m^3) direct method
 * is both the simplest and the fastest option. Returns false when the matrix is
 * singular to working precision. */
static bool mat_inv(float *out, const float *in, uint8_t m)
{
    float a[M_MAX * M_MAX];
    memcpy(a, in, (size_t)m * m * sizeof(float));

    for (uint8_t i = 0; i < m; i++) {
        for (uint8_t j = 0; j < m; j++) {
            out[i * m + j] = (i == j) ? 1.0f : 0.0f;
        }
    }

    for (uint8_t col = 0; col < m; col++) {
        uint8_t pivot = col;
        float best = a[col * m + col];
        if (best < 0.0f) best = -best;

        for (uint8_t row = col + 1; row < m; row++) {
            float v = a[row * m + col];
            if (v < 0.0f) v = -v;
            if (v > best) { best = v; pivot = row; }
        }

        /* Singular to working precision. The caller must not proceed: the gain
         * would be meaningless and the covariance would be poisoned. */
        if (best <= 1e-12f) {
            return false;
        }

        if (pivot != col) {
            for (uint8_t j = 0; j < m; j++) {
                float t = a[col * m + j];
                a[col * m + j] = a[pivot * m + j];
                a[pivot * m + j] = t;

                t = out[col * m + j];
                out[col * m + j] = out[pivot * m + j];
                out[pivot * m + j] = t;
            }
        }

        const float inv_pivot = 1.0f / a[col * m + col];
        for (uint8_t j = 0; j < m; j++) {
            a[col * m + j] *= inv_pivot;
            out[col * m + j] *= inv_pivot;
        }

        for (uint8_t row = 0; row < m; row++) {
            if (row == col) continue;
            const float factor = a[row * m + col];
            if (factor == 0.0f) continue;
            for (uint8_t j = 0; j < m; j++) {
                a[row * m + j] -= factor * a[col * m + j];
                out[row * m + j] -= factor * out[col * m + j];
            }
        }
    }

    return true;
}

eai_status_t eai_ekf_init(eai_ekf_t *ekf, uint8_t n, float initial_variance)
{
    if (!ekf || n == 0 || n > EAI_EKF_MAX_STATE) {
        return EAI_ERR_INVALID;
    }

    memset(ekf, 0, sizeof(*ekf));
    ekf->n = n;

    for (uint8_t i = 0; i < n; i++) {
        ekf->P[i * n + i] = initial_variance;
    }

    ekf->initialised = true;
    return EAI_OK;
}

eai_status_t eai_ekf_predict(eai_ekf_t *ekf, const float *F, const float *Q)
{
    if (!ekf || !ekf->initialised || !F || !Q) {
        return EAI_ERR_INVALID;
    }

    const uint8_t n = ekf->n;

    float x_new[N_MAX];
    mat_vec(x_new, F, ekf->x, n, n);
    memcpy(ekf->x, x_new, (size_t)n * sizeof(float));

    /* P = F P Fᵀ + Q */
    float FP[N_MAX * N_MAX];
    float P_new[N_MAX * N_MAX];
    mat_mul(FP, F, ekf->P, n, n, n);
    mat_mul_t(P_new, FP, F, n, n, n);

    for (uint8_t i = 0; i < (uint8_t)(n * n); i++) {
        ekf->P[i] = P_new[i] + Q[i];
    }

    return EAI_OK;
}

eai_status_t eai_ekf_update(eai_ekf_t *ekf, const float *z, uint8_t m,
                            const float *H, const float *R)
{
    if (!ekf || !ekf->initialised || !z || !H || !R) {
        return EAI_ERR_INVALID;
    }
    if (m == 0 || m > EAI_EKF_MAX_MEAS) {
        return EAI_ERR_INVALID;
    }

    const uint8_t n = ekf->n;

    /* Innovation: y = z - H x */
    float Hx[M_MAX];
    float y[M_MAX];
    mat_vec(Hx, H, ekf->x, m, n);
    for (uint8_t i = 0; i < m; i++) {
        y[i] = z[i] - Hx[i];
    }

    /* Innovation covariance: S = H P Hᵀ + R */
    float HP[M_MAX * N_MAX];
    float S[M_MAX * M_MAX];
    mat_mul(HP, H, ekf->P, m, n, n);
    mat_mul_t(S, HP, H, m, n, m);
    for (uint8_t i = 0; i < (uint8_t)(m * m); i++) {
        S[i] += R[i];
    }

    float S_inv[M_MAX * M_MAX];
    if (!mat_inv(S_inv, S, m)) {
        /* R is zero, or the model says this measurement carries no information.
         * Either way the gain is undefined; leave the estimate untouched rather
         * than corrupt it. */
        return EAI_ERR_RUNTIME;
    }

    /* Gain: K = P Hᵀ S⁻¹ */
    float PHt[N_MAX * M_MAX];
    float K[N_MAX * M_MAX];
    mat_mul_t(PHt, ekf->P, H, n, n, m);
    mat_mul(K, PHt, S_inv, n, m, m);

    /* x = x + K y */
    float Ky[N_MAX];
    mat_vec(Ky, K, y, n, m);
    for (uint8_t i = 0; i < n; i++) {
        ekf->x[i] += Ky[i];
    }

    /* Joseph form: P = (I - K H) P (I - K H)ᵀ + K R Kᵀ.
     *
     * The short form (I - K H) P is algebraically identical and one multiply
     * cheaper, but it loses symmetry to floating-point rounding. An asymmetric
     * P drifts toward indefiniteness over a long run, and the filter then
     * diverges with no error ever being reported — hours into a deployment,
     * not during a unit test. This form is symmetric by construction. */
    float KH[N_MAX * N_MAX];
    float IKH[N_MAX * N_MAX];
    mat_mul(KH, K, H, n, m, n);
    for (uint8_t i = 0; i < n; i++) {
        for (uint8_t j = 0; j < n; j++) {
            const float ident = (i == j) ? 1.0f : 0.0f;
            IKH[i * n + j] = ident - KH[i * n + j];
        }
    }

    float tmp[N_MAX * N_MAX];
    float P_new[N_MAX * N_MAX];
    mat_mul(tmp, IKH, ekf->P, n, n, n);
    mat_mul_t(P_new, tmp, IKH, n, n, n);

    float KR[N_MAX * M_MAX];
    float KRKt[N_MAX * N_MAX];
    mat_mul(KR, K, R, n, m, m);
    mat_mul_t(KRKt, KR, K, n, m, n);

    for (uint8_t i = 0; i < (uint8_t)(n * n); i++) {
        ekf->P[i] = P_new[i] + KRKt[i];
    }

    return EAI_OK;
}

float eai_ekf_state(const eai_ekf_t *ekf, uint8_t i)
{
    if (!ekf || !ekf->initialised || i >= ekf->n) {
        return 0.0f;
    }
    return ekf->x[i];
}

float eai_ekf_variance(const eai_ekf_t *ekf, uint8_t i)
{
    if (!ekf || !ekf->initialised || i >= ekf->n) {
        return 0.0f;
    }
    return ekf->P[i * ekf->n + i];
}

eai_status_t eai_ekf_const_velocity_F(float *F, uint8_t n, float dt_s)
{
    if (!F || n == 0 || n > EAI_EKF_MAX_STATE || (n % 2) != 0) {
        return EAI_ERR_INVALID;
    }

    const uint8_t half = (uint8_t)(n / 2);

    memset(F, 0, (size_t)n * n * sizeof(float));
    for (uint8_t i = 0; i < n; i++) {
        F[i * n + i] = 1.0f;
    }
    /* position_i += velocity_i * dt */
    for (uint8_t i = 0; i < half; i++) {
        F[i * n + (half + i)] = dt_s;
    }

    return EAI_OK;
}
