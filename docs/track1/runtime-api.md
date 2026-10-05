# eAI runtime API: the on-device inference service

**Status:** Design sketch (Track 1 — tightly-coupled AI).
**Date:** 2026-10-05
**Cross-links:** `eos` `docs/track1/accelerator-hal-profiles.md` (HAL tiers +
`eos_accel_backend_t`), `eosllm` `docs/track1/inference-service.md` (first
implementation), `eIPC` `docs/track1/zero-copy-tensor-ipc.md` (transport),
`eApps` manifest `ai_capabilities` (capability declarations).

## 1. Shape: a service, not a library

Today eAI is a library you link. Track 1 promotes inference to an **eos
service** (future home: `eos/services/ai/`), exposing a vendor-neutral API.
Vendor backends (CMSIS-NN, ESP-NN, ExecuTorch-style delegates) register
against the HAL's `eos_accel_backend_t`; applications never touch a backend
directly. The service owns model loading, arena allocation, dispatch, and
capability enforcement.

```c
/* Load + verify (Ed25519 envelope, #162) + parse. */
eai_model_t *eai_load_model(const eai_model_ref_t *ref);

/* Arena contract for the 520 KB-SRAM world: the caller sizes the arena,
   the service never mallocs on the hot path. */
eai_arena_t *eai_alloc_arena(size_t bytes);
void         eai_free_arena(eai_arena_t *arena);

/* Run one inference. Capability argument scopes peripheral access
   (see §3). */
int eai_infer(eai_model_t *m, const eai_capability_t *cap,
              const eos_tensor_t *in, eos_tensor_t *out);
```

`eai_load_model` refuses unsigned or capability-exceeding models before any
byte of weights is mapped. `eai_infer` takes input/output tensors the caller
owns — zero-copy where the transport (eIPC) already mapped them.

## 2. The Zephyr question, answered

Zephyr's pattern for on-device ML is an **ExecuTorch-module**: a
vendor-neutral core with pluggable NPU modules, rather than a bespoke kernel
inference service. Adopt that shape here, for five reasons:

1. **Backend churn is real.** CMSIS-NN, ESP-NN, vendor delegates, and AIE-class
   runners have incompatible lifecycles; a module boundary absorbs that.
2. **The HAL already draws the seam.** `eos_accel_backend_t` (dispatch +
   scratch-sizing + quantization descriptor) is exactly the module interface.
3. **Testability.** Modules can be validated against EoSim's fault-injection
   harness without booting the service.
4. **Footprint.** A `tiny`-only product links one module, not the service's
   full backend set.
5. **Precedent.** Copying the ecosystem's converged shape beats inventing a
   fourth one; eos's differentiation is the trust story (§3), not the
   module loader.

So: vendor-neutral service core + pluggable backend modules + HAL-typed
module interface. Not a bespoke kernel service.

## 3. Capability-scoped inference

The `eai_capability_t` argument declares what the model may touch —
`{ .camera = true, .mic = false, .npu = true }`. Declarations originate in
the app manifest (`eApps` `ai_capabilities`: `needs_camera`, `needs_mic`,
`needs_npu`, `model_ref` content-hash, `max_power_mw`) and are enforced two
ways:

- **Load time:** the service checks the envelope's capability claims against
  the manifest; mismatch → refuse.
- **Hardware:** TrustZone secure-peripheral attribution (camera secure, mic
  non-secure; inference service in the secure partition). The envelope
  claims map onto SAU/MPU regions, so an overreaching model cannot address
  the peripheral at all. See the eos HAL doc §4 for the full pattern.

## 4. What eosllm implements (M1)

`eosllm` becomes the first inference-service implementation against this API
(see `eosllm` `docs/track1/inference-service.md`): its loader becomes
`eai_load_model`, its allocator adopts the arena contract, and its current
module set maps onto backend modules. Gaps for M1 (enumerated in the eosllm
doc): envelope verification in the loader, arena allocator, capability
plumbing from manifest to `eai_infer`.
