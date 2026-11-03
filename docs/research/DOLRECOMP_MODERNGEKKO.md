# DolRecomp / ModernGekko Assessment (@ 48c4ef11 / 048c426b, 2026-08-03)

Condensed from full source inspection + local build (2026-08-09). Both repos
~2 months old (first commits 2026-06-09). DolRecomp GPL-3.0; ModernGekko
GPL-3.0-or-later (forced by Dolphin).

> **Reassessment note (2026-08-13):** This document preserves the original
> pinned-SHA audit, but its final rejection is superseded by
> `../archive/FEASIBILITY_REASSESSMENT_2026-08-13.md`. A newer second opinion reports
> a demonstrated no-runtime-PowerPC-JIT iOS mode in SunPad and existing
> RecompCore REL address remapping. Those facts make a bounded prototype
> credible. No Wind Waker module or runtime has yet been tested.

## DolRecomp (the recompiler) — genuinely solid translator, naive backend

- Inputs: DOL, REL (file or folder), RPX; optional linker MAP (cosmetic
  symbol #defines only — does NOT drive codegen). Outputs: split C11 chunk
  files + manifest (portable backend), or LLVM objects (**hard-gated to
  x86-64 Linux/Windows** — `llvm_backend.cpp:83-89`; unusable on Apple).
- CPU model: flat `CPUState` struct; paired singles as two f64 arrays;
  FPSCR/NaN/FPRF/rounding modeled carefully (incl. Gekko 25-bit frC truncation,
  pinned by tests); `fesetround` host rounding — ARM64-clean. 236+ opcodes,
  all ps_* and psq_* covered. 9,184 lines of tests; 16/16 pass on Apple
  Silicon (E5). macOS universal CI.
- **Backend architecture (the weak half):** NO function-boundary recovery —
  fixed 4096-instruction chunks, each entered through a 4096-case
  `switch(ctx->pc)`; cross-chunk branches exit to a runtime dispatcher; every
  memory access is a runtime call with 4 range checks + byte-at-a-time
  big-endian swap (no __builtin_bswap); every FP op an out-of-line call.
  Guest RAM = calloc'd 24 MB big-endian buffer. Correct-first, slow-first
  design; WW's ~2 MB .text → est. hundreds of MB of generated C (unmeasured).
- **Fallback is structural:** unknown opcodes, unhandled SPRs, `dcbst/dcbf/
  dcbi/icbi` (C backend), and dispatcher misses route to
  `ppc_fallback_instruction` / return-0 — the runtime is EXPECTED to cover
  them. SMC is detected (smc.txt report) but explicitly unhandled.
- **REL support exists but is AOT-fixed:** relocations resolved at recompile
  time to build-time-invented sequential base addresses (`next_rel_base`);
  runtime `OSLink` at arena-dependent addresses is not modeled; no
  OSUnlink/reload story; cross-REL imports require same-batch compilation;
  the port tool (`moderngekko-port`) never invokes REL recompilation at all.

## ModernGekko (the "runtime") — it is Dolphin

- `Config::SetBase(MAIN_CPU_CORE, PowerPC::CPUCore::StaticRecomp)` +
  `BootManager::BootCore` of an extracted disc: **normal Dolphin boot with
  recompiled native code substituted per-block, Dolphin ARM64 JIT as the
  load-bearing fallback** (PROVENANCE.md: "yielding fallback jit").
- Graphics = Dolphin VideoCommon (Vulkan→MoltenVK on macOS). Audio = Dolphin
  DSP HLE/LLE. DVD/PAD/CARD/VI = Dolphin. The first-party "LegacyRuntime"
  (own GX decode, AI DMA, DI) is a renderer-less, DSP-less prototype linked
  into tests only.
- iOS: zero support (no UIKit/TARGET_OS_IPHONE anywhere). Mod ABI is
  AOT-safe (hash-map dispatch, no RWX), but mod delivery is dlopen-only.
  One good pattern: `ModuleSource::AttachedDescriptor` supports statically
  linking the recompiled game into the binary.
- Hall of Fame: Luigi's Mansion title screen etc. — all DOL-only or
  single-REL games. No multi-REL title demonstrated.

## Original verdict for BlueWake (superseded 2026-08-13)

**Rejected as foundation.** Three independent disqualifiers for the stated
goal (native, non-emulation, iOS-capable):
1. The only working runtime configuration is Dolphin-with-precompiled-blocks
   — i.e. conventional whole-system emulation under the hood, with a JIT
   fallback that cannot exist on iOS.
2. No credible REL runtime model for a 416-REL game.
3. GPLv3 + Dolphin provenance conflicts with signed iOS distribution.

**Salvageable pieces:** DolRecomp's decoder + Gekko semantics library
(~3.5k lines, arm64-clean, well-tested) and the AttachedDescriptor
static-link pattern — useful IF BlueWake ever needs a bespoke AOT translator
for residual unmatched functions. The sp00nznet/ww experiment (E3)
independently confirms where pure recomp stalls: authentic initialization,
interrupts, DSP — the runtime, not the translation.

## 2026-08-13 corrections and remaining gaps

The newer reassessment narrows the original three disqualifiers:

1. **Runtime PowerPC JIT is not architecturally mandatory on iOS.** SunPad
   reportedly runs AOT game regions through ModernGekko/RecompCore with the
   PowerPC JIT disabled, interpreter fallback, and the portable software
   vertex loader. The BlueWake question is fallback correctness and hotness,
   not whether a no-JIT mode exists.
2. **REL integration exists below the product layer.** Current DolRecomp
   reportedly supports REL folders, stable AOT virtual addresses,
   self-relocations, and same-batch cross-module imports. RecompCore reportedly
   maps runtime-loaded REL sections back to AOT-linked ranges. Missing work is
   composite DOL+REL generation/packaging, metadata consistency, and lifecycle
   validation.
3. **GPL is not a code-signing prohibition.** It creates compliance and
   source-distribution obligations and may constrain distribution choices.
   User-signed/sideloaded builds are technically possible. Public distribution
   requires separate legal and license review.

The current active-REL discovery path reportedly scans guest RAM for headers
matching compiled descriptors. That may not scale across roughly 415 RELs.
Measure it, then prefer a bounded `OSLink`/`OSUnlink` registration seam or an
explicit runtime-section mapping API while retaining RecompCore's generic
address translation.

Reported reassessment revisions: DolRecomp
`fa0cf619e8d7eb8cba7eaf55267a12caaebb46aa`, ModernGekko
`884c20505d7179160f8bb01d9db0f723c53b09cb`, and RecompCore
`e13ab348f13cd67879f6db6e9d7185410f8f62c6`. These newer claims have not been
reproduced locally and must be refreshed before implementation.
