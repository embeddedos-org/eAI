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

## Additions — 2026-10-12 (verified in-window, October 2026)

| Silicon (product) | Compute | CPU | Track-1 tier mapping | Status |
|---|---|---|---|---|
| NXP Ara240 discrete NPU | 40 eTOPS (INT4/INT8/mixed), up to 16 GB LPDDR4, PCIe Gen4 + USB 3.2 host, secure boot + HW root of trust, ~6.6 W, M.2 2280 / USB module | Host: FRDM i.MX 8M Plus / i.MX 95 class | Application-processor / large tier — the discrete-NPU target for the `ebuild quantize --target aie` class; runs LLMs/VLMs at the edge (eosllm's "large models locally" hardware story) | Shipping (Mouser, Oct 2026) |
| Nordic nRF54LC10A | No NPU (CPU inference only) | 128 MHz Cortex-M33 + 128 MHz RISC-V coprocessor, 1 MB NVM / 192 KB RAM, BLE 6.x + Channel Sounding, 802.15.4 (Thread/Matter), sub-50 nA hibernate | MCU ternary/CPU tier — the low-power connectivity end of the tier ladder | Announced ~Oct 9 2026 |
| Ezurio BL54LM20A/B modules | On-chip neural accelerator | nRF54LM20: M33 + RISC-V @ 128 MHz, 2 MB / 512 KB, BLE 6.x, Thread/Matter, NFC, PSA L3 | MCU ternary tier with neural accelerator; Zephyr-first (nRF Connect SDK) — a module-level HAL reference | Modules announced Oct 2026 |
| Nuvoton NuMaker-GestureAI-M55M1 kit | Ethos-U55 @ 220 MHz, 111 GOPS | Cortex-M55 @ 220 MHz, 1.5 MB SRAM / 2 MB flash, TrustZone, PSA L2; kit: GC0308 camera + digital mic + microSD | MCU ternary tier — **the Ethos-U55 reference NPU** for the accelerator HAL (cheapest credible Ethos-U55 board) | Kit, Oct 2026 |
| Nuvoton MA35D0 "Chili Pro" | No NPU | Dual Cortex-A35 @ 650 MHz + Cortex-M4 @ 180 MHz, RJ45/USB, 45×45 mm | Application-processor heterogeneous tier (A-class + M-class split mirrors eos service placement) | Eval board, Oct 2026 |

Evidence: electronicdesign.com (Ara240, Mouser shipping notice Oct 5 2026);
electronics-lab.com (nRF54LC10A, BL54LM20); cnx-software.com (M55M1 kit,
MA35D0 Chili Pro) — all surfaced in the 2026-10-11 research sweep.

Notes:

- Ara240 is the first *discrete* NPU on the roster: it defines the top of
  the tier ladder the HAL abstracts over, and the first target where
  capability-scoped inference (Track 1 trust work) has a hardware root of
  trust to bind to.
- The M55M1 kit is the Ethos-U55 reference: CMSIS-NN / Ethos-U backend
  work in the accelerator HAL should be validated against this board
  before any other MCU-tier NPU.
- **Known silicon (pre-October 2026):** Synaptics SYN765x and TI
  TinyEngine-NPU MCUs (MSPM0G5187 / AM13Ex) are March 2026 parts — valid
  roster targets, but not new this week; they get full rows when a board
  using them enters the board queue, not before.
