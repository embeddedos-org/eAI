# eAI — Embedded AI Layer

[![CI](https://github.com/embeddedos-org/eAI/actions/workflows/ci.yml/badge.svg)](https://github.com/embeddedos-org/eAI/actions/workflows/ci.yml)
[![CodeQL](https://github.com/embeddedos-org/eAI/actions/workflows/codeql.yml/badge.svg)](https://github.com/embeddedos-org/eAI/actions/workflows/codeql.yml)
[![Scorecard](https://github.com/embeddedos-org/eAI/actions/workflows/scorecard.yml/badge.svg)](https://github.com/embeddedos-org/eAI/actions/workflows/scorecard.yml)
[![Release](https://github.com/embeddedos-org/eAI/actions/workflows/release.yml/badge.svg)](https://github.com/embeddedos-org/eAI/actions/workflows/release.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

eAI (CMake project `EAI`) is the on-device AI layer of the EmbeddedOS ecosystem,
written in C. It provides a runtime contract for loading and running models plus
several build-selectable runtimes: **EAI-Min**, a lightweight agent runtime for
IoT devices, and **EAI-Framework**, a larger platform for industrial systems. It
adds model-format loaders, optional hardware-accelerator backends, a
brain-computer-interface module, a CLI, and adapters for a range of host and
embedded platforms.

eAI is part of the **EmbeddedOS**
([embeddedos-org](https://github.com/embeddedos-org)) ecosystem, alongside
[EoS](https://github.com/embeddedos-org/eos) (OS core),
[eBoot](https://github.com/embeddedos-org/eBoot) (secure bootloader), and
[eNI](https://github.com/embeddedos-org/eNI) (neural interface). Project version
0.2.0.

## What's inside

| Path | Contents |
|------|----------|
| `common/` | Shared contracts and utilities: types, config, logging, model manifest, runtime contract, tool registry, security/permissions — builds `eai_common` |
| `platform/` | Platform adapters: Linux, Windows, macOS, Android, iOS, container, Zephyr, FreeRTOS, bare-metal, EoS — builds `eai_platform` |
| `min/` | EAI-Min lightweight agent runtime (agent loop, router, KV memory) |
| `framework/` | EAI-Framework industrial platform |
| `bci/` | Brain-computer-interface module |
| `models/` | Model manifests and metadata |
| `accel/` | Accelerator backends (optional Vulkan Compute, CoreML/Metal, Qualcomm QNN) |
| `formats/` | Model format loaders |
| `cli/` | `eai` command-line tool |
| `bindings/` | C++ and Python bindings |
| `profiles/` | Example profiles: `adaptive-edge`, `bci-assistive`, `industrial-gateway`, `mobile-edge`, `robot-controller`, `smart-camera` |
| `tests/` | Unit, functional, performance, and simulation tests |

`CODE_STRUCTURE.md` is the detailed, module-by-module developer reference.

## Build

Requires CMake ≥ 3.16 and a C compiler. `CMakePresets.json` defines the
host/target presets used by CI (e.g. `linux-x64-release`).

```bash
cmake --preset linux-x64-release
cmake --build --preset linux-x64-release

# or without presets
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

### Selected build options

| Option | Default | Meaning |
|--------|---------|---------|
| `EAI_BUILD_MIN` | `ON` | Build the EAI-Min lightweight runtime |
| `EAI_BUILD_FRAMEWORK` | `ON` | Build the EAI-Framework platform |
| `EAI_BUILD_BCI` | `ON` | Build the brain-computer-interface module |
| `EAI_BUILD_ACCEL` | `ON` | Build the accelerator module |
| `EAI_BUILD_FORMATS` | `ON` | Build model-format loaders |
| `EAI_BUILD_CLI` | `ON` | Build the CLI |
| `EAI_BUILD_TESTS` | `OFF` | Build unit tests |
| `EAI_ACCEL_VULKAN` / `EAI_ACCEL_COREML` / `EAI_ACCEL_QNN` | `OFF` | Enable a specific accelerator backend |
| `EAI_PLATFORM_*` | varies | Enable a platform adapter (Linux on by default; auto-selected on Windows/macOS/Android) |

## Test

```bash
cmake -B build -DEAI_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure

# Python-driven suites
python run_all_tests.py
```

## Docs

See [`docs/`](docs/) and [`CODE_STRUCTURE.md`](CODE_STRUCTURE.md); API docs are
generated via `Doxyfile`.

## License

Licensed under the [MIT License](LICENSE).
