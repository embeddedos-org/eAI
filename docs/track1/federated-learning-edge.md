# Federated learning on the edge (Track 1, phase 3)

This is the phase-3 training-loop datapoint for tightly-coupled AI:
today eAI does inference on-device; this document records the existence
proof that on-device *training* is within reach.

## The result

Mutex-based federated learning (*Cluster Computing*, Oct 2026):
**TinyLlama fully fine-tuned across edge clients, 99.02%
adversarial-detection accuracy.** Full fine-tuning — not adapters, not
last-layer — running on a federation of edge devices, with adversarial
robustness as the measured outcome.

## Why it matters for Track 1

- **Phase 3's missing half is now evidenced.** The phased path has
  sim-to-real and on-device inference as near-term work; this result
  says the training loop can eventually live where the data is —
  devices fine-tune locally, the federation aggregates.
- **Pairs with the ternary tier.**
  [accelerator-hal-ternary-tier.md](accelerator-hal-ternary-tier.md)
  argues inference belongs below FP32; this argues the same hardware
  will eventually train. The two docs are the ends of the phase-3
  loop: train (federated) → quantize (ebuild `quantize`) → infer
  (ternary tier).
- **Trust follows the data.** Federated clients must be attested
  (measured boot, signed model updates) — the #162 envelope and the
  eBoot trust track apply to training updates exactly as they do to
  packages.

## Visibility

The **EDGE AI Foundation** announced 3 new working groups plus an LF
Edge strategic partnership (EDGE AI Taipei, Oct 22–23: physical AI,
edge security, SLM). eAI should consider participating — the ternary
tier and this training-loop note are the org's concrete contributions
to those working-group conversations.
