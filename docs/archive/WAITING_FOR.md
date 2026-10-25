# BlueWake — WAITING_FOR

> **SUPERSEDED STATUS (2026-09-02):** This file preserves the original wait
> decision and prototype promotion gates. Active work is governed by
> `docs/archive/status/CURRENT.md`, `docs/archive/status/BLOCKERS.md`, and `docs/GOAL_LOOP.md`.
> macOS Route B is active; iOS/iPadOS remains deferred until macOS is stable.

Original decision 2026-08-09: **WAIT on all implementation.**

Reassessed decision 2026-08-13: **do not wait on the bounded Mac-first
mechanical prototype. WAIT on full-product commitment and iPad product work**
until the gates below pass. See `FEASIBILITY_REASSESSMENT_2026-08-13.md`.

The source-native decompilation/Aurora track remains waiting on upstream
coverage or a deliberate commitment to reconstruct the missing behavior. The
static-recompilation track can measure its own feasibility now.

## Active prototype promotion gates

1. Private `GZLE01` extraction produces an authoritative DOL+REL manifest.
2. Every executable image translates and every fallback/relocation/SMC issue
   is enumerated.
3. A composite arm64 module has complete DOL/REL dispatch coverage.
4. REL link, unlink, reload, address reuse, and indirect calls pass synthetic
   and runtime tests with bounded mapping cost.
5. Normal no-runtime-PowerPC-JIT macOS boot reaches controllable Link, one
   authentic transition, and save/quit/reload without captured state.
6. Representative content failures are bounded rather than architectural.
7. Physical iPad preserves the same save and sustains original game speed
   across representative workloads with measured thermal headroom.

Only gate 5 authorizes iPad proof work. Only gate 7 supports a full-product
commitment. Complete-game claims still require end-to-end acceptance.

## Prototype pause or stop triggers

- Hot execution remains interpreter-dominated after reasonable translation
  fixes.
- REL mapping requires pervasive runtime changes rather than a bounded
  registration seam.
- Authentic initialization needs unmodeled behavior with no tractable
  replacement.
- Composite module code size, link time, memory, or signing is impractical.
- Representative content reveals systemic corruption.
- Physical iPad profiles show no credible path to sustained original speed.

A failed, measured gate is a valid research outcome.

## Source-native Track B promotion triggers

1. **zeldaret/tww matched functions ≥ 90%** (was 79.99% on 2026-08-08).
   Check: https://decomp.dev/zeldaret/tww (JSON: `…/tww.json`,
   `measures.matched_functions_percent`). At current velocity (+2.3–3.8
   pts/month) expect this around **late 2026 / early 2027**.
   → At 90%, the sourceless tail (~2,000 functions) becomes small enough
   that in-house functionally-equivalent reconstruction of the remainder is
   a months-scale, not years-scale, add-on.
2. **zeldaret/tww effectively complete (~100% functions matched)** —
   naive ETA **Q2–Q3 2027**. → Full Dusk-recipe port becomes pure
   engineering (~12–20 engineer-months to 1.0; prototype in ~6 weeks).
3. **Audio stack closes upstream:** JSystem/JAudio + JAZelAudio stub count
   reaches ~0 (baseline 166 stubs + 14 NonMatching units at 2289b54; census
   method in docs/research/TWW_DECOMP.md). Audio is the weakest subsystem
   and the one library Dusklight cannot lend (TP=JAudio2, TWW=JAudio1).
4. **Menu/map cluster closes upstream:** d_menu_fmap/fmap2/item/dmap/
   collect + d_map + d_message_paper stubs ~0 (baseline ~460).
5. **TwilitRealm announces a Wind Waker port** (rumored next target,
   UNVERIFIED as of 2026-08-09; watch https://github.com/TwilitRealm and
   their Discord). → Decision flips from "build" to "contribute/adopt";
   duplication would be irrational against the team that shipped Dusklight.
6. **Aurora lands EFB color peek** (`GXPeekARGB`) upstream
   (https://github.com/encounter/aurora) — removes the last identified
   WW-specific graphics gap (Picto Box).
7. **Sponsor constraint changes** — if "weeks, all in-house" relaxes to a
   phased year-horizon source-port effort, explicitly select Track B in
   `IMPLEMENTATION_PLAN.md`.

## Cadence

Monthly check of Track B triggers 1–4 (single decomp.dev JSON fetch + two greps on
a fresh tww clone; baseline numbers and commands in
docs/research/TWW_DECOMP.md and docs/research/EXPERIMENTS.md E2/E4).
Triggers 5–6 are event-driven (watch releases/announcements).

## Where research stopped

The 2026-08-09 audit proved local tool builds and executable topology but ran
no disc-dependent experiment. The 2026-08-13 second opinion also had no disc
and ran no Wind Waker module. Current execution begins at
`IMPLEMENTATION_PLAN.md` Track A0 and `HANDOFF.md`; do not infer runtime proof
from either document set.
