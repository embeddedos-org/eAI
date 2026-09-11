# Development

## Contribution source of truth

[CONTRIBUTING](https://github.com/embeddedos-org/eAI/blob/master/CONTRIBUTING.md)

Before proposing a change, also review the [README](https://github.com/embeddedos-org/eAI/blob/master/README.md). Keep changes scoped, add tests appropriate to the affected behavior, and follow the repository's current automation and review requirements.

## Build and dependency inputs found

`CMakeLists.txt`, `Dockerfile`, `accel/CMakeLists.txt`, `bci/CMakeLists.txt`, `bindings/python/pyproject.toml`, `bindings/python/setup.py`, `cli/CMakeLists.txt`, `common/CMakeLists.txt`, `docker-compose.yml`, `formats/CMakeLists.txt`, `framework/CMakeLists.txt`, `min/CMakeLists.txt`, and 3 more.

## Tests found in the default-branch tree

`tests/CMakeLists.txt`, `tests/__init__.py`, `tests/functional/__init__.py`, `tests/functional/test_functional_e2e.py`, `tests/performance/__init__.py`, `tests/performance/test_performance_benchmarks.py`, `tests/simulation/__init__.py`, `tests/simulation/test_emulation_simulation.py`, `tests/test_accel.c`, `tests/test_accel_backend.c`, `tests/test_adaptive.c`, `tests/test_agent.c`, and 25 more.

## Documented test commands

These commands are reproduced from the inspected root README or contributing guide:

```bash
cmake --build --preset linux-x64-release
```

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

```bash
cmake --build build --parallel
```

```bash
cmake -B build -DEAI_BUILD_TESTS=ON
```

```bash
ctest --test-dir build --output-on-failure
```

```bash
cmake --build build
```

```bash
cmake -B build -DEAI_PROFILE=smart-camera
```

```bash
cmake --build build --config Release
```

## Verification baseline

This inventory comes from `master` at [`abd678b6c627`](https://github.com/embeddedos-org/eAI/commit/abd678b6c627f8e960657d03fa6309d1448c7c37) and found 37 test-related paths among 345 files. Re-check the source tree when that commit is no longer current.
