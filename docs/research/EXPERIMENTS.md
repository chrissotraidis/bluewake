# BlueWake Experiments & Evidence

> Historical research from August 2026, BlueWake's first days. The Route B prototype that followed it (a native build from the decompilation) was later removed; current work is in [GOAL_LOOP.md](../GOAL_LOOP.md).

Machine: Apple Silicon Mac (arm64), macOS 26.5, Xcode 26.6 (clang 21.0.0),
CMake 3.27.1, Ninja 1.13.2, Python 3.8.10 (system; newer available via Xcode).

Conventions: every entry records date, repo revision, command, result,
interpretation, remaining uncertainty. Logs preserved under `local-research/`
(gitignored) when large.

---

## E1 — Repository acquisition and pinning (2026-08-09)

**Command:** `git clone` of 8 repos into `ref/` (tww and tp with
`--filter=blob:none`); SHAs recorded via `git rev-parse HEAD`.

**Result:** SUCCESS. All 8 repos public and cloned. Pinned SHAs in
[DEPENDENCIES.md](DEPENDENCIES.md). Notable: `TwilitRealm/dusk` is public
(not just binary releases), last commit 2026-08-08; contains
`ios.toolchain.cmake` and a `platforms/` tree at top level.

**Interpretation:** All candidate infrastructure is inspectable primary
source. No reliance on press claims needed.

---

## E2 — decomp.dev progress API for zeldaret/tww (2026-08-09)

**Command:** `curl https://decomp.dev/zeldaret/tww.json` at tww commit
`2289b54` (2026-08-08). Raw JSON: `local-research/tww-report.json` (copy in
scratchpad).

**Result:** SUCCESS. Versions tracked: D44J01 (demo), GZLE01 (US), GZLJ01,
GZLP01. Default GZLE01. Overall: **73.42% matched code**, **80.0% matched
functions** (31,458 / 39,324), **59.63% fully-linked code**. Per category
(matched code % / matched functions %):

| Category | Matched code | Matched funcs | Complete units |
|---|---|---|---|
| DOL (main.dol) | 78.65% | 92.03% | 587/670 |
| Modules (RELs) | 69.46% | 73.52% | 904/1013 |
| TWW game code | 70.20% | 75.55% | 444/585 |
| Core engine | 91.07% | 99.49% | 68/71 |
| SDK | 86.24% | 95.28% | 745/773 |
| Third party | 94.26% fuzzy | 96.80% | 234/254 |

**Interpretation:** The decomp is well past half but far from the ~100% that
zeldaret/tp reached before Dusk shipped. ~7,866 functions (~2.08 MB of code
bytes) remain unmatched, concentrated in REL game code (actors). The DOL is in
much better shape than the RELs. A pure "Approach A" (decomp → port) is
blocked on this remainder unless hybridized.

**Remaining uncertainty:** which *specific* subsystems the unmatched 26.6%
falls in (player actor? stage? audio?) — see per-subsystem analysis.

---

## E3 — Inspection of sp00nznet/ww (Wind Waker pure static recomp attempt) (2026-08-09)

**Repo:** https://github.com/sp00nznet/ww @ `146ecde` (last commit
2026-03-29; 55 commits; ~884 KB source, solo author). Cloned to
`ref/ww-sp00nz`.

**Method:** README + source inspection (no build; Windows/D3D11-only).

**Findings (claims from README, structure confirmed in source):**
- Recompiler (`tools/recompiler/`: dol_parser, rel_parser, ppc_disasm,
  cfg_builder, ppc_to_c, symbol_map) processes the **full GZLE01 main.dol: 8,148
  functions, 348,958 instructions, 99.99% instruction coverage (1 unknown
  encoding)**, emitting 41 C++ translation units + function-pointer
  registration table for indirect dispatch.
- Runtime reaches: 100/100 static constructors, `fapGm_Create` init, main
  loop at 60fps, JKR archive mounting, Yaz0 + RARC + J3D/BDL parsing, TEV→HLSL
  shader gen, D3D11 draw of the Great Sea stage (terrain + water) with
  orbiting camera. DVD/VI/CARD HLE implemented.
- **Critical caveats (this is the evidence that matters):**
  - 16 recompiled functions require **manual source patches** (display init,
    heap, VI wait, GX sync, exception handler, DVD read, async DVD, `bctr`
    tail call...).
  - The frame gate (VRetrace interrupt wait) is **bypassed**, not implemented
    — no interrupt model.
  - Scene creation is **HLE-overridden**, and the process/actor system is fed
    with **"Dolphin reference state": 1,170 global values + 831 per-process
    template values captured from Dolphin during real gameplay** — i.e.
    authentic initialization does NOT complete on its own.
  - No controllable Link, no audio DSP, no claim of menus/title screen.
  - Repo commits `dolphin_*.bin` memory captures (game-derived data —
    provenance problem; noted in LEGAL_AND_PROVENANCE.md).
  - x86-64 / Windows 11 / D3D11 / XInput only. Stalled ~4.5 months.

**Interpretation:** Direct experimental evidence that (a) full-DOL static
recompilation of Wind Waker is tractable (instruction coverage is not the
hard part), and (b) the hard part is the **runtime**: interrupts, threading,
DSP, and faithful OS semantics. A pure static-recomp approach stalls exactly
at the goal spec's Gate 4 ("real game initialization") without a much more
complete GameCube runtime. This is the strongest available falsification
evidence against "Approach B alone" and shifts weight to decomp-based or
decomp-hybrid architectures with a proven runtime (Dusk-class).

**Remaining uncertainty:** whether ModernGekko's runtime is materially more
complete than this project's (under inspection).

---

## E4 — TWW decomp velocity (decomp.dev history API) (2026-08-09)

**Command:** `curl 'https://decomp.dev/zeldaret/tww.json?mode=history'`
(1,804 snapshots, 2023-09-13 → 2026-08-08). Raw JSON preserved:
`local-research/tww-history.json`.

**Result (matched code % over time, GZLE01-weighted all-version metric):**

| Date | Matched code | Matched funcs | Fully linked |
|---|---|---|---|
| 2024-08 | 28.6% | 41.1% | 16.6% |
| 2025-01 | 30.3% | 42.8% | 17.4% |
| 2025-08 | 46.6% | 58.5% | 27.1% |
| 2026-01 | 56.7% | 67.3% | 36.6% |
| 2026-05 | 61.2% | 71.6% | 41.9% |
| 2026-08-08 | **73.4%** | **80.0%** | **59.6%** |

(matched_code metric only exists from 2024-08; earlier snapshots lack it.)

**Velocity:** +2.3 pts/month average across 2026; **+3.8 pts/month since
May 2026** (post-Dusk-release acceleration; June alone jumped ~7 pts).
Naive extrapolation of the remaining 26.6 pts: **7–12 months to effective
completion (Q2–Q3 2027)**, with the usual caveat that the tail can be
disproportionately hard (the last % are the ugliest functions), or
disproportionately easy (data sections, mechanical cleanup) — TP's own
history showed a sprint at the end.

**Interpretation:** Approach A ("Dusk model") is not feasible *today* but has
a credible upstream ETA measured in months, not years. This materially
affects the PROCEED/WAIT/PROTOTYPE decision: platform/runtime work
(Approach D substrate) can proceed now in parallel with upstream matching.

---

## E5 — DolRecomp builds and passes tests on Apple Silicon (2026-08-09)

**Repo:** ref/DolRecomp @ `48c4ef11` (2026-08-03).
**Commands:**
```
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release   # exit 0
cmake --build build -j8                              # exit 0, 70/70 targets
ctest --output-on-failure                            # 16/16 PASSED (3.2 s)
file build/dolrecomp                                 # Mach-O 64-bit executable arm64
```
Logs: `local-research/dolrecomp-{configure,build}.log`.

**Result:** SUCCESS. Native arm64 build, zero patches needed. Test suite
passes **on ARM64**, including `fpscr`, `float_semantics`, `jumptables`,
`dispatch`, `codegen_compile`, `c_execute` — i.e. the generated-code path is
exercised end-to-end on this exact host architecture.

**CLI capabilities confirmed:** GameCube mode (`--gamecube`), CPU profile
`--cpu gekko`, **REL module input** (`<input.rel | rel_folder>`), linker-MAP
symbol ingestion (`--map`), split-C multi-job output, and an alternative
`--backend llvm`. Also a disc extractor (`extract game.iso`).

**Interpretation:** Gate 0 (toolchain) passes for the static-recomp leg on
Apple Silicon. The recompiler itself is not the risk; runtime completeness
(ModernGekko) is where scrutiny belongs.

**Remaining uncertainty:** semantics of generated code (memory model,
dispatch) for iOS static linking; whether `--cpu gekko` covers paired
singles fully (source inspection in progress).
→ RESOLVED same day by source inspection: see DOLRECOMP_MODERNGEKKO.md
(paired singles fully covered; runtime model disqualifying).

---

## E6 — Aurora builds and passes tests on Apple Silicon (2026-08-09)

**Repo:** ref/aurora @ `1d10fa1` (2026-08-06), submodules initialized.
**Commands:**
```
git submodule update --init --recursive --depth 1   # exit 0
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release  # exit 0 (Dawn resolved
                                                    #  as prebuilt darwin-arm64 package)
cmake --build build -j8                             # exit 0, 180/180 targets
ctest                                               # 238/238 PASSED (5.5 s)
```
Logs: `local-research/aurora-{submodules,configure,build}.log`.

**Result:** SUCCESS. All aurora libs built natively:
`libaurora_{core,gx,gd,os,pad,si,ms,vi,mtx,card,dvd,main}.a` + examples.
Test suite (GX FIFO round-trip, display lists, texture cache, texture
replacement streaming, render worker, DVD, OSAlloc) green on arm64 macOS.

**Interpretation:** Gate 0 (toolchain) passes for the decomp-port leg. The
entire graphics/SDK substrate of the favored architecture compiles and
self-tests on the target host with zero patches.

---

## E7 — TWW executable topology from tww build config (2026-08-09)

**Method:** grep/count over `ref/tww/config/GZLE01/` (no game data needed).

**Result:** GZLE01 = `main.dol` + **416 REL modules** — 235 loose under
`files/rels/`, 181 packed in `RELS.arc` split into `mmem` (119) and `amem`
(61) pools (+ `f_pc_profile_lst.rel`). Per-REL split configs exist for all
416 (`config/GZLE01/rels/`). Versions tracked: GZLE01/GZLJ01/GZLP01/D44J01.

**Interpretation:** WW is one of the most REL-modular GameCube titles
(TP: 757 per dusk's files.cmake, statically linked there without issue —
so 416 statically-linked RELs is precedented). For any static-recomp
architecture this topology is disqualifying today (no runtime OSLink model);
for the decomp architecture it is routine (compile all RELs into the
binary, stub the dynamic linker, exactly as dusk's c_dylink.cpp does).

---

## E8 — tww build system configures natively on Apple Silicon (2026-08-09)

**Repo:** ref/tww @ `2289b54`.
**Commands:**
```
python3 configure.py --version GZLE01     # FAILS: system Python 3.8 too old
                                          #  (match statements need 3.10+)
python3.13 configure.py --version GZLE01  # exit 0
ninja                                     # downloads dtk-macos-arm64 v1.8.3,
                                          # then FAILED (expected):
                                          # "orig/GZLE01/files/RELS.arc not found"
```

**Result:** PARTIAL SUCCESS, exactly as predicted. The dtk-template build
system is Apple-Silicon-native (dtk ships a macos-arm64 release binary;
tools/project.py invokes mwcc under wibo, which supports arm64 macOS). The
ONLY missing ingredient is a user-supplied disc image at `orig/GZLE01/`.
Requires Python ≥3.10 (3.13 present on this machine).

**Interpretation:** Gate 0 fully passes for the decomp leg. The full
byte-verified rebuild (main.dol + 415 RELs, sha1-checked) is one
disc-copy away on this machine. **Blocked experiment (user input needed):**
place a legally-dumped GZLE01 image at `ref/tww/orig/GZLE01/` and run
`ninja` to verify the byte-perfect rebuild end-to-end. Nothing else in the
research required the disc.

---

## E9 — No local Wind Waker disc image present (2026-08-09)

**Command:** Spotlight (`mdfind`) search for GZLE01 / *.rvz / *.gcm / Wind
Waker disc formats.

**Result:** none found (only repo artifacts and DolRecomp's synthetic
`sample.gcm` test fixture).

**Interpretation:** All disc-dependent verification (E8 completion; any
runtime experiment) is deferred and documented. No Nintendo data was
downloaded; none will be.

---
