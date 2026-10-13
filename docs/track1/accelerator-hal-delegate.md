# Track 1: accelerator HAL delegate interface

**Status:** design (2026-10-13), pre-implementation. Part of the
tightly-coupled AI track: the accelerator HAL abstracts inference
backends the way the peripheral HAL abstracts hardware. This document
specifies the *delegate* shape of that HAL — the interface a backend
plugs into — building on the tier design in
[`accelerator-hal-ternary-tier.md`](accelerator-hal-ternary-tier.md),
which it does not restate.

## Why a delegate

The pattern is the one LiteRT (formerly TensorFlow Lite) proved: the
runtime owns the model and the call sequence; a **delegate** owns one
backend's lifecycle — it claims the subgraphs it can run, prepares
them against its hardware, executes them, and releases them. The
runtime never calls a vendor API directly, and a backend never sees
more of the model than the runtime hands it. That boundary is what
makes capability-scoped inference enforceable: the delegate is where
the scope check physically happens.

## The interface

One delegate type for every backend, MCU tier to large tier:

```c
typedef struct eai_delegate eai_delegate_t;

/* Capability query: which tiers / ops / precisions this backend claims,
 * plus its trust features (secure boot, OTP, signed-model support). */
int  eai_delegate_query(const eai_delegate_t *d, eai_backend_caps_t *out);

/* Create: bind the delegate to one pinned model artifact
 * (name@sha256:…, see "Binding" below). No model bytes are parsed
 * before the artifact's digest is verified. */
int  eai_delegate_create(const char *model_pin, const eai_caps_t *granted,
                         eai_delegate_t **out);

/* Prepare: claim and compile the subgraphs this backend can run.
 * Anything unclaimed falls back per the tier rules — never silently
 * to a backend the model did not declare. */
int  eai_delegate_prepare(eai_delegate_t *d, const eai_model_t *m);

/* Invoke: run the prepared graph over caller-owned tensor arenas. */
int  eai_delegate_invoke(eai_delegate_t *d, eai_tensor_io_t *io);

/* Release: free backend state; arenas stay caller-owned. */
void eai_delegate_release(eai_delegate_t *d);
```

Rules the interface carries:

- **Fail closed.** If a model requires a backend that is absent, or a
  capability the host did not grant, `create`/`prepare` returns an
  error — the model does not run degraded on an undeclared path.
- **Declared caps only.** The delegate receives exactly the
  capabilities granted in the model's capability manifest (eosllm's
  manifest format); it cannot widen them, and `invoke` checks tensor
  access against them.
- **Caller-owned memory.** As with the inference-service arena
  contract, tensor arenas belong to the caller; a delegate maps them
  by handle and never retains raw pointers past `invoke`.

## Backends

| Backend | Tier | Notes |
|---|---|---|
| Ethos-U55 | MCU ternary / int8 | First backend. The M55M1 kit on the [silicon roster](silicon-roster.md) is the reference target; the delegate wraps the Ethos-U driver lifecycle. |
| CMSIS-NN | MCU int8 / ternary | Portable Cortex-M kernels; the baseline every MCU target can claim. |
| ESP-NN | MCU int8 | ESP32-S3/P4 optimized kernels, including the SPI scale-out ternary path from the tier design. |
| Ara240 | Large | Discrete NPU (PCIe/USB host); the delegate claims whole-graph execution where the model fits. |
| Zephyr 4.4 TFLM module | Hosted | The TensorFlow Lite Micro module shipped with Zephyr 4.4, for Zephyr-hosted targets. The delegate owns the TFLM interpreter lifecycle (arena sizing, op resolver, invoke) behind the same interface. This is a **hosted** backend — it runs under an RTOS host, not bare metal — so its trust and arena rules are the host's, stated explicitly rather than assumed. |
| CPU fallback | All | Always present. Scalar/kernel fallback so any model that declares it can run anywhere; a model that does *not* declare CPU fallback must fail closed instead of silently landing here. |

## Binding: registry pins and capability manifests

A delegate never loads "a file". It loads a **pinned artifact**:

- ebuild's model registry names artifacts as `model@sha256:…`; the
  pin is what `eai_delegate_create` takes, and the digest is verified
  (`ebuild model verify`) before any weight byte is parsed.
- eosllm's capability-manifest format declares, next to the pin, the
  capabilities the model may use and the backends it may run on. The
  loader refuses a model whose manifest requests a capability the
  host policy denies; the delegate receives only the granted set.
- Model signing (the eos #162 envelope) covers the artifact and its
  manifest together, so a swapped manifest is as detectable as a
  swapped weight.

## Cross-references

- [`accelerator-hal-ternary-tier.md`](accelerator-hal-ternary-tier.md) — the tier design this interface serves.
- [`silicon-roster.md`](silicon-roster.md) — the parts these backends target.
- ebuild `docs/quantize.md` — the model registry that produces the pins.
- eosllm `docs/track1/capability-manifest.md` — the manifest a delegate enforces.
