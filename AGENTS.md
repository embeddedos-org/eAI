# AGENTS.md — eAI

eAI (CMake project `EAI`, v0.2.0) is the on-device AI layer of the EmbeddedOS
ecosystem, written in C (C11): a runtime contract for loading and running
models plus build-selectable runtimes — **EAI-Min** (lightweight agent runtime
for IoT devices) and **EAI-Framework** (industrial platform) — with
model-format loaders, optional hardware-accelerator backends, a
brain-computer-interface module, a CLI, and host/embedded platform adapters.
(Provenance: `README.md` intro, `CMakeLists.txt` `project()`/options.)

## Layout (from `README.md` "What's inside")

- `common/` — shared contracts and utilities (types, config, logging, model
  manifest, runtime contract, tool registry, security/permissions); builds
  `eai_common`
- `platform/` — platform adapters (Linux, Windows, macOS, Android, iOS,
  container, Zephyr, FreeRTOS, bare-metal, EoS); builds `eai_platform`
- `min/` — EAI-Min lightweight agent runtime (agent loop, router, KV memory)
- `framework/` — EAI-Framework industrial platform (runtimes, connectors,
  orchestrator, policy, observability)
- `bci/` — brain-computer-interface module
- `models/` — model manifests and metadata
- `accel/` — accelerator backends (optional Vulkan Compute, CoreML/Metal,
  Qualcomm QNN)
- `formats/` — model format loaders
- `cli/` — `eai` command-line tool
- `bindings/` — C++ and Python bindings
- `profiles/` — example profiles: `adaptive-edge`, `bci-assistive`,
  `industrial-gateway`, `mobile-edge`, `robot-controller`, `smart-camera`
- `tests/` — C tests plus `unit/`, `functional/`, `performance/`,
  `simulation/` Python suites
- `cmake/toolchains/`, `ci/`, `docs/`, `tools/` — cross-compile toolchains,
  CI build/test scripts, docs, developer tools

`CODE_STRUCTURE.md` is the detailed, module-by-module developer reference.

## Build (from `README.md` "Build" and `CMakePresets.json`)

Requires CMake ≥ 3.16 and a C compiler. `CMakePresets.json` defines the
host/target presets used by CI (e.g. `linux-x64-release`).

```bash
cmake --preset linux-x64-release
cmake --build --preset linux-x64-release

# or without presets
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

## Test (from `README.md` "Test" and `CONTRIBUTING.md` "Development Setup")

```bash
cmake -B build -DEAI_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure

# Python-driven suites
python run_all_tests.py
```

`CONTRIBUTING.md` "CI Requirements" also documents profile builds (at least
one of `smart-camera`, `industrial-gateway`, `robot-controller`,
`mobile-edge`) and the nightly regression (full tests on 3 OSes, all
profiles, AArch64/ARM/RISC-V cross-compiles) as informational.

## Lint / format

Not defined: no lint or format command is documented in `README.md` or
`CONTRIBUTING.md`. C style rules — C11, `-Wall -Wextra` clean, platform
`#ifdef` guards, no hardcoded paths, `#ifndef` include guards, `<stdint.h>`
types — are in `CONTRIBUTING.md` "Code Guidelines". (The dev Docker image
installs `cppcheck`/`clang-format`, but the repo documents no command to
invoke them; do not invent one.)

## Contributing

See `CONTRIBUTING.md`: fork, create a feature branch
(`git checkout -b feat/my-feature`), run the build and tests locally, then
submit a pull request. Follow Conventional Commits; new features need unit
tests in `tests/`; platform-specific code needs `#ifdef` guards for all
targets; every human-authored PR must close a same-repository issue with
`Fixes #<number>`.

## Security

See `SECURITY.md`. Report vulnerabilities to security@embeddedos.org — do NOT
open public issues for vulnerabilities. Response SLA: acknowledgment within
48 hours.
