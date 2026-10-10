# Track 1: silicon roster

Shipped silicon targets for the tightly-coupled AI tiers — the actual
parts the accelerator HAL tiers, the quant roster, and the power-aware
model selection logic target. One row per part, kept current as new
silicon ships. (Track 1: AI as an OS subsystem.)

| Silicon (product) | Compute | CPU | Track-1 tier mapping | Status |
|---|---|---|---|---|
| CVITEK CV1842H-P (AIMORELOGY Ovis) | 1.5 TOPS INT8 + BF16 NPU, AI-ISP | 1× Cortex-A53 @ 1.1 GHz + 1× RISC-V C906 @ 800 MHz | Vision-inference tier (1080p night vision, on-device) | Shipped — open-source AI vision camera module |

Evidence: https://www.cnx-software.com/news/kickstarter/

Notes:

- The Ovis is the first roster entry because it is a *shipped,
  open-source* vision-inference target: the NPU + AI-ISP combination is
  what the eAI runtime's vision pipelines need below the application-
  processor class.
- Tier mapping is to the accelerator-HAL tiers in
  [`accelerator-hal-ternary-tier.md`](accelerator-hal-ternary-tier.md):
  the CV1842H-P's NPU sits above the MCU ternary tier (application-
  processor class, int8/BF16), which is exactly the gap the tiering is
  meant to bridge — the HAL picks the deepest tier the hardware and the
  model both support.
- Power: the roster is read together with the **tokens-per-watt** metric —
  a part earns its row by being measurable, not by its TOPS headline.
