# BlueWake Implementation Plan

**Current plan — September 9, 2026:** Route B whole-unit integration is selected
and the goal loop is active by user request. Follow
`status/REORIENTATION_2026-09-09.md` for execution order and acceptance.
Route A is frozen as an oracle. The two-track authorization and calendar estimates
below are historical; they do not override current decisions or the PRD.

Two-track engineering plan. **Track A is the current bounded prototype** and
must run first. **Track B preserves the original decompilation/Aurora plan**
as the cleaner long-term architecture; it is not authorized as product work
until Track A reaches authentic no-JIT gameplay or a future decision explicitly
selects Track B.

See `FEASIBILITY_REASSESSMENT_2026-08-13.md` for evidence boundaries and stop
criteria. Do not begin UI, touch, packaging, or product-shell work before the
Track A macOS gameplay gate.

## TRACK A — Mac-first static-recompilation prototype

### A0 — Private input and REL manifest

**Objective:** establish one reproducible `GZLE01` USA revision 0 input
without adding game data to Git.

- Verify the user-owned disc and `main.dol` against pinned identifiers.
- Extract into an ignored private workspace.
- Normalize loose RELs and `RELS.arc` contents into one inventory.
- Record filename, module ID, source location, size, hash, section count,
  imports, and relocations.
- Resolve the existing 415-versus-416 count discrepancy from the disc-backed
  manifest; fail explicitly on missing or duplicate module IDs.

**Exit:** reproducible manifest, with no Nintendo data or derived generated
module tracked by Git.

### A1 — Mechanical translation audit

**Objective:** determine whether every executable image can be translated.

- Build the current pinned DolRecomp on Apple Silicon.
- Translate `main.dol` with the portable backend.
- Translate all RELs in one batch for cross-module imports.
- Aggregate instruction counts, unknown instructions, fallback reasons and
  addresses, SMC candidates, unsupported relocations, and generated/object
  sizes per image.
- Fail on unresolved imports or unexplained relocation failures.

**Exit:** every discovered image generates and every non-AOT site is
enumerated. Stop if unpatchable hot-path fallback makes no-JIT execution
clearly uneconomic.

### A2 — Composite arm64 module

**Objective:** package the DOL and complete REL collection as one module or
attached descriptor.

- Generalize the RecompCore module template to a generated-image manifest.
- Namespace generated symbols across images.
- Emit one dispatcher plus DOL/REL chunk, SMC, module, and section metadata.
- Validate complete, non-overlapping executable-address coverage.
- Measure generated size, object size, link time, startup cost, and memory.

**Exit:** ABI validation and synthetic representative DOL/REL dispatch tests
pass.

### A3 — Active-REL lifecycle and mapping

**Objective:** enter AOT code correctly as Wind Waker links and unlinks RELs.

- Measure the existing RAM-scan discovery once with the full descriptor set.
- If it does not scale, add the smallest link/unlink-driven registration seam
  at `OSLink`/`OSUnlink`, the active-module list, or an explicit section API.
- Test unload/reload, guest-address reuse, multiple executable sections,
  constructors/destructors, unresolved handlers, and indirect calls.

**Exit:** deterministic synthetic lifecycle tests pass and mapping work scales
with link events or active modules rather than all descriptors times guest RAM.

### A4 — Authentic no-JIT macOS boot

**Objective:** exercise the same PowerPC execution contract required by iOS.

- Enforce `STATICRECOMP_NO_FALLBACK_JIT=1` or its current equivalent.
- Follow normal boot from the user's disc/extraction.
- Do not use Dolphin memory captures, seeded state, scene bypasses, or a
  synthetic stage.
- Record native dispatch, interpreter fallback, active REL mappings, SMC
  retirements, exceptions, missing services, emulated speed, and frame time.
- Progress through boot/logo, title, file select, new-game intro, Outset,
  controllable Link, one authentic transition, and save/quit/reload.

**Exit:** controllable Link in a real stage plus one transition and a valid
save reload, reached normally with runtime PowerPC JIT disabled. **This is the
first proceed-to-product gate.**

### A5 — Representative compatibility campaign

Exercise Outset/interiors, sailing, Forsaken Fortress, a dungeon, a boss,
inventory/map/message UI, saves across a story flag, cutscenes, and varied
audio. Remaining failures must be individually attributable and not indicate
a missing architectural subsystem.

### A6 — Physical iPad proof

Create a BlueWake target and selectively reuse reviewed SunPad Apple-host
components only after A4. Prove normal boot to the authenticated save,
controllable gameplay, a transition, lifecycle behavior, and save-hash
preservation through backgrounding and restart.

### A7 — Sustained original-speed gate

Target normal 30 FPS behavior. Measure FPS separately from emulated speed,
frame-time distribution, AOT/interpreter counts, subsystem CPU cost, memory
and jetsam margin, thermals, energy, and extended-session behavior across
field, sailing, dungeon, boss, and cutscene workloads.

### A8 — Complete-game acceptance

Require a clean new-game-to-ending run, all dungeons and bosses, required
cutscenes, representative optional content, save and retry flows,
suspend/resume, controller reconnect, long sessions, and regression replay.

## TRACK B — Source-native decompilation/Aurora plan (retained)

The original plan below remains useful if Track B is selected. Its phase
numbers are historical and independent of Track A.

Conventions: `tww` = ref/tww (zeldaret/tww @ 2289b54 or later), `dusk` =
ref/dusk (TwilitRealm @ 7434a0f8), `aurora` = ref/aurora (@ 1d10fa1).
"Portability pass" = the BE<T>/OffsetPtr/AVOID_UB/heap/ABI recipe documented
in docs/research/DUSK.md §Portability-pass-taxonomy.

---

## PHASE 0 — Reproducible environment & pinned deps (≤2 days)

**Objective:** deterministic dev environment on Apple Silicon macOS.
**Work:**
- New repo `bluewake` (this repo). Vendor as submodules: `aurora`,
  `borealis` (pin the SHAs dusk pins), SDL3 (aurora provider), and record
  the tww SHA being ported (subtree or fork — dusk chose hard fork; do the
  same so the portability pass has a home; keep `upstream` remote for
  subtree merges).
- Copy dusk's build skeleton: root CMakeLists structure, `cmake/`
  (GameABIConfig, AppleExports, SymbolManifest), `CMakePresets.json`
  (macos/ios presets), `ios.toolchain.cmake`, `.github/workflows/build.yml`
  — rename dusklight→bluewake, GZ2E01→GZLE01.
- Toolchain check script: Xcode ≥16, CMake ≥3.27, Ninja, Python ≥3.10,
  Rust (for nod, incl. `aarch64-apple-ios` target later).
**Failure modes:** none novel (all components built here already, E5/E6/E8).
**Exit gate:** `cmake --preset macos-default-relwithdebinfo && ninja` links
an empty `bluewake` executable against aurora::main on arm64 macOS.

## PHASE 1 — BOUNDED PROTOTYPE: authentic spine to controllable Link (≤6 weeks, hard stop)

**Objective:** falsify or confirm the last open feasibility question.
**Prerequisites:** Phase 0; a legally-dumped GZLE01 image (user-supplied).
**Scope discipline:** do NOT fix menus, NPCs, audio, Picto Box. Stub
aggressively; log every stub hit.

**Work, in order:**
1. **Source import (spine only):** from tww: `m_Do/`, `f_pc/`, `f_op/`,
   `f_ap/`, `SSystem/`, `c/`, `d/` core files (d_com_inf_game, d_stage,
   d_resorce, d_save, d_event*, d_kankyo core, d_s_logo/title/play/room,
   d_bg*, d_particle, d_drawlist...), `d/actor/` DOL-resident actors
   (player*, sea, bg, vrbox*, item...), JSystem (JKernel, JUtility,
   JSupport, JMath, JFramework, JStage, J2D, J3D, JParticle, JStudio,
   JMessage), `f_pc_profile_lst`. Defer: JAudio/JAZelAudio (stub JAI* API
   surface to silence), menu/map files (stub), the 102 stub-heavy RELs
   (register profile with a generic no-op actor), TRK/Odemu (drop).
2. **Header portability pass:** apply BE<T>/OffsetPtr to on-disc structs
   actually touched by the spine (d_stage.h, d_resorce, J3D loaders, dzb
   collision, dzr/dzs). Steal dusk's `include/helpers/*` verbatim. `-fsigned-char`,
   MULTI_CHAR, MWCC intrinsic shims from dusk's global.h adaptations.
3. **SDK layer:** link aurora::{core,gx,gd,os,pad,si,vi,mtx,card,dvd,main};
   port dusk's `src/dusk/OSThread.cpp`, `OSMutex.cpp`, `OSContext.cpp`,
   `stubs.cpp` (OSMessageQueue/alarm/VI-retrace/EXI/DC), strip TP tails.
   Wire `m_Do_main.cpp` main() to aurora_initialize + run loop.
4. **REL statics:** generate files.cmake REL list from tww configure.py
   (script it); adopt dusk's c_dylink TARGET_PC stubs + static
   `g_fpcPf_ProfileList_p`; keep RELS.arc mounting for dvd_asset reads.
5. **Data escape hatch:** port `dvd_asset.cpp` mechanism (DOL VA→file
   offset) for any data the spine needs that tww hasn't decompiled.
6. **Bring-up sequence (test gates within the phase):**
   a. process framework ticks with null scene →
   b. d_s_logo runs (deps: J2D, tex from disc) →
   c. d_s_title with 3D title scene →
   d. d_s_play loads sea stage: stage/room .arc via DVD+JKR, dzb collision,
      J3D rendering through aurora →
   e. Link spawns (d_a_player), PAD input moves him, camera runs →
   f. one authentic transition (room/stage change via original dStage code).
**Likely failure modes & diagnostics:**
- MWCC-ism / UB landmines (uninit reads, &ref==NULL, vtable layout):
  AddressSanitizer preset (dusk ships one); compare against Dolphin-run
  behavior for the same code path.
- Heap exhaustion from 64-bit growth: adopt dusk's heap multipliers early.
- Hidden JAudio init dependency in m_Do_audio/mDoAud_Create: stub at the
  mDoAud_* boundary (dusk did the analogous thing during bring-up).
- EFB-copy path differences (I8 grayscale intro): aurora covers formats
  (verified); if d_s_logo depends on GXPeekARGB unexpectedly, stub to
  constant.
- A required function turns out sourceless: count them; >~200 on critical
  path = WAIT signal (per FEASIBILITY_REPORT).
**Exit gate (= PROCEED/WAIT decision):** the milestone quoted in
FEASIBILITY_REPORT §Exact-first-implementation-milestone.

## PHASE 2 — Game-data pipeline hardening (1 wk)

Disc verification (xxh3 catalogue for GZLE01 rev0; later GZLJ01/GZLP01),
borealis file-picker/data-dir integration, RVZ support via nod, sha1 gate
against `config/GZLE01/build.sha1` hashes for extracted objects.
**Exit:** cold start from clean machine + disc → title, no manual steps.

## PHASE 3 — Full source import & stub inventory (2 wks)

Import ALL remaining tww source incl. NonMatching units; auto-generate a
stub registry (every `/* Nonmatching */` empty body → logged panic-or-noop
shim + telemetry). Build the whole-game binary (all 415 RELs statically).
**Exit:** binary links; stub-hit telemetry dashboard exists; sea + Outset +
Forsaken Fortress intro path playable with known stub hits enumerated.

## PHASE 4 — GameCube OS services completeness (1–2 wks)

Close every stubs.cpp TODO the game actually hits: OSAlarm-driven timing,
DVD error/cover callbacks, reset semantics, GBA (agb) stubs-to-benign,
memory-card flows via aurora CARD (.gci compatible with Dolphin saves —
test save roundtrip vs Dolphin).
**Exit:** save/load/quit/relaunch cycle identical to Dolphin reference.

## PHASE 5 — Dynamic-module semantics audit (1 wk)

WW loads/unloads RELs per-stage with mmem/amem pools. Statically linked,
lifetime bugs can hide (static ctors that assumed fresh .bss per load).
Audit `prof` init/dtor paths; replicate dusk's per-load reinit solutions;
fuzz stage-warp soak test.
**Exit:** 2-hour scripted warp soak, zero stale-state faults.

## PHASE 6 — JSystem deltas (1–2 wks)

TWW's JSystem is an earlier revision than TP's (plus JRenderer). Diff
against dusk's patched libs where filenames match; port fixes; write new
ones for JRenderer/J3D revision differences. Keep dusk's 21-static-libs
RESCAN linking layout.
**Exit:** no J3D/J2D-origin rendering faults on the Phase 3 playable path.

## PHASE 7 — Rendering completeness (2–3 wks)

GXPeekARGB implementation in aurora (clone depth_peek.cpp pattern) for
Picto Box; verify WW's I4/I8 EFB tricks (grayscale intro, heat haze,
d_drawlist mirrors) frame-by-frame vs Dolphin captures; widescreen +
resolution scale QA; ship initial_pipeline_cache.db equivalent.
**Exit:** visual parity checklist (20 canonical scenes) vs Dolphin.

## PHASE 8 — Audio (3–6 wks; biggest new engineering)

Bring up JAudio1 (JAI/JAS) + JAZelAudio from tww source over a DuskDsp-
derived host DSP model (ADPCM, mixer, resampler, reverb). Upstream has 166
stubs here — expect to write functionally-equivalent C (contribute
matches back upstream where possible). Streamed audio (sea ambience, AW/AF
files), sequenced music, SFX, Wind Waker baton mini-sequences.
**Exit:** title music, sea ambience, sword/UI SFX, baton conducting all
correct by ear vs reference; no DSP-sync deadlocks.

## PHASE 9 — Input (1 wk)

Aurora PAD bindings + rumble; analog trigger semantics (WW uses L-target,
R-crouch analog edge); GameController profiles; remap UI (dusk pattern).
**Exit:** full game controllable on Xbox/PS/GC-adapter pads on macOS.

## PHASE 10 — Menus/HUD/map completion (parallel, tracks upstream)

The d_menu_fmap/fmap2/dmap/item/collect + d_map + d_meter cluster (~460
stubs) — adopt upstream matches as they land; write equivalents where
schedule demands. dvd_asset for the map textures/tables meanwhile.
**Exit:** sea chart, item screens, dungeon maps functional.

## PHASE 11 — NPC/actor tail (largest bucket, tracks upstream)

3,459 stubs across 102 RELs, overwhelmingly NPCs/minigames. Strategy:
(a) consume upstream continuously (they're closing NPCs now); (b) triage
by quest-criticality (main-quest NPCs first: Tetra, King of Red Lions,
Aryll, town gates); (c) write equivalent C for stragglers.
**Exit:** main quest completable start-to-credits.

## PHASE 12 — macOS application polish (1–2 wks)

.app bundle, icns, entitlements (disable-library-validation only if mods),
ad-hoc + Developer-ID signing paths, sparkle-free updater via borealis,
crash reporting opt-in, save-folder UX.
**Exit:** notarizable-quality .app (notarization itself optional given
provenance posture).

## PHASE 13 — macOS validation (2 wks + ongoing)

Full-game playthrough checklist (main quest + Picto/Fairy/side systems),
Dolphin-diff triage for the ~3,450 unverified non-matching functions'
behavior, perf baseline (should idle far above 60; verify no aurora
worker stalls), memory profile.
**Exit:** 100% main-quest pass, zero P0s open.

## PHASE 14 — iOS-safe conversion (1 wk)

Flip to `ios-default`-style preset: OS64 toolchain, BUILD_SHARED_LIBS=OFF,
static Dawn/SDL3, Rust aarch64-apple-ios for nod, disable mods dlopen path
(compile-time), Info.plist (UIFileSharingEnabled, landscape-only,
UIDeviceFamily 1+2, CADisableMinimumFrameDurationOnPhone, GameMode),
LaunchScreen. Confirm zero W^X/JIT/dlopen at runtime (static audit +
Instruments).
**Exit:** BlueWake.app builds & signs for iOS in CI (mirror dusk's
build-apple job).

## PHASE 15 — Metal/iOS integration & memory (1–2 wks)

Device-class memory budget: shrink guest MEM1/heap multipliers from the
desktop 256 MB pattern to a measured budget; `Increased Memory Limit` +
`Extended Virtual Addressing` entitlements; pipeline-cache prewarm on
first launch (progress UI); thermal/ProMotion frame pacing.
**Exit:** 30-min gameplay on a 4 GB-class device without jetsam;
steady 60 (or paced 30) on target hardware.

## PHASE 16 — iPhone application (1 wk)

Safe-area layout, Files.app disc import flow, iCloud-backup exclusion for
disc images (large), save backup/export UI, onboarding (verify disc hash,
explain sideload resign).
**Exit:** clean first-run UX on iPhone.

## PHASE 17 — iPad application (0.5–1 wk)

iPad idiom assets, pointer/trackpad support (indirect input events),
Stage-Manager/external-display windowing, Smart-Connector keyboards.
**Exit:** iPad-native review checklist passes.

## PHASE 18 — Touch controls (1–2 wks)

Port dusk's RmlUi touch system + editor; design WW-specific layout (items
on 3 slots, sail/baton contexts, camera drag, gyro aim option via aurora
sensor API); PADSetVirtualStatus injection (already game-agnostic).
**Exit:** full game completable touch-only.

## PHASE 19 — Lifecycle/suspend/resume (1 wk)

SDL3 background events → pause game clock + audio; Metal surface
teardown/rebuild on background (Dawn handles via SDL, verify); safe
autosave-on-background (CARD flush); resume-state QA incl. audio session
interruption (calls).
**Exit:** 50 background/foreground cycles incl. mid-cutscene, no faults.

## PHASE 20 — Full-game validation on devices (2–3 wks)

Playthrough matrix: {macOS arm64, iPhone, iPad} × {pad, touch} main quest;
save interchange with Dolphin; soak tests; battery/thermal report.

## PHASE 21 — Performance (1–2 wks)

Tracy profiles (aurora built-in), draw-call batching via aurora display-
list optimizer, shader-cache coverage, load-time (RVZ decompress) tuning.
**Exit:** perf budget doc met on oldest supported devices.

## PHASE 22 — Distribution & provenance (1 wk)

Release pipeline mirroring Dusklight: source-first GitHub release, macOS
.app, iOS .ipa for sideload; no assets, no orig code, xxh3 disc gate;
LEGAL_AND_PROVENANCE.md shipped; trademark-clean naming/branding
("BlueWake", no Nintendo marks); SBOM of licenses (CC0/MIT/BSD/zlib).

## PHASE 23 — Release documentation

Install guides (macOS, AltStore/SideStore iOS), dumping guide links,
troubleshooting, contribution guide pointing decomp work upstream to
zeldaret/tww.

---

## Standing tracks (run alongside all phases)

- **Upstream sync:** weekly subtree merge from zeldaret/tww; convert stub
  registry entries to real source as matches land; PR functionally-
  equivalent reconstructions upstream as NonMatching candidates.
- **Reference diffing:** maintain a Dolphin-side trace harness (frame-state
  checksums at fixed points) to triage behavior differences.
- **Monthly go/no-go review** against decomp.dev velocity if Phase 1
  returned WAIT.
