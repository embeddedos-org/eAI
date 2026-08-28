// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// ISO/IEC 25000 | ISO/IEC/IEEE 15288:2023

#ifndef EAI_FW_EKF_H
#define EAI_FW_EKF_H

#include "eai/types.h"

/**
 * @file ekf.h
 * @brief Multi-state Kalman filter for sensor fusion.
 *
 * `sensor_fusion.h` fuses several sensors that each measure *the same scalar*
 * into one number. Its `EAI_FUSION_KALMAN` mode is a one-dimensional filter: a
 * single `float` estimate and a scalar covariance. That is the right tool for
 * smoothing three thermometers, and it is structurally unable to do the job
 * V2X object tracking, IMU attitude and world-model state estimation need.
 *
 * Those need a *state vector*: quantities that are never measured directly but
 * are observable through the ones that are. Velocity from a sequence of
 * positions is the canonical case — no scalar filter can produce it, however
 * well tuned, because it has nowhere to keep it.
 *
 * This module adds that. Fixed-size, no dynamic allocation, no recursion, and
 * a bounded worst case, so it is usable inside a control loop on an MCU.
 *
 * Conventions:
 *   x  state vector          (n)
 *   P  state covariance      (n x n)
 *   F  state transition      (n x n)
 *   Q  process noise         (n x n)
 *   z  measurement           (m)
 *   H  measurement model     (m x n)
 *   R  measurement noise     (m x m)
 *
 * Matrices are row-major and dense. At these sizes a dense representation beats
 * a sparse one on both code size and cache behaviour.
 */

/** Maximum state dimension. 6 covers 3-D position + velocity. */
#define EAI_EKF_MAX_STATE   6

/** Maximum measurement dimension per update. */
#define EAI_EKF_MAX_MEAS    3

typedef struct {
    uint8_t n;                                          /**< State dimension. */
    float   x[EAI_EKF_MAX_STATE];                       /**< State estimate. */
    float   P[EAI_EKF_MAX_STATE * EAI_EKF_MAX_STATE];   /**< State covariance. */
    bool    initialised;
} eai_ekf_t;

/**
 * @brief Initialise a filter with state dimension @p n.
 *
 * The state is zeroed and P is set to @p initial_variance on the diagonal.
 * A large initial variance says "the first measurement should dominate";
 * a small one says "trust the zeroed state", which is almost never wanted.
 *
 * @return EAI_ERR_INVALID if @p ekf is NULL, @p n is 0, or @p n exceeds
 *         EAI_EKF_MAX_STATE.
 */
eai_status_t eai_ekf_init(eai_ekf_t *ekf, uint8_t n, float initial_variance);

/**
 * @brief Time update: x = F x, P = F P Fᵀ + Q.
 *
 * @param F  n x n state transition, row-major.
 * @param Q  n x n process noise, row-major. Larger Q means the filter trusts
 *           its own prediction less and reacts faster to measurements.
 */
eai_status_t eai_ekf_predict(eai_ekf_t *ekf, const float *F, const float *Q);

/**
 * @brief Measurement update with @p m observations.
 *
 * Uses the Joseph form for the covariance update:
 *
 *     P = (I - K H) P (I - K H)ᵀ + K R Kᵀ
 *
 * rather than the shorter `P = (I - K H) P`. The short form is algebraically
 * equal but loses symmetry to rounding, and an asymmetric P diverges silently
 * over a long run — which on an embedded target means after hours of uptime,
 * not during the test. The Joseph form stays symmetric and positive
 * semi-definite by construction. It costs one extra n x n multiply.
 *
 * @param z  m measurements.
 * @param H  m x n measurement model, row-major.
 * @param R  m x m measurement noise, row-major.
 * @return EAI_ERR_RUNTIME if the innovation covariance is singular — that
 *         means R is zero or the model claims a measurement carries no
 *         information, and continuing would divide by zero.
 */
eai_status_t eai_ekf_update(eai_ekf_t *ekf, const float *z, uint8_t m,
                            const float *H, const float *R);

/** @brief Read one state element. Returns 0.0f for an out-of-range index. */
float eai_ekf_state(const eai_ekf_t *ekf, uint8_t i);

/** @brief Read one diagonal element of P — the variance of state @p i. */
float eai_ekf_variance(const eai_ekf_t *ekf, uint8_t i);

/**
 * @brief Build a constant-velocity transition matrix into @p F.
 *
 * The common case for tracking: state is [position..., velocity...] and
 * position advances by velocity * dt. @p n must be even.
 *
 * This is a convenience, not a requirement — any F may be passed to
 * eai_ekf_predict().
 */
eai_status_t eai_ekf_const_velocity_F(float *F, uint8_t n, float dt_s);

#endif /* EAI_FW_EKF_H */
