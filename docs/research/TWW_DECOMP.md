# zeldaret/tww Decomp Assessment (@ 2289b54, 2026-08-08)

> **Correction 2026-09-01:** this historical assessment treated every
> `/* Nonmatching */` marker as an empty body. Current executable census proves
> that is false: many marked functions contain complete non-byte-matching
> source. Use `scripts/route_b_source_manifest.py` and
> `docs/archive/status/EXECUTION_ROUTE_CENSUS_2026-09-01.md` for route decisions.

Condensed from full repo inspection + decomp.dev API (2026-08-09). License
CC0-1.0. No assets/assembly in repo; build requires user's disc at
`orig/<VER>/` and produces byte-perfect (sha1-verified) main.dol + 415 RELs.

## Versions & topology

- Versions: **GZLE01 (US, primary)**, GZLJ01, GZLP01, D44J01 (demo). All in CI.
- GZLE01 = main.dol ("framework", 667 split units) + **415 RELs** =
  414 single-actor RELs (`d_a_*`: 139 obj, 57 npc, 20 tag, 187 other) +
  `f_pc_profile_lst.rel`. 119 mmem + 61 amem (in RELS.arc) + 235 loose.
- 26 always-resident actors live in the DOL (incl. all of `d_a_player*`,
  `d_a_sea`, `d_a_bg`, `d_a_vrbox*`, `d_a_item`...).
- **REL dynamic-linking path is 100% matched** (DynamicLink.cpp,
  c_dylink.cpp, OSLink.c, REL executor) — fully readable, and exactly the
  piece a port replaces with static linking (dusk precedent).
- No runtime code generation/self-modification observed: byte-perfect
  rebuild from static per-module images; no asm directory at all in repo.

## Progress (decomp.dev @ 2289b54 + configure.py object states)

- Overall 73.42% matched code; **79.99% matched functions** (31,458/39,324);
  88.6% units complete. DOL: 92.0% functions matched. RELs: 73.5%.
- **The number that matters for a port: 4,250 functions (10.8%) have NO
  source at all** (bare `/* Nonmatching */` empty bodies, 155 files).
  A further ~3,450 functions (~8.8%) have full source that doesn't yet
  byte-match (usable by a port, correctness unverified).
- GZLE01 object states: DOL 593/678 matching; RELs 305 Matching +
  4 Equivalent + 105 NonMatching. **312/414 actor RELs are stub-free.**

## Per-subsystem status (port-relevant)

COMPLETE (matching, zero stubs):
- `f_pc` 31/31, `f_op` 21/22, `f_ap`, `SSystem` 36/36, `c/` 2/2
- REL infra (DynamicLink, c_dylink, OSLink, executor)
- `m_Do` machine layer effectively complete (15/17; main(), machine, audio
  glue, dvd thread, memcard, reset, pad all matching; only m_Do_graphic 3
  stubs / m_Do_ext 1 stub)
- **`d_a_player*` — ALL of Link: 34,554 lines, matching, zero stubs**
- `d_stage`, `d_save`, `d_com_inf_game`, `d_resorce`, `d_event*`,
  `d_kankyo` (core), `d_s_play`, `d_s_room`, `d_s_open`, `d_s_menu`,
  `d_s_logo`, `d_s_title`
- JSystem: JKernel 25/25, JUtility 21/21, J2DGraph 8/8, JSupport, JMath,
  JFramework, JStage, JStudio 25/26, J3D near-complete (28/31)

GAPS (empty-body stub counts):
- **NPC/enemy/object actor tail: 3,459 stubs in 102 RELs** (worst:
  d_a_npc_ko1 134, bj1 119, kk1 112, p2 92, kf1 90...)
- **Menu/map/HUD cluster: ~460 stubs** (d_menu_fmap 131, fmap2 113,
  d_message_paper 55, d_map 52, d_menu_item 50, d_menu_dmap 35, ...)
- **Audio: 166 stubs + 14 NonMatching units** — JSystem/JAudio (JAudio1:
  JAI/JAS incl. DSP microcode driver) 112 stubs; JAZelAudio glue 54 stubs.
  Weakest engine subsystem. (TWW = JAudio1; TP = JAudio2 — Dusk's audio
  library layer does not transfer, only its DuskDsp hardware model.)
- Misc: d_camera 15, d_meter 15, d_kankyo_rain 18, d_s_name 14, JParticle
  11, JMessage 6.
- SDK: dolphin/ 78/93 matching — the 15 NonMatching are ALL GX/GF (replaced
  by Aurora in a port anyway). MSL/Runtime effectively complete.

## Velocity & activity

2,734 commits total; 283 in 2026 (to Aug 8). Matched code 56.7% (Jan 2026)
→ 73.4% (Aug 2026); +3.8 pts/mo since May. Driving maintainer LagoLunatic
(1,417 commits) + Jasper St. Pierre (581). Current work: NPC actor tail,
message system, menus. Naive 100% ETA: Q2–Q3 2027.

## Portability baseline

Zero portability work upstream: no CMake, no platform ifdefs, strictly
32-bit PPC assumptions, MWCC intrinsics in global.h (the natural first
seam), `VERSION_SELECT` multi-version macro (keep). Structure mirrors tp
closely (same d/f_op/f_pc/m_Do/SSystem naming; SDK under src/ not libs/) —
dusk's recipe applies nearly verbatim.

## Build system (verified locally, E8)

dtk-template: configure.py (Python ≥3.10) + ninja + dtk v1.8.3 (native
macos-arm64 binary) + mwcc GC/1.3.2 under wibo (arm64-capable). Only
missing input on this machine: the user's disc image.
