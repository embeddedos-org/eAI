# eAI — Embedded AI

[![CI](https://github.com/embeddedos-org/eAI/actions/workflows/ci.yml/badge.svg)](https://github.com/embeddedos-org/eAI/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

On-Device Neural Network Inference Engine.

---

## Simulation & Analytics

### Real-Time Emulation Dashboard
Below is the real-time simulation dashboard generated from the test suite. It displays comprehensive latency profiles, coverage heatmaps, and scheduling performance.

![Emulation Dashboard](docs/screenshots/eai_simulation.png)

### Unified Organization Health Matrix
We continuously benchmark eAI — Embedded AI against the entire EmbeddedOS ecosystem to check interoperability.

![Overall Dashboard](docs/screenshots/overall_dashboard.png)

---

## Product Video

See eAI — Embedded AI in action! Watch our high-fidelity product demonstration and marketing video:

> 🎥 **[Watch the eAI — Embedded AI Product Video](docs/videos/eai_marketing.mp4)**

---

## Architecture

- **Domain**: C • NPU • INT8 Quantization

---

## Test Suite

The suite is organised into four categories — unit, functional end-to-end,
performance, and hardware simulation.

> Coverage is not currently measured, so no coverage figure is published here.
> Live build status is the CI badge at the top of this file.

To run the entire suite locally:
```bash
python run_all_tests.py
```

---

## 📜 License & Compliance

Licensed under the MIT License. Aligned with ISO/IEC 25000 software quality standards.
