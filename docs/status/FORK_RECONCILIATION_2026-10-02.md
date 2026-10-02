# Fork reconciliation ledger, October 2, 2026

**In progress.** Chris and Elliott have agreed to consolidate development in
BlueWake. This migration directly imports Elliott's enhancements, preserves his
authorship and credits combined implementations with co-author trailers.
Source integration is reviewable in stacked draft PRs; required
performance, player-build and gameplay acceptance remains open. No release,
fork redirect, issue closure or migration acceptance is authorized. Personal
modules, generated source, discs, profiles, builds, saves and captures stay local.
Public app-only candidates still require a Clear private audit and the content gate.

This is the authoritative current ledger. The
[evidence appendix](FORK_RECONCILIATION_EVIDENCE_2026-10-02.md) preserves the earlier
experiments, failed approaches and exact historical checkpoints. “Integrated”
below means source is present with bounded evidence, not that parity is accepted.

## Fixed baseline and selected dependencies

Reconcile the fixed donor main **and** windows-release snapshots below. Track
later donor changes separately. Baseline refs were verified October 2 JST.

| Repository / ref | Exact commit / disposition |
| --- | --- |
| BlueWake main | `72138a1eccd1800f4d85bf9718913b1d743cda32`; not the integrated candidate |
| BlueWake PR #10, `codex/fork-parity` | `22dca5d2a4e8d9c3589b22ec4163f5111a5cceaa`, open draft |
| BlueWake PR #12, `claude/git-commit-author-config-d4sdwt` | Initial `ad7f5512bdd04a0d923f77c98788a37421b3c917`; checkpoint `736f178` adds the previously unfinished runtime pin/patch/CI registration; stacked on #10 |
| Donor `main` | `e021b71bcb560109fb5c1b64cfb74f3801f6d189` |
| Donor `windows-release` | `f960ca344814fa10cb9ad692402822851b3ffeaa` |
| Donor RecompCore `windows-release` | `634895470af6e61e601f06a351f3a72888215159` |
| Donor DolRecomp `bluewake` | `b8b534591cba8ca7cd43943a655ee6e2591cf5de` |
| Baseline maintained RecompCore (PR #12) | `99e4748002d42c1a86fdcb33a47cd0e97292acff`; retains BlueWake safety fixes through patch 0130 |
| Build-selected DolRecomp submodule / profile / lock | `b8b534591cba8ca7cd43943a655ee6e2591cf5de`, matching the donor |


The current candidate's lock and builder profile both select maintained runtime
`c2905b7a6b2752611a4e1ccb2ac9e3cda1857462` (runtime PR #2, based on #1 / `70bc9957`).
Patches through 0135 preserve BlueWake's safety changes. The translator remains
`b8b534591cba8ca7cd43943a655ee6e2591cf5de`. Ordinary modules use ABI 3;
fixed-CPU modules declare ABI 4; fixed-CPU plus module-owned RAM declares ABI 5.
Old hosts reject unsupported ABIs before player storage. Apple shell support for
the new storage ABIs is not claimed.

## Later donor changes, tracked separately

A live ref refresh during #29 found donor `main` still at `e021b71b`, while
`windows-release` is now `7ca0cb94b73af9802d1a40c726409e5be3af08f2`, ten commits
beyond the fixed `f960ca3` Windows baseline. Five are README history already
represented by the fixed main snapshot. The later implementation commits are:

- `cde4df7` (plus status `e1732fd`): donor reports a dual-texture post-transform
  renderer correction for lava colour, with a runtime pin/patch change.
- `e108437`, merge `4da4d09`, and `7ca0cb9`: motor-command decoding and enhanced
  controller haptics, including Mac/Windows controls and Windows build wiring.

These changes are not included in the fixed-baseline parity evidence above.
The current candidate has no `haptics.c` implementation. Review and reconcile
this later-change list before recommending cutover; donor commit descriptions
are not BlueWake validation. Do not silently repin the active qualification
builds or treat the fixed baseline as the donor's current head.

Preflight against maintained runtime `c2905b7a` confirms the donor's later
dual-texture patch applies cleanly (`git apply --check` only). It has not been
applied, built or gameplay-qualified in BlueWake.

## Source checkpoints and integration order

All listed BlueWake PRs remain drafts. Source checkpoints are evidence identities;
a later documentation-only commit may change the PR head. Preserve the original
contributor authorship, eight pre-existing edits checkpointed in #12, and other PRs.
PR #11's SDK preflight is reconciled by #14; do not import it again.

| PR | Source checkpoint | Scope / next gate |
| --- | --- | --- |
| [#10](https://github.com/chrissotraidis/bluewake/pull/10) | `22dca5d` | Earlier fork integration; base for #12 |
| [#12](https://github.com/chrissotraidis/bluewake/pull/12) | `736f178` | Preserved stability work and runtime safety pin |
| [#14](https://github.com/chrissotraidis/bluewake/pull/14) | `95adeed` | Fresh mouse queue, SDK preflight and source-only PadMint repair; primary player-build baseline |
| [#15](https://github.com/chrissotraidis/bluewake/pull/15) | `54bbb26` | Windows disc import/recovery foundation; real UI/disc acceptance open |
| [#16](https://github.com/chrissotraidis/bluewake/pull/16) | `5c8d348` | Maintained runtime/display-rate policy; physical pacing/play open |
| [#17](https://github.com/chrissotraidis/bluewake/pull/17) | `5208567` (docs `05df605`) | Conservative prepared blocks; O0/O2 function and intro/saved-game state/image comparisons pass; quiet matched measurements open |
| [#18](https://github.com/chrissotraidis/bluewake/pull/18) | `6532973` (docs `f1c1c7a`) | Fixed CPU / ABI 4; bounded arm64 O0 checks pass |
| [#19](https://github.com/chrissotraidis/bluewake/pull/19) | `3222b91`, fixture `0e78428` | Module RAM / ABI 5 and extended-alias fix; bounded arm64 O0 checks pass |
| [#20](https://github.com/chrissotraidis/bluewake/pull/20) | `9d3729a` | Corrected inline FP; native instruction tests and arm64 O0 module/boot pass |
| [#21](https://github.com/chrissotraidis/bluewake/pull/21) | `7b7e530`, Windows includes `9303569` | Active Release assertions and restored GroundCross observation; bounded host check and native Windows pass |
| [#22](https://github.com/chrissotraidis/bluewake/pull/22) | `69426b2` | Per-turn consecutive dispatch counter; unit/sanitizer, strict module boot and native Windows checks pass |
| [#23](https://github.com/chrissotraidis/bluewake/pull/23) | `b85f931` | Gather/inline-memory contract foundation; two reproduced guards, local contracts and all 41 native Windows tests pass; module/renderer/performance qualification open |
| [#24](https://github.com/chrissotraidis/bluewake/pull/24) | `608edaa` | Independent gather-module preparation; nine cache tests, strict function/boot comparisons and 41 native Windows tests pass; host batching not selected |
| [#25](https://github.com/chrissotraidis/bluewake/pull/25) | `80eb353` | Explicit host writer/batching; 42 native Windows tests and bounded Mac route/card/intro-image comparisons pass; 23,000-retrace route/card match; player-control trigger not reached |
| [#26](https://github.com/chrissotraidis/bluewake/pull/26) | `9de9cf2` | Optional complete-coverage checkpoint comparisons; six CPU/RAM/REL hash checkpoints match over 6,000 headless retraces; Aurora checkpoints and two intro frames match; native Windows host build passes |
| [#27](https://github.com/chrissotraidis/bluewake/pull/27) | `8233de9`, import `d176f3e` | Direct/indirect calls and certified register inlining; 30,000 active function cases, three-way headless/Aurora routes and intro frames, and 45 native Windows tests pass; optimized measurements and gameplay remain open |
| [#28](https://github.com/chrissotraidis/bluewake/pull/28) | `ce31672`, import `60a713d` | Certified J3D module/host selection; 100,000 function cases, three-way intro/saved-game comparisons and 46 native Windows tests pass; optimized/native Windows module measurements open |
| [#29](https://github.com/chrissotraidis/bluewake/pull/29) | `fc02056`, import `ed994c4` | Nine certified native vector functions; strict function/guard/cache checks, three-way intro/saved-game comparisons and 47 native Windows tests pass; combined optimized qualification open |
| [#30](https://github.com/chrissotraidis/bluewake/pull/30) | `a294985`, import `a080b16` | Four certified native matrix functions, cached dispatch and optional workers; routed fixtures, four-way intro/saved-game state/image comparisons and all 48 native Windows tests pass; combined performance/gameplay open |

Runtime [PR #1](https://github.com/chrissotraidis/RecompCore/pull/1) at `70bc9957`
rebases donor global MEM1/display overload work with bounds checks. Runtime
[PR #2](https://github.com/chrissotraidis/RecompCore/pull/2) at `c2905b7a` fixes
alias tracking above retail 24 MiB. Both are drafts; all public checkpoints are
source only. No source merge or test result implies release readiness.

## Required parity inventory

| Feature / platform | Donor source / dependency | BlueWake state / difference | Required acceptance / status |
| --- | --- | --- | --- |
| Interpolation, batching, upload staging, worker/cache improvements; Apple/Windows | main `b6f87e0`, `7f4f1a0`, `e5c6ae4`, `b39bd0d`; runtime through `9618e9d` | Integrated in #10 plus newer synchronization fixes in #12 | Integrated; final same-scene images, 60/120 pacing and sustained play open |
| Swapchain/fullscreen and orderly restart; Windows | `db2944e`, `1ce29ac`; runtime `9618e9d` | Swapchain fix already in maintained runtime; #12 orderly quit/relaunch replaces donor process-exit approach | Integrated; actual Windows F11/Restart/settings/startup recovery open |
| Disc picker, remembered disc, compressed import and recovery; Windows | `0b463cc`, `61e891e`, `640094e`; shared `disc_import.c`, nodtool | Launcher source integrated in #15; unique import folders retain original/converted/previous files; candidate native 29/29 regressions pass | Integrated; real supported/rejected/missing disc and UI/Unicode interruption/recovery checks open |
| Disc picker / launcher; Apple Silicon Mac | main `561ddbf`, `7a2ac23`, `5eaa981` | Developer host route exists; donor bundled-game packaging is intentionally excluded | Open; personal clean source build and app-only launcher with player's generated module, save/reload and installation |
| Camera, right-stick aiming/zoom/collision; desktops | main `887c26d`, windows `9511241` | Latest SDL queue timing integrated with window-scoped filtering; prior BlueWake camera/menu code retained | Integrated; actual SDL queue regression passes; real mouse/controller/camera timing acceptance open |
| Jump, sprint, quick doors, transitions; Apple/Windows | main `22fa284`, `d55ce11`, `2c9f16c`, `df62ae0` | Integrated; Apple touch controls use existing editor, new options remain opt-in | Integrated; final new-module progression and controls on/off open |
| Fifteen Better Wind Waker options, 16:10; all claimed targets | main `b6f87e0`; DolRecomp `b8b5345` | Integrated; same verified base-source digest; new options require module rebuild, legacy fallback retained | Integrated; clean player-generated module, all exports/options and relevant gameplay open |
| Desktop save states and climbing | main `b39bd0d`, windows `3ba8599`, `1510ed1` | Integrated; #12 adds checked/atomic serialization; states experimental, climbing off | Integrated; current module state compatibility, real save/load and climbing acceptance open; no new Apple touch state UI claimed |
| Controller face layouts/navigation | donor reports #2/#8/#14; existing SDL controls | #12 adds A/B and X/Y swaps, navigation/game-input isolation and virtual-controller checks | Integrated; real Switch Pro/Xbox/8BitDo hot-plug/menu/closing-input checks open; arbitrary remap is separate scope |
| Prepared-block/global-register module optimizations; Windows | `f319afa`, `8435ec7`, `16fabda`; scripts/windows transformers and cmake/composite helpers | Generic prepaid-block transform and portable strict A/B fixture imported; fixed-CPU preparation from `4b6b268` added separately with a declared module ABI and explicit builder opt-in. Both default off; module-owned MEM1 is integrated separately in #19, native batches remain open | Open; isolate generic transforms from native/decomp work, private generated-code correctness and matched before/after performance |
| Inline floating-point interpreter operations; Windows | `4b6b268`; `inline_fp.h`, chunk header preparation | Separate `--inline-fp` opt-in, off by default; no game-native replacements or ABI change | Open; Corrected helper: arm64/Rosetta and native Windows checks pass; 30,000 module cases and 6,000-retrace arm64 boot match. Optimized module/performance/gameplay open |
| Direct cross-chunk/indirect calls and inline GPR save/restore; Windows | `4b6b268`, `742f1a0`, `76c688d`, `7576dc9` | Imported directly with Elliott's authorship in `d176f3e`; candidate adds versioned host readiness, shared feature predicates and guarded/certified register inlining. Builder options remain independent and off by default | Integrated with bounded checks; all 823 private O0 module units link, 30,000 active function cases and three-way headless/Aurora route/checkpoint/intro-image comparisons pass. All 45 native Windows tests pass. Optimized combined modules, gameplay and matched timing remain required |
| Gather-pipe batching and inline memory wrappers; Windows | `3e14287`, `6def7cd` | Generic helper foundation imported with original authorship; separate default-off `--gather-pipe` prepares modules. Host direct writer/batching now has separate default-off selection; bounded Mac route/card/intro-image comparisons pass. Undersized-RAM and pipe-alias precedence regressions fixed | Open; source-only memory/flush fixtures and nine cache tests pass locally; 813 chunks prepare reproducibly and unprepared inputs are rejected. Arm64 O0 function/boot comparisons pass with the host writer disabled; optimized modules, sustained renderer/gameplay qualification and matched timing remain required |
| Native math/J3D/skin/vector replacements; Windows | `689f042`, `845a589`, `833e6fb`, `1e97dca` | J3D rotation/translation functions integrated in #28 with certified builder preparation and versioned host opt-in; recovered-source attribution verified. Nine vector functions are selected independently in #29; four SDK matrix functions and optional workers are integrated in #30; game-math/skin replacements remain open | Open; J3D, vector and matrix function and arm64 O0 intro/saved-game state/image comparisons pass. Remaining native imports, combined optimized/native Windows module checks and matched measurements required |
| Global MEM1 / inline guest-memory accesses; Windows | `f70305c`, `6def7cd`; runtime `460b5b84`, `63489547` | Optional runtime global MEM1 rebased in #16; explicit ABI-5 module storage adoption now in a separate default-off candidate. Extended alias guard fixed in maintained runtime `c2905b7a`; inline module wrappers imported and separately qualified in #23/#24 | Open; remaining MMIO/reservations/journaling/dispatch, module compatibility and matched measurements |
| Overload suspension and display-rate interpolation, up to 240 | `f70305c`; runtime `63489547`; UI `4fedbfc` | Runtime and Mac/Windows display settings source integrated in #16; three/seven-step regressions and rate/preference policy pass; 30 Hz logic and interpolation off remain defaults | Open; Off/120/Off/relaunch UI persistence passes on Mac; real display changes, pacing, slow-game workloads, images and matched performance remain open |
| Save durability, failed startup/audio recovery; all claimed targets | Current reports; donor initial mechanisms | #12 adds stronger atomic card/settings/state writes, lock/recovery, crash logs, launch marker, sink audio recovery; retained over donor files | Integrated; real error/restart/output-device/upgrade acceptance open, no power-loss guarantee |
| Apple identity, touch, saves, settings, tvOS | BlueWake contributions incl. Ian MacFarlane #3, #6, #8 | Preserve existing Apple shell and BlueWake styling; tvOS remains preview with separate hardware/storage gates | Integrated; latest iPhone/iPad candidate acceptance open; TV parity only where claimed, no physical TV evidence |
| Source build from owned USA rev-0 disc | BlueWake shell builder; donor Windows builder | Clean owned-disc iOS build/package and bounded resume pass at `95adeed`; final integrated-source build and actual run/save/upgrade still required. Windows training/build path incomplete | Open; fresh output, pinned public sources, validation/translation/mods/train/compile/package, interruption/resume, local personal run/save/reload |
| PadMint | `padmint.json`, docs/PADMINT_HANDOFF.md; actual selected PadMint adapter/revision must be refreshed | Manifest: experimental iOS on Apple Silicon, macOS planned; no Windows/tvOS adapter claim | Open; Fresh PadMint iOS translation/training, module compilation, assembly and provenance checks pass at the baseline; player signing/install, final-source selection, device save/reload and matched performance open |
| Public app-only candidate and compatibility | BlueWake `--app-only` / `--app` model | Donor game-containing distribution excluded; keep own-disc modules local | Baseline and candidate runtime app-only audits pass. Compatible old-module update vs rebuild and physical acceptance open; no publication |
| Consecutive non-advancing dispatch bound; desktop modules | Existing BlueWake loop, found during #21 assertion audit | #22 makes the counter per-call and resets after progress; stuck guest still yields on ninth non-advancing successor | Open; Release and ASan/UBSan regression, strict module boot and all 38 native Windows checks pass; optimized/performance/gameplay acceptance open |

Original 30 Hz game logic, Smooth Motion Off and experimental 60 Hz simulation
Off remain defaults. Explicit preferences are preserved. New module transforms
remain independent opt-ins. Displayed FPS is not simulation speed. Experimental
60 Hz simulation, Linux, broader discs and wider decompilation are separate
scope unless required by the agreed current parity target.

## Current acceptance evidence and limits

**Trace coverage correction:** the legacy 600 `[guest-state]` records all occur
before retrace 8, ending at cycle 60,001,200. They contain selected startup fields,
not the full CPU or RAM, and do not cover the full 6,000-retrace route. Earlier
wording saying "600 exact guest states" overstated this evidence; this correction
also applies to the historical appendix. Route/card, independently checked timing,
function-fixture and image results retain their stated scopes. The 30,000-case
function fixtures do compare full CPU/RAM within those cases. Broader optional
checkpoint instrumentation and its local fixtures pass; six headless checkpoints
now match across the 6,000-retrace Off/direct pair. This is sampled coverage, not
a claim of identical state at every intervening instruction. The existing route comparator remains unchanged.

- **Host/runtime regressions:** all 248 Mac tests pass at `7b7e530` (67.65 s).
  Eight older assertion-based fixtures were previously disabled by Release's
  `NDEBUG`; earlier 247-test counts include those ineffective passes. The repaired
  fixtures are active, and five portable ones now run on Windows. At `9303569`,
  Windows builds the app and passes all 38 tests (35.49 s;
  [run 36970389671](https://github.com/chrissotraidis/bluewake/actions/runs/36970389671)).
- **Prepared blocks:** the donor-style broad transform fails the controlled
  route despite passing the function fixture; retaining PC stores alone also
  fails (22/600 selected startup records differ). The conservative successor preserves
  PC/suffix observations and leaves refund/unknown prepaid-state blocks alone.
  It passes 30,000 function cases and 6,000 retraces: 1,050 canonical/card records,
  600 matching selected startup records and delivery/clock timing match. This is bounded O0
  correctness, not full donor optimization/performance parity. Both matched O2
  modules now compile/link successfully (baseline source `95adeed`, runtime
  `99e47480`, transform `5208567`; ordinary ABI 3, no fixed CPU/RAM/native helpers).
  The O2 pair passes 30,000 full CPU/RAM cases, including 264 partial returns and
  12,539 deadline cases. With current host `ce31672` / host runtime `c2905b7a`,
  it also passes the 6,000-retrace intro (six CPU/MEM1/MEM2/REL checkpoints) and
  2,000-retrace Aurora existing-save movement route (four checkpoints, all 37
  sampled player states and both nonblank Outset frames at 1,200/1,600).
  Each pair matches all 1,050 route/card records and has zero delivery/clock drift.
  Original save backup hashes remain unchanged. A six-run alternating-order
  measurement harness is prepared, but its initial quiet-machine preflight
  refuses to run while unrelated local compilation is active. No performance
  result is accepted. Preserve failed candidates and the unchanged comparators.
- **Fixed CPU and module RAM:** each independent O0 candidate passes the same
  30,000 function cases and strict 6,000-retrace pair. The RAM fixture also checks
  assigned pointer identity; plain/global alias, reservation, journal, MMIO,
  endian and bounds checks pass sanitizers. Full storage ABI adoption/cleanup and
  actual old-host rejection pass. Whole-module optimized x86 and gameplay remain open.
- **Inline FP:** the strengthened fixture exposed 9,526 native Windows division
  differences. A zero/subnormal divisor guard fixes the host-DAZ case; deterministic
  tests fail before and pass after. The corrected helper passes 38 million operation
  comparisons on arm64, x86-64/Rosetta and native Windows. Corrected arm64 module
  `9d3729a` passes 30,000 function cases and the strict 6,000-retrace pair with
  1,050 matching records/cards, 600 matching selected startup records and zero schedule drift.
  The host is `3222b915`, both modules use runtime `c2905b7a`. Optimized/performance
  and gameplay acceptance remains open.
- **Restored observation:** #21's old/new-host pair uses the same ordinary module.
  Exactly one canonical record changes: GroundCross visits rise from 0 to 1,974,
  with valid and sentinel values observed. The strict comparator correctly rejects
  equality; preserve that result. All other 1,049 records, cards, 600 selected startup records,
  1,024 delivery cycles and route clock match. The explicit intended diagnostic
  difference is recorded without weakening the comparator. This is bounded
  headless evidence, not gameplay acceptance.
- **Counter repair:** #22's new test fails on the old loop and passes through both
  entry points after the repair. It checks alternating progress, interrupted runs,
  independent turns/CPUs and bounded stuck execution. The candidate module links;
  all 819 other object hashes match the baseline, with only module-export/dispatch
  objects changed. The strict 6,000-retrace pair passes with 1,050 matching
  records/cards, 600 matching selected startup records and zero scheduling drift. Native Windows
  passes all 38 tests (31.87 s;
  [run 36970729454](https://github.com/chrissotraidis/bluewake/actions/runs/36970729454)).
  The incremental build is not clean player-build evidence.
- **Inline-memory module:** #24 at `608edaa` builds all 823 O0 units and passes
  30,000 strict function cases against #22, including 264 partial returns and
  12,539 deadline cases. The same `7b7e530` host / `c2905b7a` runtime passes the
  6,000-retrace pair: 1,050 canonical/card records, 600 matching selected startup records and zero
  delivery/clock drift. The optional host FIFO writer is **disabled** in both
  runs. This does not qualify GPU batching, optimized performance, gameplay or
  a clean player build. Native Windows at `d0859b3` passes all 41 tests
  (40.03 s; [run 36974344310](https://github.com/chrissotraidis/bluewake/actions/runs/36974344310)).
- **Host writer/batching:** #25 at `80eb353` passes all 42 native Windows tests
  (43.99 s; [run 36975727076](https://github.com/chrissotraidis/bluewake/actions/runs/36975727076)).
  Same-host/module Mac O0 pairs compare writer Off/direct headless and Off/batch
  with Aurora over 6,000 retraces. Each passes 1,050 canonical/card records,
  600 matching selected startup records and zero delivery/clock drift. The
  renderer pair also has two identical nonblank 1920x1440 intro frames at
  retraces 2,000/4,000. An older module without writer exports stays Off when
  batching is requested. Both 23,000-retrace Aurora runs complete normally with
  matching route/card records and zero delivery/clock drift, but neither reaches
  the player-control trigger or captures player frames. The private driver's
  incorrect expectation of 2,300 legacy trace records fails; its original failure
  is retained. Empty capture sets do not establish image agreement. Player-control,
  sustained gameplay and performance acceptance remain open.
- **Checkpoint coverage:** optional `BLUEWAKE_GUEST_CHECKPOINT_INTERVAL=N`
  hashes normalized CPU fields, full MEM1/MEM2 and registered REL storage at each
  Nth retrace. Native addresses and padding are excluded; pointer presence is
  retained. The C fixture passes Release and arm64 ASan/UBSan. The separate
  `scripts/guest_checkpoints.py --interval 1000 --through 6000 A.log B.log`
  comparator requires every expected checkpoint, increasing cycles, one normal
  stop and the requested terminal retrace. Its fixtures reject missing, duplicate,
  malformed, truncated and differing evidence. Use it alongside route/card and
  image checks. It is disabled by default, adds measurement overhead when enabled,
  and does not cover GPU/peripheral state or execution between checkpoints.
  At host `9de9cf2` / module `608edaa` / runtime `c2905b7a`, all six checkpoints
  at retraces 1,000 through 6,000 match in the headless Off/direct pair, alongside
  route/card and zero delivery/clock drift. Comparing the new instrumented Off
  run with the previous host's Off run also preserves route/card/timing results.
  The corresponding Aurora Off/batch pair also matches all six checkpoints,
  route/card/timing and both nonblank intro frames. Native Windows at `91e0417`
  builds the host successfully (run 36980025091).
- **Direct calls/register inlining:** source `8233de9` preserves Elliott's exact
  import (`d176f3e`) and credits the adaptation with a co-author trailer. The O0
  candidate passes 30,000 full CPU/RAM cases against the ordinary #22 module,
  including 264 partial returns, 12,539 deadline cases and 6,400 active readiness
  queries. The fixture gives both modules the same observation/budget boundaries.
  Same-host three-way comparisons (ordinary module, candidate Off, candidate On)
  complete 6,000 retraces headless and with Aurora: all 1,050 route/card records,
  all six normalized CPU/MEM1/MEM2/REL checkpoints and delivery/clock timing match.
  Both nonblank intro frames at retraces 2,000/4,000 also match exactly. The On
  runs record over 138 million approved readiness queries; Off runs record zero.
  Native Windows at `a2b18bf` passes all 45 tests (41.14 s;
  [run 36983460501](https://github.com/chrissotraidis/bluewake/actions/runs/36983460501)).
  Eleven cache regressions cover reuse and disabling register inlining while
  direct calls remain enabled. No timing from these traced/loaded runs establishes
  performance parity. A separate existing-card three-way Aurora route now completes
  2,000 retraces using scheduled A presses and a 20-retrace stick input: all 37
  sampled player states, four checkpoints (every 500 retraces), 1,050 route/card
  records, zero delivery/clock drift and two nonblank Outset frames at retraces
  1,200/1,600 match. All three runs show position changing with decoded stick input
  while event/demo mode is clear. The original save backup hash is unchanged.
  The new-game control marker remains absent because its overlap latch is tied
  to the new-file route; this check uses observed input and movement directly.
  This is bounded gameplay evidence; sustained play, save/reload, upgrades and
  combined optimized performance remain open.
- **Recovered J3D functions:** #28 preserves Elliott's five-file import
  `60a713d`; adaptations `5a9a59a`/`ce31672` credit him as co-author and record
  the fixed recovered-source attribution in RIGHTS_AND_LICENSES.md. The
  independent `--native-j3d` builder option certifies before generic rewrites,
  fingerprints selection/helpers, and removes routing on disable. Host
  `BLUEWAKE_NATIVE_J3D=1` requires the versioned module handshake and read-only
  observation predicate; missing support stays Off. Defaults and module ABI
  are unchanged. Twelve cache tests, three certification test methods and
  Release/ASan/UBSan handshake/guard checks pass. A pre-fix null-RAM sanitizer
  failure is retained. CMake rejects unprepared inputs. The combined fixed-CPU/
  MEM1, prepared-block, inline-FP/gather/direct-call J3D chunk certifies and
  passes syntax checking; this is not whole-module combination acceptance.
  The fresh independent O0 module links all 823 units. Across 100,000 arm64
  cases, 67,695 native executions match every CPU byte (including cycle suffix)
  and the 40 KiB RAM test area; 32,305 unsupported cases decline unchanged.
  All 67,695 accepted calls also match through the active routed module.
  Same-host three-way ordinary/candidate-Off/candidate-On runs pass the
  6,000-retrace headless intro (six checkpoints, 1,050 route/card records,
  zero timing drift; 6,873 native calls/26 guard fallbacks) and the 2,000-retrace
  Aurora existing-save movement route (four checkpoints, all 37 sampled player
  states, 1,050 route/card records, zero timing drift and both nonblank frames
  at retraces 1,200/1,600). The latter executes 235,575 native calls with 1,074
  guard fallbacks. Original save backup hashes remain unchanged. Native Windows
  at `ce31672` builds the app and passes all 46 tests (38.16 s;
  [run 36987957549](https://github.com/chrissotraidis/bluewake/actions/runs/36987957549)).
  Source/archive audits pass. Traced O0 timings under concurrent compilation
  are not performance evidence. Combined optimized modules, native Windows
  translated-module exactness, sustained play and final player routes remain open.
- **SDK vector functions:** #29 preserves Elliott's exact three-file import
  `ed994c4`; adaptation `fc02056` credits him as co-author. Independent
  `--native-vec` certifies all nine bodies in every variant before rewriting;
  the versioned host handshake requires `BLUEWAKE_NATIVE_VEC=1` and the shared
  read-only observation predicate. Missing support stays Off. Fourteen cache/
  builder tests, three certification methods and Release/ASan/UBSan guards pass.
  The strict arm64 fixture covers 100,000 cases: 29,410 accepted executions
  match every CPU byte, including observation suffix, and the 256-byte RAM test
  area; 70,590 unsupported cases decline unchanged. Every leaf has accepted
  coverage. The donor's null-RAM read at address 0x10 was reproduced with
  ASan/UBSan and fixed. Combined preparation initially changed the certified
  dot-product body; the prepared-block pass now preserves selected vector
  ranges, and certification plus fixed-CPU/MEM1, direct-call, inline-FP/gather
  syntax checks pass. The host builds and CMake rejects unprepared modules.
  Source/archive audits pass. The fresh O0 private module links all 823 units,
  and all 29,410 accepted fixture cases also match through active module routing.
  An older ordinary module safely stays Off when the host requests native vectors.
  Three-way baseline/candidate-Off/candidate-On headless runs through 6,000
  retraces match all six CPU/RAM/REL checkpoints and 1,050 route/card records,
  with zero delivery/clock drift. The native path runs 723,168 calls with 576
  unchanged guard fallbacks. Native Windows at `fc02056` builds the app and
  passes all 47 tests (37.50 s;
  [run 36992627147](https://github.com/chrissotraidis/bluewake/actions/runs/36992627147)).
  The three-way 2,000-retrace Aurora existing-save movement route also passes:
  all four checkpoints, 37 sampled player states, 1,050 route/card records,
  zero delivery/clock drift and both nonblank frames at retraces 1,200/1,600
  match. The native path runs 8,023,202 calls with 6,693 unchanged guard
  fallbacks. The original save backup hash remains unchanged. These O0/traced
  runs are correctness checks. Combined optimized/native Windows module
  exactness, sustained play, save/reload, upgrades and final player routes remain
  open; no performance acceptance is claimed.
- **Prepared-block measurement attempt:** the quiet preflight passed, but the
  six-run alternating O2 comparison stopped after competing work was detected
  during the fifth run. Four completed measurements and the failed attempt
  are retained locally. They do not establish a speedup or complete the matched
  performance gate; rerun when the full measurement window can stay quiet.
- **SDK matrix functions and workers:** #30 preserves Elliott's seven-file
  import `a080b16`; adaptation `a294985` credits him as co-author. Independent
  `--native-math` certifies four SDK functions and retains cached dispatcher
  selection. Cached wrappers recheck the versioned host readiness/enable state;
  `BLUEWAKE_NATIVE_MATH=1` is required. Certified direct calls can use the same
  helpers. POSIX workers remain opt-in; Windows retains the donor serial path.
  Both zero-worker and two-worker fixtures pass 12,000 matrix/vector cases,
  360 arrays and 2,880 integer-helper cases against the private translated
  baseline. Every CPU byte is compared, plus the 64 KiB matrix/integer or
  256 KiB array RAM test area; constants are copied separately as read-only
  inputs. Two workers execute 120 parallel array batches. The integer-helper
  fixture does not select an additional GPR optimization in player modules.
  Synthetic identity/unchanged-fallback, Release and ASan/UBSan checks pass.
  A null-RAM read at address 0x1000 was reproduced and fixed. Fifteen builder/
  cache tests and three certification methods pass, including native-matrix
  direct-call removal on disable. Repeated preparation preserves identical
  header timestamps. Matrix/vector certification survives combined fixed CPU,
  prepared-block and direct-call rewrites; gather/global-MEM1 syntax passes.
  Host build, repository/source archive audits and rejection of unprepared
  modules pass. The fresh private O0 module links all 824 units. Routed serial
  and two-worker fixtures pass all 12,360 matrix/array cases with repeated
  cached enable/disable changes and unsupported-input fallback. Each mode
  executes 9,421 native calls and 11,958 readiness queries; the worker module
  executes 91 parallel batches and unloads normally. An older ordinary module
  stays Off when the host requests native math. Native Windows at `a294985`
  builds the app and passes all 48 tests (33.98 s;
  [run 36995946064](https://github.com/chrissotraidis/bluewake/actions/runs/36995946064)).
  Four-way baseline/Off/serial/two-worker runs match all six checkpoints through
  6,000 intro retraces and all four checkpoints through 2,000 saved-game retraces.
  Both routes match 1,050 canonical/card records with zero measured delivery or
  route-clock drift. The saved route also matches 37 sampled player states,
  scripted movement and both rendered images, preserving the source save.
  Enabled intro modes execute 426,166 native calls; the saved scene executes
  5,058,650, including 1,157 array calls. No scene array exceeds eight vectors,
  so these scenes do not exercise parallel worker batches. The separate function
  fixtures establish worker execution. A combined O0 candidate links all 829
  units with all migrated native helpers, prepared blocks, fixed CPU/MEM1,
  inline FP/GPR, direct calls and gather wrappers. Its 30,000 direct-call function
  cases match every CPU/RAM byte, and its routed matrix fixture passes. Combined
  baseline/Off/On intro runs match all six checkpoints and 1,050 canonical/card
  records with zero measured timing drift. Enabled native helpers and direct
  calls are confirmed active. The initial test-driver assertion incorrectly
  expected renderer batching in headless mode; the correct direct-writer mode
  was verified against the complete retained runs. The Aurora saved-game
  comparison, which requires actual batch selection, is still running.
  These are private developer builds from
  retained translation; combined optimized performance, clean final player builds
  and final gameplay remain open.
- **Menu persistence:** Mac `3222b915` / `c2905b7a`, isolated ordinary-module session:
  fresh Off, select 120, disable while retaining three steps, close/relaunch and
  visibly remain Off with byte-identical settings. Keyboard selection passes;
  automated pointer selection was unreliable. Pacing, display hot-plug, controller
  navigation, sustained play and audible output remain open.

A separate donor gather-wrapper probe overreads an undersized RAM buffer under
ASan; the maintained ordinary memory path does not. A second regression shows
the donor pipe shortcut bypassing registered guest aliases. The imported helper
foundation guards the subtraction and checks normal memory precedence before
using the writer. Native Windows at `ab8ff52` builds the app and passes all 41
tests (43.21 s; [run 36972986519](https://github.com/chrissotraidis/bluewake/actions/runs/36972986519)). Shared maintained memory contracts exercise ordinary/global
RAM, aliases, reservation, journal, endian and MMIO behavior. Additional tests
cover byte order, batching thresholds, writer changes, interpreter/MMIO flushes
and inline/exported dispatch boundaries. Sustained gameplay and optimized performance
qualification remain open. Explicit module preparation is now available independently of inline FP;
the host has explicit direct/batch opt-ins, with ordinary MMIO retained by
default and for diagnostics. The bounded host/renderer results above are now
available; direct calls now have bounded qualification, while native imports and
combined optimized gameplay/performance remain open work.
All measured wall times under concurrent compilation are excluded from performance
acceptance. No donor benchmark is transferred.

## Player build and platform gates

| Route / platform | Current evidence | Still required |
| --- | --- | --- |
| Owned-disc source / iOS | Fresh `95adeed` / runtime `99e47480`: disc validation, translation, mods, locally generated PGO, full module/app link, personal packaging and signature/provenance checks pass. Controlled interruption/resume preserves prior objects/profile hashes | Repeat final integrated-source selection where changed; physical launch, fresh/existing save/reload and in-place upgrade preserving data |
| PadMint / iOS | Adapter `018a9f0` (CLI 0.2.8), new workspace, exact `95adeed`, matching audited app-only input: translation/mods, new 23,000-retrace local training, full compilation and personal assembly pass; package integrity, module hash and exact source provenance rechecked | Player signing/install (assembled module is unsigned), final integrated-source acceptance, device run/save/reload/upgrade and trained performance |
| App-only / iOS | Fresh matching candidate excludes translated code; PadMint audit and public-assets gate pass locally | Final-source compatible old-module update and device acceptance; no upload/publication |
| Owned-disc / Mac | Bundled development host, private module correctness and isolated menu checks pass | Complete documented fresh player route, installation/launcher, actual game/save/reload/upgrade and sustained performance |
| Owned-disc / Windows | Native app/source fixtures pass; disc/recovery and preparation scripts integrated | Full clean module build/training, real disc/Unicode/UI/recovery, Direct3D/fullscreen/restart, controllers/audio/gameplay and save-preserving upgrade |
| PadMint / Mac, Windows, tvOS | No complete adapter path accepted; Mac remains planned | Do not claim support; implement/test before widening the matrix |
| Apple TV | Existing preview source and contributor work preserved | Physical hardware/storage acceptance for any claimed feature; no new acceptance inferred |

The baseline personal iOS build cannot establish acceptance for every later
integrated change. Existing artifacts and private profiles are not prerequisites
for the clean player route. Exact hashes and private evidence stay local.

## Report reconciliation

Reports from both repositories are investigation leads, not diagnosed or resolved
bugs. No issue is closed or externally commented on by this work.

| Report | Evidence / disposition | Next discriminating check |
| --- | --- | --- |
| [BlueWake #13: Pictobox](https://github.com/chrissotraidis/bluewake/issues/13) | Windfall shutter closes, picture stops, sound continues; reports Windows 11/120 FPS but exact product/module unclear. Maintainer already asked for identity | Establish exact build; isolated Windfall save, interpolation off/on, capture GPU/EFB copy and game-thread progress. No reproduced cause |
| [Donor #16: startup freeze](https://github.com/elliotttate/Wind-Waker-Recomp/issues/16) | 0.2.2 Windows logs reject PAL, then accept USA, initialize D3D12 on RTX 2050; stop after gxcore initialization. Also occurs on integrated GPU | Symbolized/blocked-thread evidence at first-frame boundary with exact app/module/pins; avoid assuming disc/settings/GPU cause |
| [Donor #5: settings/startup crash](https://github.com/elliotttate/Wind-Waker-Recomp/issues/5) | Earlier access violation at zero; #12 startup/restart safeguards do not establish a fix | Windows HLE/LLE and real Restart/settings reproduction; bounded crash/safe-mode recovery |
| [Donor #12: scripted music](https://github.com/elliotttate/Wind-Waker-Recomp/issues/12), [#1: intro](https://github.com/elliotttate/Wind-Waker-Recomp/issues/1) | #12 identifies Windows x86/0.2.2, ambient-only intro and bird/Zelda/sister/Ganon cues; Mac intro stream/sink evidence covers only its stated scene/build | Fresh-card and matching scripted-scene Windows guest/sink/output comparison, then audible output; do not extrapolate Mac intro captures |
| [Donor #6: bird slowdown](https://github.com/elliotttate/Wind-Waker-Recomp/issues/6) | Scene report; device/settings and matched baseline still needed | Matched quiet-device bird/Outset/busy-area serial measurements, frame-time tails, stalls/audio and 30-minute run |
| [Donor #2](https://github.com/elliotttate/Wind-Waker-Recomp/issues/2), [#8](https://github.com/elliotttate/Wind-Waker-Recomp/issues/8), [#14](https://github.com/elliotttate/Wind-Waker-Recomp/issues/14): controllers | Layout swaps cover part of request; unsupported 8BitDo mapping and arbitrary remap are not accepted | Real controllers, per-device layout/hot-plug/navigation/save preference checks |
| Donor #3/#18 Linux, #7 PAL, #4 HD HUD, #9 Switch, #10/#11 portable | Follow-up platform/UI requests; explicit data-directory override exists in Windows source | Record supported matrix; preserve unsupported-disc rejection, do not broaden claims |


## Next actions and retained work

1. Run matched prepared-block O2 measurements when the machine is quiet; both
   builds and the O2 function/intro/saved-game comparisons now pass. The direct-call three-way
   headless/Aurora comparisons pass; their traced wall times are not benchmarks.
   The copied-save route now demonstrates matched scripted movement; extend it
   to sustained scenes and save/reload acceptance.
   The baseline full PadMint package is complete; player signing and final-source/
   device acceptance remain open.
2. Directly import the remaining native game-math/skin enhancements,
   adapting BlueWake hooks and retaining contributor credit. Direct-call/register
   and certified J3D/vector/matrix source and bounded routing checks are now integrated. Complete combined optimized and host FIFO batching
   qualification, including scheduling/hooks, gameplay and matched measurements.
   Skin preflight reproduces a null-RAM sanitizer failure in the unmodified
   donor helper; a minimal null-CPU/RAM guard passes the private probe but is
   not yet integrated. Its donor body certificate also does not match our
   current preparation order; establish the independent differential evidence
   before adapting certification, rather than bypassing it.
3. Complete final-source clean player routes and the full gameplay matrix:
   fresh/existing saves, narrated intro, progression, scripted music/Pictobox,
   fullscreen/restart, real controllers, save/reload, upgrades and sustained play.
   Windows gameplay hardware and current physical iPad acceptance remain
   unconfirmed; continue independent work without counting those checks passed.
4. Reconcile the draft source stack into the intended maintained branch only with
   its acceptance evidence; finish unified instructions, credits and issue mapping.
   The [proposed migration notice](FORK_RECONCILIATION_EVIDENCE_2026-10-02.md#migration-proposal-not-published-to-the-donor)
   remains unposted. Keep the donor and its history intact until acceptance.

Primary `codex/fork-reconciliation` at `95adeed` retains the completed baseline
source-build/PadMint artifacts. The
managed `bluewake-disc-parity` worktree at `05df605` retains both completed O2
modules and the required private evidence.
The managed `bluewake-cpu-contract` worktree holds the current source stack and
private qualification artifacts; its nested runtime is `c2905b7a`. Both managed
worktrees contain needed unique artifacts and remain in use. No checkout is
removed or reset. Preserve saves, settings, inputs and signing material.
