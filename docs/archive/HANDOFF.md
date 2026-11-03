# BlueWake — Handoff to the Implementation Agent

> **SUPERSEDED STATUS (2026-08-30):** This document preserves the original
> bounded-prototype handoff and early milestone sequence. Those P1-P4 boot
> milestones have since been achieved: the signed macOS app reaches
> controllable Outset gameplay, persistent save/reload, and an authentic
> Outset-to-Omasao transition. Do not restart the tasks below. Begin with
> `docs/archive/status/CURRENT.md`, `docs/archive/status/FINISH_LINE.md`, and
> `docs/GOAL_LOOP.md`; macOS performance, graphics correctness, and product
> acceptance now own the active path.

> **HISTORICAL STATUS (2026-08-13):** proceed to the bounded, private, Mac-first
> static-recompilation prototype. Do not begin UI or iPad product work until
> authentic no-runtime-PowerPC-JIT macOS gameplay passes Track A4 in
> `IMPLEMENTATION_PLAN.md`. The 2026-08-09 WAIT decision and decomp/Aurora
> plan remain historical and long-term context.

The adopted second opinion concludes that BlueWake is plausible enough for a
bounded experiment, not that Wind Waker already runs. Its author had no disc
and built or executed no Wind Waker module. Begin by converting REL
integration, fallback burden, module size, authentic initialization, and
performance from opinions into measurements.

## The one-paragraph state of the world

Track A mechanically translates the retail DOL and complete REL collection
ahead of time, packages them as one module, and uses the Dolphin-derived
ModernGekko/RecompCore compatibility runtime with runtime PowerPC JIT disabled
and interpreter fallback available. SunPad demonstrates that Apple execution
contract, but Wind Waker's dynamic REL topology and fallback burden are
unproved. Track B remains `zeldaret/tww` compiled source over
Aurora/Dusklight patterns; it is cleaner but blocked from guaranteed
completeness by missing source.

## Pinned repositories (examined 2026-08-09; all in ref/, gitignored)

| Repo | SHA | Role |
|---|---|---|
| zeldaret/tww | `2289b54d5fd4ad22f7f9ee52ea4e2e81d79b5008` | game source (fork/subtree this) |
| TwilitRealm/dusk | `7434a0f834866294e7691bd86ab9527011265585` | recipe donor (CC0 — copy freely) |
| encounter/aurora | `1d10fa1bc502910a6336fdac32f31cd0ac39710d` | SDK/GX layer (submodule) |
| encounter/borealis | (dusk submodule pin) | app shell (submodule) |
| zeldaret/tp | `c8fa8c9e2aab72cf4e5db0e5d1c84a9ea6ee6eb0` | diff reference for dusk's edits |
| encounter/decomp-toolkit | `e4219e7644fb7b96d920d5bc3d1d950f5569dcaf` | dtk (build tooling) |

## Current architecture experiment

User-owned `GZLE01` → private `main.dol` plus complete REL inventory →
DolRecomp batch generation → BlueWake composite dispatcher/metadata and
active-REL registration → ModernGekko/RecompCore on Apple Silicon macOS with
runtime PowerPC JIT disabled. iPad work follows only after authentic gameplay.

## Local setup already proven on this machine

- macOS 26.5, Xcode 26.6, CMake 3.27.1, Ninja 1.13.2, python3.13, Rust
  needed later for nod-on-iOS.
- `ref/DolRecomp` builds+tests green (informative only).
- `ref/aurora`: `git submodule update --init --recursive && cmake -B build
  -G Ninja && cmake --build build && ctest` → 238/238.
- `ref/tww`: `python3.13 configure.py --version GZLE01 && ninja` → proceeds
  to SPLIT and stops ONLY for the missing disc at `orig/GZLE01/` (E8).

## Game-version expectations

Primary: **GZLE01** (US rev 0), sha1 of main.dol
`8d28bab68bb5078c38e43f29206f0bd01f7e7a67`. Also viable: GZLJ01, GZLP01,
D44J01 (demo, behind retail). The user must supply a legally-dumped image
(ISO/GCM/RVZ/WIA/WBFS/CISO/NFS/GCZ — dtk and nod both read these).
Never commit or redistribute game data; `.gitignore` already covers it.

## What research already completed (do not redo)

- Architecture comparison incl. rejection evidence: docs/research/
  ARCHITECTURE_OPTIONS.md, DOLRECOMP_MODERNGEKKO.md.
- Aurora capability audit vs WW's actual GX call sites (EFB formats all
  covered; GXPeekARGB gap = Picto Box): docs/research/AURORA.md.
- Dusk portability-pass taxonomy + reusable-component inventory:
  docs/research/DUSK.md.
- tww per-subsystem stub census + topology: docs/research/TWW_DECOMP.md.
- Experiments E1–E9: docs/research/EXPERIMENTS.md.
- Licenses/provenance: docs/research/LEGAL_AND_PROVENANCE.md.

## Known blockers (with owners)

1. 4,250 sourceless functions (upstream zeldaret/tww; closing ~2–4 pts/mo;
   NPCs/menus/audio) — track decomp.dev, contribute back.
2. JAudio1 stack (166 of those stubs + Dusk's audio lib is JAudio2, not
   reusable) — Phase 8 owns this.
3. GXPeekARGB (aurora) — clone its depth_peek pattern (Phase 7).
4. No local disc image on this machine (E9) — ask the user before Phase 1
   bring-up.

## First implementation task (start here)

Execute `IMPLEMENTATION_PLAN.md` Track A0 then A1:

1. Obtain the user's legally dumped `GZLE01` image without committing it.
2. Build a private reproducible extractor and manifest for `main.dol`, loose
   RELs, and `RELS.arc` RELs.
3. Resolve the 415-versus-416 inventory discrepancy from that manifest.
4. Run the current DolRecomp over the DOL and all RELs in one batch.
5. Publish only a game-data-free audit of translation, fallback, relocation,
   SMC, generated size, and object/build-size results.

Do not create an application shell or UI until this gate passes.

## Next five milestones

1. Private DOL+REL manifest reconciles every executable module.
2. Complete mechanical translation audit has no unexplained failure.
3. Composite arm64 module passes dispatch and synthetic REL lifecycle tests.
4. Normal no-runtime-PowerPC-JIT boot reaches authentic title and gameplay.
5. Controllable Link, one transition, and save reload → authorize
   representative compatibility work and later iPad proof.

## Definition of success (bounded prototype)

Track A4 in `IMPLEMENTATION_PLAN.md`. If a pause/stop trigger fires, record
the measured evidence. Continue monitoring Track B against:
- https://decomp.dev/zeldaret/tww (function match % — re-evaluate at ~90%
  and ~100%)
- zeldaret/tww JAudio/JAZelAudio and d_menu_* stub counts (TWW_DECOMP.md
  has the baseline)
- TwilitRealm public activity (rumored WW interest — cooperation beats
  duplication; UNVERIFIED as of 2026-08-09)
- aurora GXPeekARGB/EFB-peek support landing upstream
