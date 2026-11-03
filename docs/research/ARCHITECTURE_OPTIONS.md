# BlueWake Architecture Options

Living evaluation. Confidence: 0–10 that the approach can reach the goal
(retail Wind Waker running natively on Apple Silicon macOS + iOS/iPadOS,
JIT-free). Updated 2026-08-09 (iteration 2 — post source-inspection of dusk,
aurora, DolRecomp/ModernGekko, sp00nznet/ww).

> **Current synthesis changed 2026-08-13.** A second opinion based on newer
> SunPad and RecompCore evidence makes Approach B credible as a bounded,
> Mac-first experiment, though not a demonstrated product path. Approach A/D
> remains the cleaner long-term architecture. See
> `../archive/FEASIBILITY_REASSESSMENT_2026-08-13.md`.

Terminology used precisely (per project rules):
- **Matching decompilation** — reconstructed C/C++ that recompiles to
  byte-identical original PowerPC code (zeldaret/tww, zeldaret/tp).
- **Static recompilation** — mechanical ahead-of-time translation of original
  PowerPC machine code into compilable C ("generated machine-code
  translation"); output is a derived encoding of the original binary
  (DolRecomp, sp00nznet/ww, N64Recomp lineage).
- **Emulation** — runtime interpretation or dynamic translation of guest code
  (Dolphin). Excluded by project goal.
- **High-level emulation (HLE)** — replacing a guest subsystem (DSP
  microcode, SDK call) with a host-native reimplementation at an API/behavior
  boundary. (DuskDsp is HLE of DSP *semantics*; acceptable and proven.)
- **Hardware compatibility layer** — clean host implementations of GameCube
  services that reconstructed game code links against (Aurora + Dusk's OS
  layer).
- **Native host code** — new platform code (Metal via Dawn, SDL3 shell,
  touch UI).

---

## APPROACH A — Matching TWW decomp → portable host (the "Dusk model")

**Status after inspection: architecture fully de-risked by a shipped
precedent; blocked solely by upstream decomp completeness.**

- **Proven by Dusk (verified in source, not press):** 100% recompiled C/C++,
  zero PPC translation; all RELs statically linked, dynamic linker compiled
  out; Aurora GX→Dawn→Metal; DuskDsp host-float DSP semantics; SDL3 iOS
  shell with live iOS CI; CC0 license. See DUSK.md, AURORA.md.
- **Cost model now known:** ~62k changed lines over the TP decomp
  (BE<T> ×1,058, OffsetPtr ×238, AVOID_UB, heap growth, ABI shims) — this is
  the transferable recipe BlueWake repeats over tww's tree.
- **Blocker:** tww at 73.4% matched code / 80.0% matched functions
  (2026-08-08), velocity +2.3→3.8 pts/mo, naive ETA Q2–Q3 2027. The
  unmatched ~7.9k functions exist only as original PPC asm → cannot be
  compiled for arm64.
- **Mitigations discovered:** (1) Dusk's dvd_asset.cpp pattern reads
  un-decompiled DATA from the user's disc at runtime — data completeness is
  not a blocker; (2) a port does not strictly need *matching* code, only
  *equivalent* code — remaining functions could in principle be decompiled
  non-matching ("functionally equivalent") faster than matching, at accuracy
  risk; zeldaret would still be the trunk.
- **iOS:** best possible fit (fully AOT, static, proven sideload pipeline).
- **Biggest unknown:** where exactly the unmatched 26.6% lives (per-subsystem
  analysis in flight) and whether critical-path subsystems (player, stage,
  JAudio1) are in the laggard set.
- **Confidence: 9/10 conditional on decomp completion; 2/10 for shipping
  something playable *this quarter*.**

## APPROACH B — Full static recompilation (DolRecomp/ModernGekko)

**2026-08-09 status after inspection: REJECTED as foundation. Superseded for
prototype purposes on 2026-08-13.**

- ModernGekko's only working runtime configuration **is Dolphin** (fork with
  a StaticRecomp CPU core + load-bearing ARM64 JIT fallback) — i.e.
  conventional whole-system emulation under the hood, explicitly excluded by
  the project goal, and impossible JIT-free on iOS.
- DolRecomp's translator is real and arm64-clean (16/16 tests pass locally,
  E5) but its C backend has no function recovery (4096-instr chunk switches,
  dispatcher exits, per-access call+swap) and its REL model resolves
  relocations to build-time-invented addresses — no OSLink/OSUnlink story for
  a 416-REL game. LLVM backend is x86-64-only.
- GPLv3 (Dolphin provenance) conflicts with signed iOS distribution.
- Independent confirmation of where pure recomp stalls: sp00nznet/ww (E3)
  recompiled 100% of the WW DOL yet needed Dolphin memory-dump seeding and
  frame-gate bypass; stalled pre-gameplay, no audio, no controllable Link.
- **Confidence: 1/10 for BlueWake's stated goal.** (Fine as *research
  instrumentation* on macOS: e.g. differential traces.)

### 2026-08-13 second-opinion update

- SunPad reportedly demonstrates AOT-recompiled regions on iPhone/iPad
  through the Dolphin-derived runtime with runtime PowerPC JIT disabled,
  interpreter fallback, and a portable software vertex loader.
- Newer DolRecomp/RecompCore reportedly provides batch REL translation,
  REL metadata, and runtime-to-AOT address remapping. BlueWake still needs a
  composite DOL+REL packager and scalable active-REL registration.
- The architecture is not entirely emulation-free; it is generated AOT game
  code plus a compatibility runtime and interpreter fallback.
- GPL compliance and Apple distribution need separate review, but GPL itself
  does not prevent code signing.
- No Wind Waker module has been built or run, so this evidence supports only
  a gated prototype, not a feasibility or performance claim.

**Current confidence:** sufficient to justify Track A0–A4 measurements;
insufficient to commit to an iPad product.

## APPROACH C — Hybrid: matched decomp (native) + static recomp (residual)

**Status after inspection: rejected at function granularity; reframed.**

- The two worlds are memory-model incompatible: natively-compiled decomp code
  uses host-endian 64-bit structs; recompiled code uses a big-endian guest
  RAM byte-array + PPCContext. Sharing the live game heap between them
  requires marshalling at every boundary — with ~40k functions and
  pervasive shared globals (g_dComIfG_gameInfo etc.), there is no viable cut
  line. No existing tool implements this; N64Recomp's "patches" solve the
  reverse problem (native→guest via recompiling patch code INTO the guest
  model), which doesn't help a native-first port.
- **Surviving hybrid ideas (recorded for the plan):**
  1. Use the decomp's complete symbol/type knowledge, not its code, to
     instrument a *reference* build (Dolphin traces) for differential
     debugging of the port.
  2. Dusk's dvd_asset pattern (runtime extraction of original DATA blobs).
  3. If, near the end, a handful of functions resist matching, write
     *functionally equivalent* C by hand — same thing zeldaret calls
     NonMatching; no recomp machinery needed.
- **Confidence as originally conceived: 1/10. As reframed (A + tricks): folded
  into A.**

## APPROACH D — Reuse Dusk/Aurora/borealis as BlueWake's host substrate

**Status after inspection: CONFIRMED as the substrate for Approach A.**

- License-clean (CC0 + MIT), separable (aurora/borealis are standalone
  repos; dusk's OS/audio/UI layers are structurally game-agnostic with ~90%
  reuse), Apple-proven (macOS arm64 CI, iOS CI, shipped IPA).
- WW-specific deltas identified and bounded: GXPeekARGB (Picto Box), JAudio1
  vs JAudio2, TWW's earlier JSystem revision, THP usage check, 416-REL
  static-link config, touch layout & bundle rebrand.
- **Confidence: 9/10** (as substrate; not standalone).

## APPROACH E — Newly discovered superior approach

None found. Adjacent-ecosystem sweep (ExpansionPak org, GameCubeRecompiled,
gbatemp/NeoGAF threads) surfaced no fifth architecture. The N64 "recomp+RT64"
lineage does not transfer: its runtime assumptions (flat RDRAM, no RELs,
LLE RSP microcode replaced per-game) are N64-specific.

---

### Current synthesis (2026-08-09, iteration 2)

The end-state architecture is settled beyond reasonable doubt:
**zeldaret/tww matching decomp, ported Dusk-style, on Aurora + borealis +
Dusk's game-agnostic runtime pieces, statically linked, Metal via Dawn,
sideloaded on iOS.** No other route reaches "native, non-emulation, JIT-free
iOS" with current technology.

The only open feasibility variable is **timing**, governed by tww matching
progress (73.4% → 100%, ETA ~Q2-Q3 2027) and by which subsystems lag.
Remaining research question: per-subsystem completeness → determines whether
BlueWake is WAIT or PROCEED-with-phased-start.

### Current synthesis (2026-08-13, iteration 3)

Keep two tracks. Run a bounded, private, Mac-first DolRecomp/RecompCore
prototype to measure full DOL+REL translation, composite packaging, active REL
lifecycle, authentic no-runtime-PowerPC-JIT boot, fallback burden, and speed.
Retain `zeldaret/tww` plus Aurora/Dusklight as the more maintainable long-term
source-native route. Do not authorize iPad product work until the macOS track
reaches controllable gameplay and a real transition; do not commit to a full
product until physical-iPad original-speed performance is measured.
