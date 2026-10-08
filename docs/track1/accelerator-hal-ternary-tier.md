# Track 1: accelerator-HAL ternary tier

**Status:** design (2026-10-08), pre-implementation. Part of the
tightly-coupled AI track: AI as an OS subsystem, where the accelerator HAL
abstracts inference backends the way the peripheral HAL abstracts
hardware.

## The tiering

The accelerator HAL currently has two conceptual tiers for MCU-class
inference:

| Tier | Backends | Precision | Notes |
|---|---|---|---|
| Vectorized int8 | CMSIS-NN, ESP-NN | int8 weights/activations | The portable default. Every MCU ML SDK speaks it. |
| **Ternary** (new) | BitNet-1.58 kernels | 1.58-bit weights in {-1, 0, +1} | No FP MACs anywhere in the matmul path. |

The ternary tier sits *below* the int8 tier: it trades precision for the
complete elimination of floating-point multiply-accumulates, which is the
dominant energy cost on microcontrollers without an FPU data path worth
the name.

## Existence proof

September 2026: seven $4 ESP32-S3 chips run a **0.4B-parameter LLM in an
SPI daisy-chain** ($28 total) with 1.58-bit weights.

- https://byteiota.com/esp32-bitnet-llm-cluster/
- https://ai-beat.github.io/news/2026/09/esp32s3-bitnet-microcluster/

This collapses the "MCU-class LLM" question from research to parts list.
The HAL tier is designed around what that demo proves works:

- **Weights** in {-1, 0, +1}: the matmul becomes sign-select and add.
- **Embeddings** flash-resident (read-only, large).
- **KV caches** in PSRAM (ESP32-S3 class).
- **Scale-out** over SPI: the daisy-chain is the reference interconnect
  for multi-MCU inference; the HAL's partition map routes shards.

## HAL shape (proposed)

```
eai_accel_ternary_matmul(out, in, ternary_weights, n, k)
eai_accel_ternary_partition_map(shards, n_devices)   // SPI daisy-chain routing
```

Backends register capability flags (`EAI_ACCEL_TERNARY`,
`EAI_ACCEL_SPI_SCALEOUT`); the runtime picks the deepest tier the
hardware and the model both support. A model quantized with
`ebuild quantize --format bitnet-1.58` (see ebuild's
`docs/quantize-mcu-target.md`) declares its tier in the artifact, so the
HAL never has to guess.

## Metric: tokens-per-watt

Tiers are compared on **tokens-per-watt on the target board** -- not
model size, not perplexity alone. The int8-vs-ternary decision for a
given product is an energy decision first: a ternary build that is
smaller but hungrier than int8 on the same silicon is not a win on
battery-powered hardware. Power-aware model selection (the track-1
kernel-service work) consumes this metric directly.

## Relationship to capability-scoped inference

The ternary tier changes nothing about trust: a ternary model is still a
model, and capability-scoped inference still applies. The tier only
changes the arithmetic. Model signing via the #162 envelope covers
ternary artifacts identically.

## Cross-references

- ebuild `docs/quantize-mcu-target.md` -- the build step that produces ternary artifacts.
- `docs/track1/runtime-api.md` -- the inference runtime API this tier plugs into.
- eNI `docs/on-device-inference-reference.md` -- the in-domain evidence pack (Oido, esp32-gpio-llm).
