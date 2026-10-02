# Fork reconciliation ledger, October 2, 2026

**In progress.** Chris and Elliott have agreed to consolidate development in
BlueWake. This migration directly imports Elliott's enhancements, preserves his
authorship and credits combined implementations with co-author trailers.
Source integration is consolidated in cumulative draft PR #37; required
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
`0568feddb332e0d9fe8a3a7cff330263f14af2f1` (runtime PR #4, stacked on #3).
Its product code is unchanged from `2218107d`; the follow-up makes the runtime
save-state test path portable to Windows. Patches through 0139 preserve BlueWake's safety changes, the later donor
post-texture correction, and PE token/capture readback. The previous fresh
player build and paused PadMint workspace remain fixed at runtime `18ba3b64`. Existing native
module/scene evidence below remains tied to its stated `c2905b7a` runtime. The translator remains
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
Both implementations are now imported; their remaining qualification is tracked
below before recommending cutover; donor commit descriptions
are not BlueWake validation. Do not silently repin the active qualification
builds or treat the fixed baseline as the donor's current head.

The later dual-texture patch is now imported into maintained runtime as
`94e9835`, preserving Elliott's authorship; adaptation `18ba3b64` credits him as
co-author and preserves the post matrices across save-state reloads. The exact
import loses those matrices after a synthetic FIFO save/reload; the new versioned
extension fixes that regression. Legacy frontend states still load with identity
post rows until the guest updates them; new post-texture states require this or
a newer runtime. Regular memory-card saves are unchanged. Direct/indexed FIFO
capture, per-draw snapshots, normalization/disable flags, valid/legacy states and
malformed-state rejection pass Release and ASan/UBSan, as do the donor matrix-fold
checks. Five related save/FIFO/replay/texture regressions and the Mac host build
pass. At source `f861eae`, the ordinary-module 2,000-retrace Outset movement route
matches the prior `0ea7ffb` / `c2905b7a` host: four full-state checkpoints, all 37
sampled player states, both nonblank frames and 1,050 route/card records match,
with zero measured delivery/clock drift and unchanged original save. An initial
private comparator compared JSON lists with regex tuples; comparing the completed
canonical receipts resolves that harness error without rerunning the game.
Native Windows at `499b369` builds the app and passes all 51 tests
([run 37008423184](https://github.com/chrissotraidis/bluewake/actions/runs/37008423184)).
The initial `f861eae` run built the app and passed 50 tests, but the frontend suite
crashed; the corrected Windows frontend test reserves 8 MiB stack: the current Mac compiler reports a 2,297,504-byte
main frame for the existing replay suite's multiple complete frontend objects.
A paired 3,000-retrace warp to Dragon Roost Cavern room 2 also matches all six
checkpoints, route/card/timing and both images, but visual inspection shows the
entrance barricade obscures the lava. This is not affected-scene acceptance.
Two further 2,500-retrace upper-bridge runs (including a camera-downward probe)
match five checkpoints each, route/card/timing and both frames per pair. Local
inspection still does not show the lava surface: the second view faces down onto
the platform. Those three room probes do not establish the colour fix.
A later paired boss-room view (`M_DragB`, room 0) directly shows the old
`0ea7ffb` / `c2905b7a` host's white lava becoming orange at `f861eae` /
`18ba3b64`. All four full-state checkpoints, 1,050 route/card records and timing
match, and the original save is unchanged. Both captures differ only within the
visible lava strip (59,685 and 63,951 pixels); the rest of each image is identical.
This accepts the reported colour correction in that bounded Mac scene, not
progression, other platforms or a performance claim. No donor screenshot or
benchmark is counted as BlueWake evidence.

Elliott's exact `haptics.c` / `.h` from `7ca0cb9` are retained in authored import
`193870f`. The adaptation connects retraces, SI motor decoding, Mac/Windows
controls and all three host source lists, preserving the Windows saved/session
preference separation and the iOS Classic stub. Two actual virtual-controller
regressions reproduce the imported menu behavior: Classic still forwards while
blocked, and Enhanced does not silence immediately when the Mac pauses retraces.
Both are corrected. Explicit shutdown clears output before SDL/CPU teardown;
DualSense watchdog callbacks validate their current timer under the joystick
lock, and disconnected slots can be reused. The test virtual controller is
closed/detached during shutdown. Strength/trigger controls apply to Enhanced.
At `558438a`, the full Mac host and eight focused regressions pass, including
Xbox-style rumble, DualSense expiry/rearm, ten reconnects, menu silence, shutdown
and saved preference preservation. Native Windows builds the app and passes all
54 tests ([run 37012303375](https://github.com/chrissotraidis/bluewake/actions/runs/37012303375)).
Its 2,000-retrace Outset movement route matches the prior `f861eae` host in all
four full-state checkpoints, 37 sampled player states, two frames and 1,050
route/card records, with zero measured delivery/clock drift and unchanged seed.
The iOS/tvOS haptics stubs compile with their SDKs; this is not a complete app build.

A follow-up reproduces feedback continuing after guest pattern cancellation or
port-zero rumble disabling. The adaptation clears cached shocks when withdrawn,
silences when no pattern remains and checks the guest's rumble flags. These
checks follow [the vibration cancellation behavior](https://github.com/zeldaret/tww/blob/main/src/d/d_vibration.cpp)
and [the controller enable mask](https://github.com/zeldaret/tww/blob/main/include/JSystem/JUtility/JUTGamePad.h);
they do not copy translated game code. At `9849eb0`, the Mac host and ten
focused checks pass; native Windows builds the app and passes all 56 tests
([run 37013449326](https://github.com/chrissotraidis/bluewake/actions/runs/37013449326)).
Five ASan/UBSan cases and the DualSense timer/reconnect case under TSan pass;
haptics and the fixture are instrumented, while linked runtime/SDL are not.
Mac keyboard interaction shows the default Enhanced/80%/triggers On, changes
strength to 37% and triggers Off, then selects Classic and Off with the retained
strength/trigger controls visibly disabled. Resume writes Off/37%/triggers Off
to an isolated settings file. A separate relaunch reads the same effective
runtime values and leaves the settings byte-identical. Visual inspection of
the reopened controls is still pending.
That menu run completes the 2,000-retrace route with all 37 sampled player states
and 1,050 route/card records matching, but the full-memory comparison differs at
retrace 500. Its changed window size and menu interactions are not a canonical
state comparison. A separate unchanged-route run at `9849eb0` matches all four
full-state checkpoints, 37 player states, both nonblank frames and 1,050
route/card records against `f861eae`, with zero measured delivery/clock drift
and unchanged original save. Automated
pointer interaction remains unreliable. Windows menu interaction, real
controllers and the game's visible vibration option remain pending; virtual
output is not felt-feedback acceptance. Repository/source-archive audits pass
for `9849eb0`.

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
| [#31](https://github.com/chrissotraidis/bluewake/pull/31) | `4cfd99e`, import `46365c2` | Native skinning with certified entry routing; full-CPU/MEM1 fixtures, three-way startup comparisons and all 49 native Windows tests pass; saved-game state/image comparisons pass; combined performance open |
| [#32](https://github.com/chrissotraidis/bluewake/pull/32) | `0ea7ffb`, import `ca8c584`, fixture `459df34` | Twelve certified game-math entries; full-CPU/protected-MEM1 fixtures, three-way intro comparison and all 50 native Windows tests pass; saved-game state/image comparisons pass; combined optimized qualification in progress |
| [#33](https://github.com/chrissotraidis/bluewake/pull/33) | `f861eae`, runtime `18ba3b64` | Later donor post-texture renderer fix with backward-compatible save-state loading; FIFO/fold/sanitizer checks and Outset state/image comparison pass; 51 native Windows tests pass; paired boss-room view confirms the white-to-orange lava correction |
| [#34](https://github.com/chrissotraidis/bluewake/pull/34) | `193870f` import, `558438a` / `9849eb0` adaptations | Controller feedback with preserved preferences, menu/teardown and guest-cancellation fixes; ten Mac checks, 56 Windows tests, bounded Outset and sanitizer checks pass; real controller qualification open |
| [#35](https://github.com/chrissotraidis/bluewake/pull/35) | `9c205c3` | Relocatable personal Mac builder; fresh shell/translation, packaging/signing and bounded existing-module relocation checks pass; complete player build and gameplay acceptance open |
| [#36](https://github.com/chrissotraidis/bluewake/pull/36) | `3392854` | Explicit combined Mac player build; fresh owned-disc training/compilation/packaging, bounded restart recovery and actual save/reload pass; remaining gameplay acceptance open |
| [#37](https://github.com/chrissotraidis/bluewake/pull/37) | `3392854` | Cumulative review against main, preserving all preceding ancestry, 26 Elliott-authored commits and 13 co-author trailers; current qualification and consolidation home |

Runtime [PR #1](https://github.com/chrissotraidis/RecompCore/pull/1) at `70bc9957`
rebases donor global MEM1/display overload work with bounds checks. Runtime
[PR #2](https://github.com/chrissotraidis/RecompCore/pull/2) at `c2905b7a` fixes
alias tracking above retail 24 MiB. Runtime [PR #3](https://github.com/chrissotraidis/RecompCore/pull/3)
at `18ba3b64` imports the later post-texture renderer correction and preserves it
in save states. All are drafts; all public checkpoints are source only. No source merge or test result implies release readiness.

## Required parity inventory

| Feature / platform | Donor source / dependency | BlueWake state / difference | Required acceptance / status |
| --- | --- | --- | --- |
| Interpolation, batching, upload staging, worker/cache improvements; Apple/Windows | main `b6f87e0`, `7f4f1a0`, `e5c6ae4`, `b39bd0d`; runtime through `9618e9d` | Integrated in #10 plus newer synchronization fixes in #12 | Integrated; final same-scene images, 60/120 pacing and sustained play open |
| Swapchain/fullscreen and orderly restart; Windows | `db2944e`, `1ce29ac`; runtime `9618e9d` | Swapchain fix already in maintained runtime; #12 orderly quit/relaunch replaces donor process-exit approach | Integrated; actual Windows F11/Restart/settings/startup recovery open |
| Disc picker, remembered disc, compressed import and recovery; Windows | `0b463cc`, `61e891e`, `640094e`; shared `disc_import.c`, nodtool | Launcher source integrated in #15; unique import folders retain original/converted/previous files; candidate native 29/29 regressions pass | Integrated; real supported/rejected/missing disc and UI/Unicode interruption/recovery checks open |
| Disc picker / launcher; Apple Silicon Mac | main `561ddbf`, `7a2ac23`, `5eaa981` | Developer host route exists; donor bundled-game packaging is intentionally excluded | Open; personal clean source build and app-only launcher with player's generated module, save/reload and installation |
| Camera, right-stick aiming/zoom/collision; desktops | main `887c26d`, windows `9511241` | Latest SDL queue timing integrated with window-scoped filtering; prior BlueWake camera/menu code retained | Integrated; SDL queue regression and bounded real Mac capture/orbit/wheel/release pass; vertical/collision, physical controller and other-platform acceptance open |
| Jump, sprint, quick doors, transitions; Apple/Windows | main `22fa284`, `d55ce11`, `2c9f16c`, `df62ae0` | Integrated; Apple touch controls use existing editor, new options remain opt-in | Fresh Mac bounded progression and scheduled-input jump/sprint on/off pass; full options, real controls and other platforms remain open |
| Fifteen Better Wind Waker options, 16:10; all claimed targets | main `b6f87e0`; DolRecomp `b8b5345` | Integrated; same verified base-source digest; new options require module rebuild, legacy fallback retained | Fresh Mac player module exports all 15 option metadata entries matching the manifest; relevant option gameplay and other platforms remain open |
| Desktop save states and climbing | main `b39bd0d`, windows `3ba8599`, `1510ed1` | Integrated; #12 adds checked/atomic serialization; states experimental, climbing off | Current Mac module F5/F9 and game save/reload pass across processes; climbing and other claimed platforms remain open; no new Apple touch state UI claimed |
| Controller face layouts/navigation | donor reports #2/#8/#14; existing SDL controls | #12 adds A/B and X/Y swaps, navigation/game-input isolation and virtual-controller checks | Integrated; real Switch Pro/Xbox/8BitDo hot-plug/menu/closing-input checks open; arbitrary remap is separate scope |
| Prepared-block/global-register module optimizations; Windows | `f319afa`, `8435ec7`, `16fabda`; scripts/windows transformers and cmake/composite helpers | Generic prepaid-block transform and portable strict A/B fixture imported; fixed-CPU preparation from `4b6b268` added separately with a declared module ABI and explicit builder opt-in. Both default off; module-owned MEM1 is integrated separately in #19, native batches are now imported separately | Open; isolate generic transforms from native/decomp work, private generated-code correctness and matched before/after performance |
| Inline floating-point interpreter operations; Windows | `4b6b268`; `inline_fp.h`, chunk header preparation | Separate `--inline-fp` opt-in, off by default; no game-native replacements or ABI change | Open; Corrected helper: arm64/Rosetta and native Windows checks pass; 30,000 module cases and 6,000-retrace arm64 boot match. Optimized module/performance/gameplay open |
| Direct cross-chunk/indirect calls and inline GPR save/restore; Windows | `4b6b268`, `742f1a0`, `76c688d`, `7576dc9` | Imported directly with Elliott's authorship in `d176f3e`; candidate adds versioned host readiness, shared feature predicates and guarded/certified register inlining. Builder options remain independent and off by default | Integrated with bounded checks; all 823 private O0 module units link, 30,000 active function cases and three-way headless/Aurora route/checkpoint/intro-image comparisons pass. All 45 native Windows tests pass. Optimized combined modules, gameplay and matched timing remain required |
| Gather-pipe batching and inline memory wrappers; Windows | `3e14287`, `6def7cd` | Generic helper foundation imported with original authorship; separate default-off `--gather-pipe` prepares modules. Host direct writer/batching now has separate default-off selection; bounded Mac route/card/intro-image comparisons pass. Undersized-RAM and pipe-alias precedence regressions fixed | Open; source-only memory/flush fixtures and nine cache tests pass locally; 813 chunks prepare reproducibly and unprepared inputs are rejected. Arm64 O0 function/boot comparisons pass with the host writer disabled; optimized modules, sustained renderer/gameplay qualification and matched timing remain required |
| Native math/J3D/skin/vector replacements; Windows | `689f042`, `845a589`, `833e6fb`, `1e97dca` | J3D rotation/translation functions integrated in #28 with certified builder preparation and versioned host opt-in; recovered-source attribution verified. Nine vector functions are selected independently in #29; four SDK matrix functions and optional workers are integrated in #30; skinning and twelve game-math entries are integrated in #31/#32 | Open; all five native families pass bounded routed function comparisons. J3D/vector/matrix/skin also pass arm64 O0 intro/saved-game state/image comparisons; game-math intro and saved-game checks also pass. Combined optimized/native Windows module checks and matched measurements required |
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
  comparison also passes all four checkpoints, 37 sampled player states, scripted
  movement, both images and 1,050 canonical/card records with zero measured timing
  drift. It confirms batch selection and 244,126,396 allowed direct-call boundaries.
  These are private developer builds from
  retained translation; combined optimized performance, clean final player builds
  and final gameplay remain open.
- **Native skinning candidate:** #31 preserves exact four-file Elliott import `46365c2`
  for the weighted-envelope matrix helper and its fixture; adaptation `4cfd99e`
  credits him as co-author. BlueWake adds
  independent `--native-skin` preparation and a versioned, default-Off host gate.
  The raw pre-transform body is certified only after independent full-function
  comparison; the entry hook supports both ordinary and direct dispatch.
  Twenty thousand randomized helper cases pass: 16,743 native results match
  every CPU byte (including the observation suffix) and every MEM1 byte;
  3,257 cases decline unchanged. The donor fixture assumed a Windows module
  completing this function in one dispatch; the portable fixture now follows
  its complete dispatch window and retains the initial single-dispatch mismatch
  as fixture-adaptation evidence. A reproduced null-RAM sanitizer failure is
  fixed by checking RAM before resolving guest addresses, with a null-CPU guard.
  Synthetic identity, unchanged CPU/RAM fallback and host handshake checks pass
  in Release and ASan/UBSan. Sixteen builder/cache tests and three certificate
  tests pass. The independent private O0 module links all 823 units. Its routed
  fixture passes another 20,000 full CPU/MEM1 comparisons, including selection
  disabled every 31st call, with 16,184 native calls and 19,354 readiness queries.
  An older ordinary module stays Off when native skinning is requested. Combined
  fixed CPU/GPR/block/direct-call/gather preparation and syntax pass. Native
  Windows builds the app and passes all 49 tests (38.49 s;
  [run 37000118965](https://github.com/chrissotraidis/bluewake/actions/runs/37000118965)).
  Baseline/Off/On startup runs match all six checkpoints through 6,000 retraces,
  1,050 canonical/card records and zero measured timing drift. Enabled skinning
  executes 1,726 native calls over 8,948 joints, with 144 translated fallbacks.
  The rendered baseline/Off/On saved-game comparison also passes: four checkpoints,
  37 player samples, scripted movement, two images and 1,050 canonical/card records
  match with zero measured timing drift. The copied source save is unchanged.
  Combined optimized performance and final gameplay remain open.
- **Native game-math candidate:** #32 source `0ea7ffb` and exact five-file Elliott import `ca8c584`
  preserves the donor's native arithmetic, which
  passes 130,000 private cases across twelve supported entries and one explicitly
  unsupported entry: 67,707 native results match and 62,293 decline unchanged.
  The portable fixture follows complete dispatch windows and compares every CPU
  byte, including the observation suffix. It compares the writable 24 KiB RAM
  region per case, protects the rest of MEM1 read-only and compares all MEM1
  after each entry. No microbenchmark result is transferred. Several donor
  certificates reflect a different preparation stage. BlueWake now certifies all
  nineteen pre-transform fragments and twelve entries before other native hooks;
  missing dependencies, modified variants/hooks and host-observed internal PCs
  reject preparation before changing sources. A versioned host gate keeps the
  feature Off by default. The box-line routine additionally asks the host about
  its certified register-save call with that call's fixed return PC before any
  state changes. Synthetic guards cover denial of this internal call, unchanged
  fallback and null CPU/RAM; ASan/UBSan pass. Seventeen builder/cache tests and
  six synthetic certificate tests pass, including host-source cache invalidation.
  The independent O0 module links all 823 units. The routed fixture passes
  67,707 complete CPU/RAM comparisons, including 2,190 disabled calls; all twelve
  native entries execute (65,517 calls; 70,645 readiness queries). Fixture `459df34`
  corrects an uncovered toggle: selecting every 31st generated scenario only
  chose deliberately misaligned inputs that declined before routing. It now
  selects from accepted inputs and requires both selections per supported entry,
  with zero readiness queries while disabled. The earlier raw arithmetic result
  remains valid; its claim of disabled routed coverage was not established.
  An older ordinary module stays Off when native game math is requested.
  Same-host ordinary/candidate-Off/candidate-On intro runs pass 6,000 retraces:
  all six full-state checkpoints and 1,050 route/card records match, with zero
  measured delivery/clock drift. Native Windows at `0ea7ffb` builds the app and
  passes all 50 tests (37.45 s;
  [run 37003496869](https://github.com/chrissotraidis/bluewake/actions/runs/37003496869)).
  Corrected-fixture revision `459df34` also passes all 50 Windows tests (34.06 s;
  [run 37005141587](https://github.com/chrissotraidis/bluewake/actions/runs/37005141587)).
  The three-way 2,000-retrace Aurora copied-save movement route also passes:
  four checkpoints, all 37 sampled player states, 1,050 route/card records and
  both nonblank frames at retraces 1,200/1,600 match, with zero measured timing
  drift. Movement is observed in all three modes and the original save backup
  hash is unchanged. This does not establish save-write/reload or upgrade
  acceptance.
- **Combined through game math:** source `0ea7ffb` / runtime `c2905b7a` builds
  a fresh O0 developer module with prepared blocks, fixed CPU/MEM1, inline FP/GPR,
  direct calls, gather and all five native helper families. It passes 30,000
  direct-call function cases, the routed matrix fixture, 20,000 skinning cases,
  and 130,000 game-math cases (including all twelve native entries and the
  corrected disabled-routing checks). Three-way ordinary/all-Off/all-On intro
  and Aurora copied-save movement comparisons also pass: six intro checkpoints,
  four saved-route checkpoints, all 37 sampled player states, both nonblank
  saved-scene frames and all 1,050 route/card records match, with zero measured
  delivery/clock drift. All five native helper families and direct calls execute;
  gather uses direct mode headless and actual batch mode with Aurora. Original
  save backup hashes remain unchanged. The O2 combined/reference pair has built
  from frozen module/runtime/prepared inputs with the same compiler and three
  existing O1 fallback chunks; the input digest remains unchanged. The combined O2 module passes the
  direct-call, matrix, skinning and game-math function fixtures against the ordinary
  O0 module. Four-way O0/reference-O2/combined-Off/combined-On headless intro
  comparison passes all six full-state checkpoints and 1,050 route/card records,
  with zero delivery/clock drift. The rendered saved route matches all 37 player
  states, both images and route/card records, but the strict MEM1 comparison
  fails. The timing run therefore stops before measurement.

  A focused save-state capture at retrace 501 reproduces 17 differing bytes
  between O0 and reference O2, entirely within guest `gxData` texture-region
  descriptors and its next-region counter; CPU state is identical. Repeating
  reference O2 reproduces those bytes. Disabling only asynchronous EFB depth
  feedback makes the retrace-500 checkpoint and saved CPU/MEM1/ARAM/aliases
  identical. The full four-way rendered route with that controlled input also
  passes: four complete checkpoints through retrace 2,000, 37 player states,
  both images and all route/card records match, with zero delivery/clock drift.
  Production depth feedback remains enabled. No checkpoint bytes are masked,
  and the original live-depth failure is retained. The six-run alternating-order
  timing comparison with normal live depth feedback completes on an Apple M3 Max,
  with no competing compiler/game process detected before or after each run.
  The 1,200–2,000 retrace Outset window measures median equivalent game rates of
  **24.43 FPS reference versus 30.01 FPS combined (+22.8%)**, with original 30 Hz
  logic and interpolation off. All route/card records match. Frame-time tails
  do not improve: per-run p99 is 42.64–43.37 ms reference and 44.42–46.33 ms
  combined; frames above 50 ms total one versus two across the three runs each.
  These are bounded developer-build measurements using retained translation,
  not sustained gameplay, clean player-build or donor Windows performance proof.
  Select the combined path for an explicit Mac player-build candidate, preserving
  the ordinary path until end-to-end acceptance. Live-depth full-state equality
  and overall performance parity remain unaccepted.
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
available; direct calls now have bounded qualification, while combined optimized gameplay/performance remains open work.
All measured wall times under concurrent compilation are excluded from performance
acceptance. No donor benchmark is transferred.

## Player build and platform gates

| Route / platform | Current evidence | Still required |
| --- | --- | --- |
| Owned-disc source / iOS | Fresh `95adeed` / runtime `99e47480`: disc validation, translation, mods, locally generated PGO, full module/app link, personal packaging and signature/provenance checks pass. Controlled interruption/resume preserves prior objects/profile hashes | Repeat final integrated-source selection where changed; physical launch, fresh/existing save/reload and in-place upgrade preserving data |
| PadMint / iOS | Adapter `018a9f0` (CLI 0.2.8), new workspace, exact `95adeed`, matching audited app-only input: translation/mods, new 23,000-retrace local training, full compilation and personal assembly pass; package integrity, module hash and exact source provenance rechecked | Player signing/install (assembled module is unsigned), final integrated-source acceptance, device run/save/reload/upgrade and trained performance |
| App-only / iOS | Fresh `3392854` shell excludes translated code; PadMint 0.2.9 audit and public-assets gate pass locally | Final-source compatible old-module update and device acceptance; no upload/publication |
| Owned-disc / Mac | Fresh `3392854` / runtime `18ba3b64`: owned-disc translation, new training, module compilation, signed relocatable app and actual game save/separate reload pass; bounded completed-build restart recovery passes | Audible intro/scripted music, documented state/rebuild restrictions, remaining real camera checks, real controllers and sustained performance; local replacement from the recorded earlier developer app now passes |
| Owned-disc / Windows | Native app/source fixtures pass; disc/recovery and preparation scripts integrated | Full clean O2 module build (current builder has no PGO training), real disc/Unicode/UI/recovery, Direct3D/fullscreen/restart, controllers/audio/gameplay and save-preserving upgrade |
| PadMint / Mac, Windows, tvOS | No complete adapter path accepted; Mac remains planned | Do not claim support; implement/test before widening the matrix |
| Apple TV | Existing preview source and contributor work preserved | Physical hardware/storage acceptance for any claimed feature; no new acceptance inferred |

Mac player-route implementation now extends the existing builder with
`--platform macos`, a static desktop shell and a relocatable personal bundle.
Packaged apps resolve their own game files and cannot fall back to developer
checkout caches. Personal app replacement preserves previous builder output;
normal saves/preferences remain in Application Support and experimental states
use its `States` subdirectory. The default player launch uses real-time pacing
and HLE audio, with explicit diagnostic overrides retained.

Fresh Mac shell compilation, ad-hoc signing and deep/strict signature checks
pass, with system-only host dynamic dependencies. Its local app-only archive
passes the content gate (the gate accepts archive files, not `.app` directories).
Fresh owned-disc extraction/translation produces all 415 RELs and the verified
composite digest. Four packaging preservation/failure checks, eight existing
launcher checks and seven signing-identity regressions pass. The rebuilt app-only
shell exits with builder guidance when launched outside the checkout without
game files; it does not find a developer module implicitly.

A personal app relocated outside the checkout launches from bundled inputs,
using the existing ordinary test module and fresh extracted files. With the
same unpaced diagnostic setting as the reference, its 2,000-retrace Outset
route matches all four complete guest checkpoints, 37 player states, both
captured frames and 1,050 route/card records, with zero delivery/clock drift.
The source save is unchanged. The normal paced launch also matches movement,
frames and route/card records, but differs in MEM1 checksums; disabling only
pacing restores the checkpoint match. This establishes the setting difference
in this bounded route, not general correctness of paced gameplay. At that earlier checkpoint, fresh module compilation/training and game saves
were still open; the final fresh app evidence below now closes those bounded
gates. Updates and sustained acceptance remain open. Those runs overlapped
compilation and are not benchmarks.

The Windows builder exposes individual module-preparation switches. The Mac
builder now offers an explicit `--combined-optimizations` candidate using the
same ordered steps, with matching training/compiler options and bundle launch
defaults. Ordinary builds explicitly reset those compiler switches; changing
selection or an interrupted preparation preserves the previous source and
regenerates it. Two synthetic cache/resume cases and five packaging cases pass.
The updated static host and locally signed bundle launch outside the checkout
with the retained combined test module: all native families/direct calls and
gather batching activate without explicit feature environment variables. Its
controlled-depth 2,000-retrace route matches four complete checkpoints, both
images and all route/card records; the original save remains unchanged. This
qualifies bundle configuration. The subsequent fresh owned-disc run at
`3392854` completes extraction/translation, all mod variants, combined preparation,
new 23,000-retrace local training, final O2 compilation and signed personal
packaging. Dependency pins, all 415 RELs, profile/module provenance and deep/strict
signatures pass. The bundled app runs outside the checkout without developer
game-path overrides. A pre-armed restart was interrupted during reconfiguration
of this completed build; 1,485 finished objects and profiles/module survived,
and the same-command resume completed with no compilation work. This does not
claim interruption during initial compilation.

The fresh app then used the game's Save menu to write the selected slot, quit,
and separately reloaded the saved card into visible, responsive Outset gameplay.
Independent checks validate both selected-slot copies and the container, preserve
the other slots and original seed, and confirm the saved payload survives reload.
The first harness rejected an `Opening.arc` marker occurring after quit; its
failure is retained. A title-screen capture and sequence review identified that
over-broad assertion, and the separate reload passed. Scripted movement is not
physical-controller acceptance; this functional run during compilation is not
a performance or audible-output result.

On that same fresh app, keyboard-driven fullscreen on and FPS-overlay changes
persist across a separate launch. The reopened menu shows both selected and
Smooth Motion still off. Switching fullscreen off and resuming returns to visible
windowed Outset. F5/F9 also saves and restores a current-module state in the same
process: retrace/PC/MEM1 match, with 150 fields and zero missing or mismatched.
Visible gameplay continues; both processes exit normally and copied/original
cards remain unchanged. Pointer automation did not toggle the control; keyboard
navigation worked. This closes the bounded menu/fullscreen persistence and
current-state checks, not physical controls, audio, old-module/cross-process
state compatibility, in-place updates or performance.

A further fresh-card run of the final app completes 23,000 retraces and reaches
player control at 20,255. Three inspected captures show the opening story and
Outset tower dialogue. Its 383.3015 seconds of 32 kHz guest DMA PCM contain
nonzero output in all 39 ten-second windows. Capture occurs before the platform
sink: correct music and audible speaker output remain unaccepted. A separate
app process also restores the current-module state at retrace 3,272 with all
150 fields matching, resumes visible Outset, and exits normally with unchanged
card/settings. Old-module and other-scene state compatibility remain separate.

Final-source PadMint 0.2.9 (`de13bd7`) source-only acceptance passes in a new
workspace. Its full iOS run also completes fresh 23,000-retrace local training;
final module compilation is interrupted for low disk space; personal assembly remains unverified. The matching
`3392854` app-only shell passes both content audits. Earlier `95adeed` results
above remain historical evidence. Native Windows source-only host CI
[37033022272](https://github.com/chrissotraidis/bluewake/actions/runs/37033022272)
passes all 56 tests; this is not a Windows owned-disc gameplay result.

The baseline personal iOS build cannot establish acceptance for every later
integrated change. Existing artifacts and private profiles are not prerequisites
for the clean player route. Exact hashes and private evidence stay local.

### Local Mac app replacement acceptance

The earlier developer personal app (source `2ecf471`, explicitly modified at
build time) and the clean final `3392854` app both load the same isolated copied
save, respond to scripted movement and exit normally after 2,500 retraces.
The old app is retained and the new app replaces it at the same installed path.
Card, settings and SRAM hashes remain identical across replacement and both
launches. All four captured frames were inspected and show rendered Outset with
Link moving/turning. The replacement signature passes deep/strict verification.
This accepts the bounded local Mac replacement from that developer app; it does
not establish an official release-to-release upgrade, iOS installation, arbitrary
old save-state compatibility, audible audio or sustained performance.

### Fresh Mac bounded progression acceptance

The initial fresh route continued scheduled A presses after player control.
At retrace 20501 the A bit is set in ladder procedure 60; at 20502 the guest
changes to procedure 39 above ground. The ordinary-module comparison was
stopped when this test-input confound was identified; it establishes no
candidate regression. Both attempts remain preserved.

With the same final app and route, ending scheduled dialogue A presses at
20200 resolves the ladder failure. The guest exits the ladder at Y=150 at
retrace 21066, completes all 30 outdoor waypoints at 22009 and performs the
door confirmation at 22024. Guest overlap resets and completes with a new
player actor. Captures at 24001 and 26002 show the rendered wooden interior;
the player returns to ordinary idle state with no active event and the process
exits normally at 27000 retraces. The original save seed remains unchanged.
This accepts this fresh-start/ladder/outdoor/door/indoor route, not whole-game
progression, physical control, correct audible music or sustained performance.
No game-source change was needed for the test-input problem. The log does not
print the expected archive name, so no archive-name claim is made.

### Final iOS build interruption and resume

The owned disk guard interrupted the final PadMint compile below 5 GiB free.
The failed run and build log remain preserved; 567 completed object hashes and
the merged compilation profile were recorded before resuming. Completed Mac
mod sources contained byte-identical copies: APFS shared-block replacements
preserve all 5,215 paths, contents and checked metadata while sharing about
6.5 GiB of duplicate storage. No saves, binaries or iOS build inputs were
removed. The identical PadMint command reused verified local training and
resumed with 256 compile steps remaining, then stopped again below 5 GiB after
26 further steps. All original 567 object hashes/mtimes and the prior merged
profile are unchanged; 593 objects are retained now. Final assembly is open.
Do not restart until storage headroom is stable. The maintainer has been asked
to free at least 30 GiB or provide suitable external storage.

### Mac mouse and follow-camera diagnosis

On the exact fresh app, real mouse capture and horizontal drag visibly rotate
Outset's camera. Escape releases capture and F1 opens settings; normal quit
preserves the copied card. Real wheel input reaches the camera, but the first
runs keep the effective scale at 1.00 while requesting 0.50 or 2.00. The ordinary
module reproduces this, so it does not establish a combined-module regression.

A diagnostic then guarantees guest movement and A input before wheel events.
The camera's frozen flag clears, readiness changes from 0/0/0 to 1/1/1, its
engine clock advances from 0 to 448 and effective zoom reaches both requested
scales, 0.64 and 1.57. Inspected captures show a closer and farther view; a nearby
post obstructs the farther view. The process exits normally and the copied card
is unchanged. Earlier short live-input attempts did not activate this follow
camera reliably. The evidence resolves the suspected stuck-zoom diagnosis as a
dormant-camera test precondition; no guard removal or source fix is justified.
Complete real wheel/vertical input, camera collision and physical-controller
acceptance remain open. This diagnostic is not a performance result.


### Subsequent real wheel and movement-option acceptance

With the same fresh app and copied save, guaranteed guest movement/A first
activates the follow camera, followed by real SDL mouse input without scripted
mouse events. Wheel input visibly produces closer and farther Outset views;
the trace reaches effective/requested zoom 0.50/0.50 and 2.00/2.00. Escape
releases capture and F1 opens the paused settings menu. Normal quit returns 0;
copied and original cards are unchanged. Two vertical-drag automation attempts
produce no further pitch change, without independently established relative
motion delivery. Vertical input remains unverified; no source defect is inferred.

Two further runs use the fresh app and identical copied saves with jump/sprint
explicitly off and on. Scheduled jump at 1500 is ignored when off. When on,
the guest enters auto-jump at 1501, rises airborne, lands and returns to grounded
idle by 1534. Scheduled sprint changes the top-speed/animation parameters from
17/2.3 to 25.5/3.45, then restores top speed 17 on release at 1880; the disabled
run retains 17/2.3. Six rendered frames were inspected. Both runs exit normally
and preserve their copied cards/settings. The enabled run reaches the ordinary
ladder before the later targeting check, so its rejected jump establishes no
isolated targeting result. The different positions after jumping also prevent
using these runs as a matched sprint-distance or performance comparison.
Physical controllers and optional plain-wall climbing remain open. The later
keyboard settings check below qualifies selected UI toggles.

A read-only metadata check loads the final packaged personal module, verifies
its recorded hash and calls its option accessors: all 15 names, indices, defaults
and titles match the tracked manifest, and the flags/native-hook/write exports
are present. This closes export/metadata availability for that module, not the
15 options' gameplay behavior. No translated code or private test artifacts
are included in this source-only record.

The fresh app's keyboard-driven Gameplay menu also preserves explicit choices
across a separate launch: Better Wind Waker on, instant text off, Brisk Sail on,
jump off and sprint at 1.00. The restarted UI shows those choices and runtime
startup selects the corresponding option list with 40 values written. Wall
climbing and Smooth Motion remain off. Re-enabling jump through the live menu
and pressing Space invokes the guest jump at retrace 3712, reaches airborne
state and returns to grounded idle by 3747. Both processes exit normally;
copied and original cards are unchanged. The second run changes only the
intended jump preference. This accepts the tested keyboard menu/persistence
and live-jump flow, not pointer/controller navigation, each option's scene
behavior, audible music or sustained performance.

### Pictobox capture blocker on the fresh Mac candidate

The exact fresh `3392854` app and packaged module reproduce a black shutter
on Mac/Metal, at original 30 Hz with interpolation and Better Wind Waker off.
An isolated copy of the preserved Windfall fixture is loaded; the regular
Pictobox is granted in guest memory because that fixture does not own it.
Scheduled X enters the view, A takes a photo at retrace 2600, and B at 3300
fails to return to gameplay. Seven inspected captures show the normal view
before the photo and the same black shutter afterward. Rendering and retraces
continue through the normal 4200-retrace exit. Copied card, seed and preferences
are unchanged. This is a bounded item-path diagnostic, not natural item
acquisition or reproduction on the reporters' Windows hardware.

The pre-photo state has capture step 0; the post-photo state has step -1,
cleared capture buffers and no initialized compression thread. The reference
capture routine uses -1 after its draw-sync timeout, matching this state.
Runtime `18ba3b64` models PE finish delivery but lacks PE token delivery and
its token MMIO register. A missing draw-sync callback is the leading diagnosis;
it is not yet a validated fix or proof that the Windows reports share its cause.
A second run samples capture step 2 at retraces 2605 and 2625, with both
buffers allocated, the timeout alarm armed and the photo callback registered;
by 2700 the timeout has set step -1 and freed the buffers. Both runs exit
normally and preserve their copied saves. The next check tests the smallest
complete token-delivery correction with interrupt/acknowledgment regressions,
photo completion, B return, repeated captures and save/reload. Photo content
must also be inspected: clearing the shutter alone is insufficient.

### Pictobox repair candidate, October 3

Runtime `2218107d` ([PR #4](https://github.com/chrissotraidis/RecompCore/pull/4))
completes PE token delivery with independent token/finish masks and acknowledgments.
The first token-only experiment reached the guest compressor, which then halted
on an all-zero capture buffer. The complete candidate submits the actual EFB
copy and reads I8/RGB565 pixels back into tiled guest RAM before publishing the
token. It does not bypass the capture state or fabricate photo pixels. The
BlueWake host adds the draw-sync boundary and a separate optional PE state chunk,
preserving the old named interrupt field's layout and preflighting the new chunk
before restoring CPU or memory.

Six focused runtime/interrupt/edge/save-state/texture tests pass, along with the
encoder's ASan/UBSan fixture and the Mac host build. The final test host hash is
`70168c1e333cd53ea141ee3b066f1684168566780272d5ae48d1bc4aecfef1da`,
with unchanged fresh-player module
`3edc9c4dbf6d1f3fc9f840efa0e7ea39ffdd0333688d7876da14b0f271b34c7e`.
The host was built from modified `b3f3f53` with the source now checkpointed in
this PR and runtime `2218107d`; the frozen `3392854` baseline stays intact.

On Mac/Metal at original 30 Hz and interpolation off, regular and Deluxe
Pictobox previews contain the actual grayscale and color scene, respectively.
Accepting a photo changes the remaining count from three to two; a repeated
capture, cancellation and return to gameplay pass. These are bounded diagnostics
on a copied Windfall fixture, with the relevant item granted in memory.
Both photo types then pass the actual game Save menu and a separate normal
launch from the saved card, without a state load or item grant on that launch:
the album displays the corresponding stored grayscale/color image and one recorded
pictograph.
Both copies of the changed quest log have valid checksums; the other two quest
slots are unchanged. Originals and preferences are preserved.

The first persistence harness could not open the menu because its restored
state contained disabled automated button schedules. Explicit scripted menu
presses resolve that harness problem. One intermediate restored-state save
panel showed unrelated item-message text before the final saved confirmation;
this remains a state/message restoration observation, not a Pictobox freeze.
The pre-fix state also loads all 150 host fields without missing/mismatched
fields, then takes an actual grayscale photo, cancels and returns to gameplay.
This closes the bounded Mac Pictobox checks. Native Windows reproduction and
the broader gameplay/performance gates remain open. Private receipts and captures
stay local; neither report is closed and no builds are published.

### Camera qualification and Windows test follow-up, October 3

The same `70168c1e…` Mac host passes queued SDL pitch and collision checks
on copied Outset and Windfall saves. Mouse capture is active before the measured
input sequence. Outset pitch moves from the normal view to a low angle, then
75 degrees high and back to 31.8 degrees; six inspected frames match. At the
low angle the game corrects requested pitch -35 degrees to about -32.17,
shortens the eye distance from 249.8 to 174.4 units and keeps eye Y 160.5 above
water Y 145.9. A Windfall orbit beside the stone arch similarly pulls the camera
in to 105 units from its 250-unit requested radius, corrects its angle, and
restores the radius after the obstruction. A focused eight-frame replay shows
the closer view and return without drawing through the wall. Both directions
of orbit complete; copied saves remain unchanged. These bounded gameplay checks
accept the camera's queued-input pitch and collision path, not physical mouse
feel or controller acceptance. CUA vertical drags remain unreliable as an input
source; their incomplete run is retained rather than counted as a pass.

Windows run [37062541658](https://github.com/chrissotraidis/bluewake/actions/runs/37062541658)
exposed an outdated synthetic host-save fixture missing the new interrupt state.
`b4b0aea` provides the real PE type/state and verifies its serialized bytes; the
fixture passes locally and on Windows. Run
[37063695344](https://github.com/chrissotraidis/bluewake/actions/runs/37063695344)
then links the host and passes 57 of 58 tests, including the new texture encoder.
The remaining runtime-suite assertion is its hard-coded `/tmp` output path,
which Windows does not provide. Runtime `0568fedd` uses the working directory,
as the suite's other file fixtures do; the local runtime suite passes again.
A new Windows run is required. These fixture-only changes do not alter the
validated Mac host or require rebuilding its personal module.

## Report reconciliation

Reports from both repositories are investigation leads. The bounded Mac
Pictobox reproduction above does not establish the Windows reports' cause.
No issue is closed or externally commented on by this work.

| Report | Evidence / disposition | Next discriminating check |
| --- | --- | --- |
| [BlueWake #13](https://github.com/chrissotraidis/bluewake/issues/13), [donor #19: Pictobox](https://github.com/elliotttate/Wind-Waker-Recomp/issues/19) | Reporter confirmed donor Windows x64 0.3.0 on Windows 11/RTX 4070; a second Windows/Nvidia user confirms the same symptom. Windfall photo shutter blacks out while sound continues, with 120 displayed FPS / 30 game FPS, maximum render settings and 16:9. Exact binary-to-source identity still unverified | Mac final-candidate diagnostic now reproduces the black shutter with interpolation off and capture timeout state; see above. Runtime #4 and the host boundary now pass bounded Mac regular/Deluxe photo, repeat/cancel and both card-save/reload checks and legacy-state capture. Verify Windows separately. Neither issue closed |
| [Donor #16: startup freeze](https://github.com/elliotttate/Wind-Waker-Recomp/issues/16) | 0.2.2 Windows logs reject PAL, then accept USA, initialize D3D12 on RTX 2050; stop after gxcore initialization. Also occurs on integrated GPU | Symbolized/blocked-thread evidence at first-frame boundary with exact app/module/pins; avoid assuming disc/settings/GPU cause |
| [Donor #5: settings/startup crash](https://github.com/elliotttate/Wind-Waker-Recomp/issues/5) | Earlier access violation at zero; #12 startup/restart safeguards do not establish a fix | Windows HLE/LLE and real Restart/settings reproduction; bounded crash/safe-mode recovery |
| [Donor #12: scripted music](https://github.com/elliotttate/Wind-Waker-Recomp/issues/12), [#1: intro](https://github.com/elliotttate/Wind-Waker-Recomp/issues/1) | #12 identifies Windows x86/0.2.2, ambient-only intro and bird/Zelda/sister/Ganon cues; Mac intro stream/sink evidence covers only its stated scene/build | Fresh-card and matching scripted-scene Windows guest/sink/output comparison, then audible output; do not extrapolate Mac intro captures |
| [Donor #6: bird slowdown](https://github.com/elliotttate/Wind-Waker-Recomp/issues/6) | Scene report; device/settings and matched baseline still needed | Matched quiet-device bird/Outset/busy-area serial measurements, frame-time tails, stalls/audio and 30-minute run |
| [Donor #2](https://github.com/elliotttate/Wind-Waker-Recomp/issues/2), [#8](https://github.com/elliotttate/Wind-Waker-Recomp/issues/8), [#14](https://github.com/elliotttate/Wind-Waker-Recomp/issues/14): controllers | Layout swaps cover part of request; unsupported 8BitDo mapping and arbitrary remap are not accepted | Real controllers, per-device layout/hot-plug/navigation/save preference checks |
| Donor #3/#18 Linux, #7 PAL, #4 HD HUD, #9 Switch, #10/#11 portable | Follow-up platform/UI requests; explicit data-directory override exists in Windows source | Record supported matrix; preserve unsupported-disc rejection, do not broaden claims |


## Next actions and retained work

Chris reoriented the loop after the initial integration campaign. The next
iterations prioritize the complete player experience; see the current section
of [GOAL_LOOP.md](../GOAL_LOOP.md). The remaining cutover gates are:

1. **Pictobox Windows qualification:** bounded Mac regular/Deluxe photos,
   repeat/cancel, both card-save/reload routes and legacy-state capture pass
   with runtime `2218107d`. Verify the reporters' Windows path separately;
   retain the failing baseline and avoid claiming cross-platform acceptance.
2. **Remaining Mac qualification:** complete real mouse/controller and
   required gameplay-option checks, audible intro/scripted music and sustained
   play with matched measurements on an idle host. Queued pitch and water/wall
   collision now pass. The fresh build, bounded
   progression, local upgrade, save/reload, settings and current-module states
   stay accepted unless a relevant change invalidates them.
3. **Final iOS player path:** resume the existing `3392854` build only after
   stable disk headroom is available. It is stopped, not running; 593 objects
   and verified profiles remain intact. Inspect final assembly/provenance, then
   finish coordinated signing, device save/reload and preserved upgrades.
4. **Windows player acceptance:** complete clean O2 builds and native
   gameplay/report checks. Hardware availability remains unconfirmed; continue
   independent work without seizing devices or counting missing checks as passed.
5. **Consolidation and migration preparation:** continue through cumulative
   PR #37. Land maintained runtime in `bluewake-next`, not the unrelated default
   `codex/galaxypad-integration`, and BlueWake in `main` with authorship retained.
   Complete one support matrix, tested instructions and report mapping. The
   [proposed migration notice](FORK_RECONCILIATION_EVIDENCE_2026-10-02.md#migration-proposal-not-published-to-the-donor)
   remains unposted until migration is accepted.

Primary checkout now uses `codex/fork-consolidated` for the cumulative source
changes; branch `codex/fork-reconciliation` at `95adeed` and its completed
baseline source-build artifacts remain preserved. The
managed `bluewake-disc-parity` worktree at `05df605` retains both completed O2
modules and the required private evidence.
The managed `bluewake-cpu-contract` worktree holds the current source stack and
private qualification artifacts; its nested runtime is `18ba3b64`. The completed
combined/reference O2 builds used frozen copies of source/runtime `0ea7ffb` /
`c2905b7a`; later renderer edits cannot change their inputs. Both managed
worktrees contain needed unique artifacts and remain in use. No checkout is
removed or reset. Preserve saves, settings, inputs and signing material.
