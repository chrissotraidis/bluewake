# BlueWake Feasibility Report

**Question:** Can the original retail GameCube *The Legend of Zelda: The Wind
Waker* (GZLE01) run natively on Apple Silicon macOS, iOS, and iPadOS via
matching decompilation, ahead-of-time static recompilation, or a hybrid —
without emulation, without JIT, and without shipping Nintendo data?

**Research date:** 2026-08-09. All claims below are backed by pinned-SHA
source inspection and local experiments on an Apple Silicon Mac (macOS 26.5,
Xcode 26.6); see `docs/research/` for evidence. Anything not verified is
marked UNVERIFIED.

> **Superseded decision note (2026-08-13):** This report is preserved as the
> original audit. A newer second opinion corrects several categorical claims
> about no-JIT iOS execution, REL runtime mapping, and GPL/signing, and changes
> the current decision from blanket WAIT to a bounded Mac-first static-recomp
> prototype. See [FEASIBILITY_REASSESSMENT_2026-08-13.md](FEASIBILITY_REASSESSMENT_2026-08-13.md).
> Wind Waker gameplay and performance remain unproved.

---

## Original project decision (recorded 2026-08-09)

After reviewing this report, the project sponsor set a hard constraint: the
complete game must be achievable **entirely in-house, in a matter of weeks**.
The evidence below shows that constraint cannot be met by any investigated
architecture: ~4,250 functions (~10.8%) of the game exist only as original
PowerPC machine code with no reconstructed source, and producing correct
source for them is reverse-engineering work measured in months-to-years
(TP's equivalent took ~5.5 years community-wide), not weeks. The
no-decompilation shortcut (static recompilation) was independently
disqualified (see Rejected architectures).

**Therefore the 2026-08-09 project decision was WAIT (Exit Condition B).** The technical
verdict of the research itself — bounded-prototype-leaning-PROCEED on a
~1-year horizon — stands unchanged and is preserved below for whenever the
constraint is relaxed or the upstream decomp closes the gap. Concrete
re-evaluation triggers: `WAITING_FOR.md`.

## Executive conclusion (technical, schedule-unconstrained)

**Verdict: BOUNDED PROTOTYPE REQUIRED — strongly leaning PROCEED.**

The architecture question is settled. There is exactly one credible route,
and it is proven end-to-end *for the sibling game*: compile the
**zeldaret/tww matching decompilation** natively for arm64, statically link
all 415 REL actor modules into one binary, and run it against the
**Aurora** GameCube SDK/GX compatibility layer (GX→WebGPU/Dawn→**Metal**)
plus the game-agnostic runtime pieces Dusklight built for Twilight Princess
(OS threading, VI retrace, host-float DSP audio, SDL3 app shell, touch
controls, iOS packaging). Dusklight shipped precisely this stack for
Twilight Princess in May 2026 on Windows/macOS/Linux/Android/**iOS**, from
the completed zeldaret/tp decomp, under CC0 — and its source is public and
inspectable. Every layer of that stack builds and passes its test suite on
this machine today (Aurora: 238/238 tests, arm64).

What is NOT settled is timing: **zeldaret/tww is not finished.** At
2026-08-08 it stands at 73.4% matched code / 80.0% matched functions, and —
the number that actually matters for a port — **4,250 of 39,324 functions
(10.8%) have no reconstructed source at all.** Those gaps are heavily
concentrated in NPC actor modules (3,459 stubs across 102 of 414 actor
RELs), the menu/sea-chart/HUD cluster (~460), and the JAudio1 audio stack
(166). Meanwhile the *critical execution path is essentially complete
today*: the boot/machine layer, the entire process framework, the REL
infrastructure, JKernel/JUtility/J2D, stage loading, saves, the scene
classes for logo/title/gameplay, and **the entire player actor (34,554
lines, zero stubs)**.

Because the sibling precedent is decisive but Wind-Waker-specific execution
has never been demonstrated, the honest exit condition is a **bounded
prototype**: a time-boxed port of the completed spine that must reach the
authentic title screen and controllable Link in a real stage, with the gap
areas stubbed. Success flips the verdict to PROCEED (all remaining work is
enumerable engineering plus an upstream tail that is closing at ~2–4
percentage points/month). Failure modes are enumerable and would flip to
WAIT with precise triggers.

## Confidence

- End-state architecture is correct: **very high (9/10)** — proven by a
  shipped sibling product, verified in source, license-clean.
- Prototype succeeds within its time box: **high (~75–85%)** — the spine it
  exercises is the most-complete part of the decomp; the recipe (Dusk's
  ~62k-line portability pass) is public; the biggest unknowns (JAudio1 boot
  entanglement, TWW-specific engine deltas) have known fallbacks.
- Shippable, complete game in 2026: **low** — gated on upstream decomp tail
  (naive ETA Q2–Q3 2027) unless the project writes functionally-equivalent
  C for the remaining stubs itself.

## What has been experimentally proved (this machine, this week)

| # | Result |
|---|---|
| E1 | All 8 candidate repos cloned & pinned; all public, active |
| E2/E4 | tww progress: 73.42% code, 79.99% funcs; velocity +2.3→3.8 pts/mo (accelerating post-Dusk) |
| E3 | sp00nznet/ww (pure WW static recomp): full-DOL translation worked; stalled pre-gameplay needing Dolphin memory dumps — falsifies "recomp alone" |
| E5 | DolRecomp builds arm64, 16/16 tests — but its runtime (ModernGekko) is a Dolphin fork w/ JIT fallback → rejected |
| E6 | Aurora builds arm64, 238/238 tests; prebuilt Dawn for darwin-arm64 AND ios-arm64 |
| E7 | WW topology: main.dol + 415 RELs; REL loader 100% decompiled; no self-modifying code |
| E8 | tww build system runs natively on this Mac (dtk-macos-arm64); blocked only on user disc |
| E9 | No local disc image present; disc-dependent verification deferred |

## What is inferred but not proved

- That tww's completed spine actually boots and plays after a Dusk-style
  portability pass (BE<T>/OffsetPtr/AVOID_UB/heap sizing). Inference base:
  Dusk did exactly this to the same engine family; tww's spine is
  stub-free. **This is what the prototype proves.**
- That the ~3,450 written-but-not-byte-matching functions are behaviorally
  correct (they compile, but matching is the only proof upstream uses).
- JAudio1 (WW's audio stack, ≠ TP's JAudio2) can sit on DuskDsp's hardware
  model with moderate effort. 166 upstream stubs remain in that area.
- iOS memory ceiling: Dusk reserves 256 MB MEM1 + heaps; fine on modern
  iPhones/iPads, but untested for WW's allocation patterns.
- TwilitRealm is rumored to target Wind Waker next (UNVERIFIED, press only)
  — a strategic duplication risk, also potentially an accelerant.

## Chosen architecture

**Matching decompilation → native arm64 compilation → hardware
compatibility layer → native host code.** Precisely:

1. Game code: zeldaret/tww reconstructed C/C++ (CC0), all 415 RELs compiled
   statically, dynamic linker stubbed (dusk's `c_dylink` pattern).
2. Compatibility layer: Aurora (MIT) — GX (FIFO-accurate, full TEV incl.
   indirect, EFB copies verified against WW's actual call sites), DVD (nod;
   user's ISO/RVZ read in place), CARD, PAD, MTX, OS heap/arena — plus
   Dusk-pattern OS threads/mutex/VI-retrace/message-queues.
3. Audio: JAudio1+JAZelAudio from tww source over a DuskDsp-style host-float
   DSP semantic model (HLE at the DSP boundary; no microcode execution).
4. Host: SDL3 shell, Metal via Dawn, GameController+CoreHaptics, RmlUi/ImGui
   UI, borealis app services; static single binary on iOS (BUILD_SHARED_LIBS
   OFF, no JIT, no runtime code loading — Dusklight's exact preset).
5. Game data: never shipped; user-supplied disc image, xxh3-verified;
   un-decompiled data blobs pulled from the user's own DOL/RELs at runtime
   (dusk `dvd_asset` pattern).

## Rejected architectures (and why)

- **Pure static recompilation (DolRecomp/ModernGekko):** the only working
  runtime is a Dolphin fork whose correctness model *requires* a JIT
  fallback (impossible on iOS; disqualified as disguised emulation
  regardless); REL relocations are baked to invented addresses (no runtime
  OSLink model for a 415-REL game); GPLv3; x86-64-only optimizing backend.
  Independent corroboration: sp00nznet/ww recompiled 100% of the WW DOL yet
  never achieved authentic initialization. Full analysis:
  `docs/research/DOLRECOMP_MODERNGEKKO.md`.
- **Fine-grained hybrid (decomp + recomp interleaved):** natively-compiled
  decomp code (host-endian, 64-bit) and recompiled code (big-endian guest
  RAM, PPCContext) cannot share the live game heap without marshalling at
  every one of thousands of boundaries; no tool exists; N64Recomp's patch
  mechanism solves the reverse direction only.
- **Emulator embedding / streaming / WebView:** excluded by definition of
  the goal.

## Remaining major risks (ranked)

1. **Upstream tail** — 4,250 sourceless functions. Mitigations: velocity
   (+3.8 pts/mo), gaps concentrated in stub-able side content, option to
   write functionally-equivalent C in-project (feeding matches back
   upstream), dvd_asset escape hatch for data.
2. **Audio (JAudio1)** — least-complete subsystem upstream AND the one
   library Dusk cannot lend. Fallback: ship prototype silent; schedule
   audio as its own phase; DSP semantic model ports from DuskDsp.
3. **Correctness of non-matching source** — playtest-driven; upstream
   matching progress steadily converts this bucket to proven.
4. **iOS memory/jetsam** — 256 MB guest-RAM pattern needs profiling on
   4 GB-class devices; `Increased Memory Limit` entitlement available for
   sideloaded builds.
5. **Bus factors** — Aurora is 58% one author; tww driven by one
   maintainer. Both healthy today.
6. **Legal** — architecture is the community-standard posture (CC0/MIT
   code, no assets, user disc), same as Dusklight, publicly tolerated for
   TP/SM64/OoT to date, but legally untested; a Nintendo enforcement action
   is a permanent background risk for ANY variant of this project.

## Platform feasibility summary

- **macOS (Apple Silicon):** fully feasible; every component already builds
  and self-tests natively here. Primary development target.
- **iOS:** feasible — proven by Dusklight's shipped IPA from the identical
  stack: all-static, AOT-only, Metal-via-Dawn, SDL3 lifecycle, touch
  controls injected via `PADSetVirtualStatus`, disc image via Files.app.
  Distribution is sideload (AltStore/SideStore, free-Apple-ID 7-day resign,
  Developer Mode) or EU alt-marketplaces — App Store is not realistic for
  Nintendo-derived code. This is a distribution constraint, not technical.
- **iPadOS:** same binary/family as iOS (UIDeviceFamily 1+2 in Dusk's
  Info.plist); external-controller and ProMotion support come from the same
  stack.

## Estimated project complexity

- Bounded prototype (Phase 1): ~4–6 engineer-weeks.
- To "TP-parity shippable 1.0" after prototype success: on the order of
  12–20 engineer-months of port engineering *plus* the upstream decomp tail
  (or equivalent in-project function reconstruction). Dusk's observable
  cost: ~62k changed lines over the decomp + ~50k lines of new port code,
  on top of a finished decomp.

## Exact first implementation milestone

See `IMPLEMENTATION_PLAN.md` PHASE 1 (the bounded prototype), and
`HANDOFF.md` for the first concrete task. Success criterion, verbatim:

> From a user-supplied GZLE01 disc image, an arm64 macOS binary compiled
> solely from tww reconstructed source + Aurora + new shim code reaches the
> authentic in-engine title screen (d_s_logo → d_s_title driven by original
> scene code), then loads sea stage room(s) with a controllable Link
> (original d_a_player code, original collision), and performs one
> authentic stage/room transition — audio may be silent, menus/NPCs may be
> stubbed, Picto Box may be broken.

**PROCEED trigger:** the milestone above within the time box (≤6 weeks).
**WAIT triggers:** (a) critical-path code turns out to require >~200
sourceless functions that cannot be stubbed without breaking init; (b) a
TWW-specific engine mechanism (e.g. JAudio1 boot handshake, DSP-sync wait,
ARAM dependency) blocks d_s_play entry and has no Dusk-recipe workaround;
(c) pervasive misbehavior traced to the ~3,450 unverified non-matching
functions at a rate that playtesting cannot triage. If WAIT: re-evaluate
monthly against decomp.dev (see HANDOFF triggers).
