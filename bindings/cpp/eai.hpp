// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// Header-only C++17 RAII wrappers for eAI core APIs

#pragma once

#include "eai/eai_api.h"
#include "eai/platform.h"
#include "eai/accel.h"

#include <string>
#include <vector>
#include <stdexcept>
#include <optional>
#include <cstdint>
#include <memory>

namespace eai {

// ========== Status exception ==========

class Error : public std::runtime_error {
public:
    explicit Error(eai_status_t status, const char *context = "")
        : std::runtime_error(std::string(context) + ": " + eai_status_str(status))
        , status_(status) {}

    eai_status_t status() const { return status_; }

private:
    eai_status_t status_;
};

inline void check(eai_status_t st, const char *context = "eAI") {
    if (st != EAI_OK) throw Error(st, context);
}

// ========== Platform ==========

class Platform {
public:
    Platform() { check(eai_api_platform_detect(&plat_), "Platform::detect"); }
    ~Platform() { eai_api_platform_shutdown(&plat_); }

    Platform(const Platform&) = delete;
    Platform& operator=(const Platform&) = delete;
    Platform(Platform&& o) noexcept : plat_(o.plat_) { o.plat_ = {}; }

    static Platform detect() { return Platform(); }

    std::string info() const {
        char buf[256];
        eai_api_platform_get_info(const_cast<eai_platform_t*>(&plat_), buf, sizeof(buf));
        return buf;
    }

    std::pair<uint64_t, uint64_t> memory() const {
        uint64_t total = 0, avail = 0;
        eai_api_platform_get_memory(const_cast<eai_platform_t*>(&plat_), &total, &avail);
        return {total, avail};
    }

    const eai_platform_t& native() const { return plat_; }

private:
    eai_platform_t plat_{};
};

// ========== Version ==========

inline std::string version() { return eai_version(); }

// ========== Tensor ==========

class Tensor {
public:
    Tensor() = default;

    Tensor(eai_dtype_t dtype, std::initializer_list<int64_t> shape) {
        std::vector<int64_t> s(shape);
        check(eai_tensor_create(&t_, dtype, s.data(), (int)s.size()), "Tensor::create");
    }

    ~Tensor() { eai_tensor_destroy(&t_); }

    Tensor(const Tensor&) = delete;
    Tensor& operator=(const Tensor&) = delete;
    Tensor(Tensor&& o) noexcept : t_(o.t_) { o.t_ = {}; }

    float* data_f32() { return static_cast<float*>(t_.data); }
    const float* data_f32() const { return static_cast<const float*>(t_.data); }
    void* data() { return t_.data; }

    int ndim() const { return t_.ndim; }
    int64_t shape(int dim) const { return t_.shape[dim]; }
    int64_t numel() const { return eai_tensor_numel(&t_); }

    eai_tensor_t& native() { return t_; }
    const eai_tensor_t& native() const { return t_; }

private:
    eai_tensor_t t_{};
};

} // namespace eai
