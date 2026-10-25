# Independent Architecture and Finish-Line Review — 2026-09-01

**Author:** independent senior macOS / emulator-runtime / game-porting /
graphics / audio / C++ systems reviewer (out-of-loop), at the user's request.
**Audience:** the autonomous implementation loop (`docs/GOAL_LOOP.md`).
**Status:** advisory, evidence-backed. Read at Step 0 alongside `PRD.md`,
`CURRENT.md`, `TECH_DEBT.md`, `REORIENTATION_2026-09-01.md`, and the two prior
independent reviews.
**Reviewed state:** `main` = `origin/main` = `63a15cf` ("Own Route B logo host
policy", 2026-09-01 18:45 -0500). The tree was being edited by the primary
agent while this review ran: `6279140` was HEAD at start, `63a15cf` landed
mid-review, and an `audio_init_data` tier (`route_b/src/audio_init_data.cpp`,
`tests/route_b_audio_init_data_test.cpp`, `config/route_b_audio_startup_probe.json`)
appeared uncommitted at the end and was committed as `a45013e` ("Validate
Route B audio startup resources") as this report was being written. Line
numbers cite `63a15cf` unless stated. Static review
only; no BlueWake process, composite, or Simulator was launched. The two
user-edited files under `ref/recompcore` were not touched.

Every material statement is labeled **OBSERVED** (read directly in a file, log,
or command output), **INFERRED** (a conclusion from observed facts), or
**SPECULATIVE** (a judgment without decisive evidence).

---

## 1. Executive verdict

**Route B is the right primary route, and the project is currently building
the wrong first layer of it.** OBSERVED/INFERRED.

- Route A is a complete, correct-execution oracle with no bounded measured path
  to product speed. Every architecture-scale lever has been tried under the
  project's bit-exact-to-own-baseline acceptance contract and either failed
  correctness (full edge loop: turns −64.5%, user CPU −60.2%, DSP delivery
  0 of 10,281) or produced sub-15% gains. OBSERVED (`docs/archive/status/PERFORMANCE.md:78-96`,
  `:236-253`, `:3-25`). Route A should receive no further speed work now. It
  should remain the behavioral oracle and become an oracle *generator* for
  Route B (§4).
- Route B has the only native speed ceiling and a proven recipe (Dusklight),
  but the loop has spent today's thirteen Route B tiers building
  BlueWake-owned *re-implementations* of retail subsystems that already exist
  as complete source in the pinned tree: a RARC validator that cannot serve
  `dComIfG_getObjectRes` (`route_b/include/bluewake/route_b/object_resources.hpp:22-42`),
  a 58-line progressive-scan "policy" that Dusklight ships as four stub lines
  (`route_b/src/logo_host_policy.cpp`; `ref/dusk/src/dusk/stubs.cpp:222-228,358`),
  a rewritten `phase_0` (`route_b/src/d_s_logo_native.cpp:36-63`) instead of the
  original 40-line function with `TARGET_PC` guards, and now an uncommitted
  parser for `JaiInit.aaf` (`route_b/src/audio_init_data.cpp`, 116 lines) beside
  the complete, marker-free retail `JAIInitData.cpp` (121 lines, 0 nonmatching).
  OBSERVED. These pass sanitizer tests and read as progress in the ledgers, but
  none of them is on the dependency path to a native frame, and at least two
  must be discarded when the original code is admitted.
- The actual dependency path from today to the first native logo frame is
  known, bounded, and donor-proven: port JKernel heaps and archives, a
  BlueWake-owned OS thread/message-queue/alarm service (Aurora has none), the
  JUT texture / J2D picture / dDlst 2D path, and a host frame driver over
  Aurora GX. Dusklight did exactly this with 60 `TARGET_PC` sites across 25
  JKernel files, 5 in J2D, and 17 in its logo scene. OBSERVED (§7). The logo
  is a single textured GX quad from a BTI in `Logo.arc`; it needs no `.blo`,
  no BMD, no JStudio, and no audible audio. OBSERVED (§7.1).
- Audio is less blocking than the ledger says and more blocking than the loop
  hopes. The JAudio1 *initialization* graph is essentially complete source;
  phase zero's only hard audio precondition is that wave-scene group 2
  finishes a DVD-to-ARAM load. Aurora supplies ARAM, and the in-tree Dolphin
  Zelda-ucode HLE speaks the exact `0xF355` mailbox protocol TWW's audio
  thread validates. But the per-frame driver, sound start, and BGM functions
  are among 140 to 152 empty bodies, so nothing will be audible until those
  are reconstructed. OBSERVED (§6).
- Process and evidence hygiene regressed in ways that matter: the repository's
  own P0 audit has failed since 2026-08-31 15:08 because 18 files were
  force-added under the ignored `local-research/`, including four copies of
  one memory-card image and eight execution traces, all pushed to
  `origin/main`. CI runs only that failing audit and builds nothing.
  OBSERVED (§11).

The 25–30% overall estimate is defensible only if "Route A oracle, disc
pipeline, GX validation knowledge, and tooling" are counted as product
progress; a stricter reading is 20–30%. The 5–10% Route B figure is generous
by lines of retail source admitted (about 60 of roughly 1,400 configured
objects, 943 BlueWake lines against a Dusklight-scale pass of about 62,000
changed lines); 3–7% is the honest range, rising to 10–15% at the first
native frame. INFERRED (§12).

**Next-smallest action to begin immediately:** compile the original TWW
JKernel heap and archive units (`JKRHeap`, `JKRExpHeap`, `JKRSolidHeap`,
`JKRArchivePub`, `JKRMemArchive`, `JKRDecomp`, `JKRFileFinder`) plus
`JUTTexture` over Aurora's MEM1 arena, and make the original
`dRes_control_c::setRes / syncAllRes / getRes` (`ref/tww/src/d/d_resorce.cpp:378-466,640-653`)
return the toon `ResTIMG*` from the private `System.arc` read through the
already-qualified `AuroraDisc` reader. Acceptance: original `phase_1`
(`ref/tww/src/d/d_s_logo.cpp:684-724`) executes unmodified and
`dDlst_list_c::setToonImage` receives a pointer whose big-endian header
decodes to the retail toon dimensions. This retires `ObjectResources`.

**Tempting action that must not be attempted next:** the ledger's own "next
smallest action" — composing `AuroraDisc`, `ObjectResources`, and
`LogoHostPolicy` into a "default macOS Route B application service"
(`docs/archive/status/CURRENT.md:74-79`, `BLOCKERS.md:103-109`). It would wrap a
production-looking object around a resource owner that cannot serve
`getObjectRes`, a policy that models a prompt no Mac will ever show, and a
rewritten phase zero, then have to be unwound at phase one. Also not next:
reconstructing the 133 audio functions, reopening any Route A speed shape, or
any further horizontal `f_op`/wrapper leaf.

---

## 2. Verified current state

### 2.1 Repository and backup

- HEAD `63a15cf` equals `origin/main`; `git log origin/main..main` is empty;
  no stash. OBSERVED. `main` is backed up. 35 local `codex/*` branches exist
  against 5 remote refs; those branches are not backed up. OBSERVED.
- Working tree at end of review: `route_b/CMakeLists.txt` modified, five
  untracked audio-init files. OBSERVED. The tree is live; the primary agent
  is mid-tier.
- Commit cadence: 40 commits on 2026-09-01, 36 on 08-31, 28 on 08-30, 105 on
  08-25. Thirteen Route B tiers landed between 16:18 and 18:45 today, one
  every 11 minutes on average. OBSERVED (`git log --since`).
- `scripts/audit_repo.sh` prints `AUDIT FAIL` at HEAD (18 tracked files under
  `local-research/`). OBSERVED. `.github/workflows/audit.yml` is the only
  workflow and runs only that script plus a JSON check; it compiles nothing
  and runs no test. OBSERVED. CI on `main` has therefore been red since
  `370d7f9` (2026-08-31 15:08). INFERRED.
- `/tmp` still holds 647–649 `bluewake-*` entries. OBSERVED. Every
  `local-research/evidence/` directory cited in the first 400 lines of
  `CURRENT.md` and in the 08-31/09-01 `PERFORMANCE.md` entries exists on disk
  (530 evidence directories total). OBSERVED. The only cited `/tmp` path
  (`ROUTE_B_VERTICAL_BOOT_2026-09-01.md:102`) is a reproduction-command output
  whose durable copy exists. OBSERVED. Evidence promotion has genuinely
  improved since the 08-30 review's 8,545-file finding.

### 2.2 Route A (accepted oracle)

- Current promoted baseline: Outset 14,100-retrace route, 343.88 s wall /
  346.16 s user for about 235 s of guest time, roughly 68% real speed before
  pacing. OBSERVED (`PERFORMANCE.md:97-111`; `FINISH_LINE.md:11-13`).
  `TECH_DEBT.md:15-16` still quotes a stale 470.60 s figure. OBSERVED.
- Only FPS figures on record predate the current baseline: opening about 29,
  Outset 6.0 median / 2.5 tail, Omasao 13.55 median. OBSERVED
  (`FINISH_LINE.md:73-76`).
- Composite: 748 chunks, `-O2` with three `-O1` fallbacks, 15,378.82 s
  (4.27 h) wall to rebuild, 3.42 GB peak RSS. OBSERVED (`PERFORMANCE.md:1723-1726`).
  `TECH_DEBT.md:252,258` still cite the superseded `-O1` size and "about 101
  minutes". OBSERVED.
- No frame limiter or pacing logic exists in `runtime/host/src`; the sole
  pacing token is `.vsync = true` at `runtime/host/src/main.c:3124`. OBSERVED.
- The Zelda DSP HLE remains excluded from the donor archive
  (`runtime/host/CMakeLists.txt:395-398` globs only `DSP/` interpreter
  objects); the DSP adapter option defaults `OFF` (`:8-9`). OBSERVED.
- `main.c` is 10,936 lines with 37 distinct `BLUEWAKE_*` `getenv` names, 20 of
  them scripted-PAD controls. OBSERVED.
- No human has played BlueWake unscripted. `TECH_DEBT.md:425-434` (TD-002)
  records it as pending; `GATES.md:9` says P4+ "boots through
  title/file/new-game into Outset" without stating the scripted-input
  qualifier. OBSERVED.
- Link hair: the single-texmap TEV-slot binding defect is fixed at the GX
  contract with a regression (`BLOCKERS.md:475-480`, `CURRENT.md:484-489`), but
  three capture predicates failed to produce any presented frame containing
  Link (`CURRENT.md:504-524`). OBSERVED. The fix is unverified visually.

### 2.3 Route B (selected route)

What compiles and links, all under Apple Clang C++20 with
`-Werror=pointer-to-int-cast -Werror=int-to-pointer-cast -Werror=shorten-64-to-32`
(`route_b/CMakeLists.txt:33-39`). OBSERVED:

| Layer | Original TWW units admitted | BlueWake-owned replacement |
|---|---|---|
| SComponent / SStandard | 11 (`CMakeLists.txt:43-53`) | — |
| `f_pc` | 30 of 31 (glob minus manager, `:61-66`) | `f_pc_manager_adapter.cpp` (115 lines) + `process_frame.cpp` + `ProcessManagerPlatform` callbacks |
| `f_op` | 15 (`:137-151`) | `f_op_camera_manager_adapter.cpp` |
| Application root | `c_counter`, `f_ap_game`, `m_Do_hostIO` (`:155-157`) | — |
| Logo scene | `d_com_lib_game.cpp` only (`:162`) | `d_s_logo_native.cpp` (111 lines) rewrites `phase_0`, asserts at `phase_1` |
| Resources | none | `ObjectResources` / `RarcArchive` (273 lines) |
| Disc | none | `AuroraDisc` over pinned Aurora `nod/dvd/fst` (75 lines; optional tier) |
| Video/reset | none | `LogoHostPolicy` (58 lines) |
| Audio | none | smoke callbacks; uncommitted `AudioInitData` (116 lines) |

- Seven tracked TWW patches total 382 lines (`patches/tww/`, replayed by
  `scripts/prepare_route_b.sh`). OBSERVED. `fpcPf_Get` is redirected to the
  static registry (`patches/tww/0006-…:` `f_pc_profile.cpp` hunk). OBSERVED.
- Six CTests pass in Debug, Release, and ASan/UBSan (`build/route-b-foundation*/Testing/Temporary/LastTest.log`,
  18:36 today). OBSERVED. A seventh (`audio_init_data`) is being added
  uncommitted. OBSERVED.
- Route B code volume: 943 lines under `route_b/src`, 2,261 including
  headers and tests. OBSERVED. For scale, the retail units on the first-frame
  path alone (`d_drawlist.cpp` 2,108; `d_com_inf_game.cpp` 1,822;
  `d_resorce.cpp` 721; `m_Do_graphic.cpp` 1,959; `m_Do_ext.cpp` 3,282;
  `m_Do_machine.cpp` 571; `m_Do_dvd_thread.cpp` 336; JKernel 7,274 across 25
  files; `JFWDisplay.cpp` 552; `J2DPicture.cpp` 603) total about 19,000
  lines. OBSERVED (`wc -l`).
- Aurora private-disc tier reads `System.arc` (34,176 bytes, 7 entries) and
  `Logo.arc` (170,304 bytes, 12 entries) through `DVDOpen`/`DVDReadPrio`
  (`route_b/src/aurora_disc.cpp:39-64`). OBSERVED. This is the one Route B
  service that is already the production shape.

### 2.4 Ledgers

- `CURRENT.md`: 6,336 lines, 133 stacked `> **` banners, 63 occurrences of
  "next smallest action", no top-of-file authority rule. `BLOCKERS.md`: 4,998
  lines, 66 such occurrences, with an authority rule at `:3-7`. OBSERVED.
- `CURRENT.md:26` says 25–30% / 5–10%; `CURRENT.md:141` still says "from
  35–45% to 25–35%". `docs/archive/PORTING_HISTORY.md:318` asserts in the present
  tense that `TECH_DEBT.md` carries the 35–45% estimate. OBSERVED. Stale.
- `docs/archive/WAITING_FOR.md` has no supersession banner and lists as active a
  promotion gate that `FINISH_LINE.md:15-18` says was cleared. `docs/archive/HANDOFF.md:12-16`
  shows a "CURRENT STATUS (2026-08-13)" heading under a banner declaring it
  superseded. OBSERVED.
- No future-dated entries found. OBSERVED.

---

## 3. Findings ordered by severity and leverage

### F1 — Route B is re-implementing complete retail subsystems as BlueWake-owned services instead of porting them (highest leverage)

- OBSERVED: `ObjectResources::find()` returns a `RarcArchive*` exposing only
  `valid()`, `file_count()`, `size()`, `error()`; the bytes are a private
  `std::vector` with no accessor (`object_resources.hpp:22-42,52`). The retail
  consumer needs `dComIfG_getObjectRes("Logo", 3)` to return a pointer to the
  big-endian BTI bytes inside the mounted archive (`d_resorce.cpp:144-374`,
  `:369`; `JUTTexture.cpp:29-33`). The service cannot serve phase one.
- OBSERVED: `d_s_logo_native.cpp:36-63` re-expresses `phase_0` through an
  11-callback `LogoPlatform` struct (`logo_platform.hpp:12-25`) and replaces
  the two retail calls `mDoAud_zelAudio_c::isInitFlag()` /
  `mDoAud_checkFirstWaves()` with `audio_initialized` /
  `first_waves_pending`. The original is 40 lines
  (`ref/tww/src/d/d_s_logo.cpp:633-681`) and Dusklight keeps its equivalent
  original with 17 `TARGET_PC` guards (`ref/dusk/src/d/d_s_logo.cpp`).
- OBSERVED: `LogoHostPolicy` (58 lines plus a 69-line test) models cold vs
  restart, DTV capability, progressive preference, persistence, and held-B.
  Dusklight: `OSGetProgressiveMode` returns 0, `OSGetResetCode` returns 0,
  `OSSetProgressiveMode` is empty, `VIGetDTVStatus` returns 0
  (`ref/dusk/src/dusk/stubs.cpp:222-228,358`). On a Mac the retail prompt is
  unreachable and the restart code is a constant.
- OBSERVED (uncommitted): `AudioInitData` parses `JaiInit.aaf` natively while
  `ref/tww/src/JSystem/JAudio/JAIInitData.cpp` (121 lines, 0 nonmatching
  markers) already does so and is called directly by the complete
  `JAIBasic::initInterfaceMain` graph (`JAIBasic.cpp:65-89`).
- INFERRED: each of these tiers passes its own smoke, records "bounded", and
  raises no ledger flag, yet each is a horizontal callback-struct-plus-smoke
  shape — the exact shape `REORIENTATION_2026-09-01.md:36-40` and
  `TECH_DEBT.md:98-103` closed for `f_op` leaves. The reorientation stopped
  the shape at the framework layer and it resumed at the scene layer under a
  "vertical" label. Under `GOAL_LOOP.md §6` this is the third-plus repetition
  and should trigger the anti-stall response.
- INFERRED: the correct pattern, proven by Dusklight in 125 JSystem files
  with `TARGET_PC` guards, is *compile the original unit, guard the hardware
  lines, supply the SDK service*. BlueWake-owned code belongs at the SDK
  seam (OS threads, VI, AI, DSP, host window), not above it.

### F2 — The first native frame's dependency path is bounded and known; it is not what the loop is building

- OBSERVED (§7.1): the Nintendo logo is one `GX_QUADS` draw of a BTI texture
  through `dDlst_2D_c` → `J2DPicture::draw` → `JUTTexture`
  (`d_drawlist.cpp:655-666`; `J2DPicture.cpp:335-395`; `JUTTexture.cpp:24-122`).
  No `.blo`, no BMD, no JStudio, no audible audio.
- OBSERVED: phase one is tiny (`d_s_logo.cpp:684-724`: sync, two
  `getObjectRes`, two static stores, one heap alloc). Phase two's first
  eleven statements create the seven `dDlst_2D_c` pictures
  (`:740-859`); everything after (`:865-956`: 24 ARAM mounts, 4 RAM loads,
  `Always`/`Link`/`Agb` requests, `mDoAud_loadStaticWaves`, fade, reset
  callback) is boot sequencing whose completion is not consumed until
  `dvdWaitDraw` (`:418-466`).
- OBSERVED: the draw is reached only after `phase_2` returns
  `cPhs_COMPLEATE_e` (`:958`), so the first frame requires those phase-two
  *requests* to be issued authentically (threads, ARAM heap, DVD commands)
  but not to complete.
- INFERRED: minimum admission set for one frame: JKernel heaps/archives/
  decomp/file-finder; `JUTTexture`, `JUTFader`, `JUTVideo`/`JUTXfb`
  (or host substitutes); `J2DPane`/`J2DPicture`/`J2DOrthoGraph`/
  `J2DGrafContext`; `dDlst_2D_c` and the 2D-opa half of `dDlst_list_c`
  (its `init()` allocates 15 `J3DDrawBuffer`s at `d_drawlist.cpp:1888-1902`,
  which must be guarded or J3DPacket is dragged in); `mDoExt` heap wrappers;
  a host frame driver in the shape of `mDoGph_Painter`
  (`m_Do_graphic.cpp:1585-1940`) that skips the `getWindowNum()!=0` 3D block
  (`:1611`) and the particle draw calls (`:1914-1916`) and calls
  `dComIfGd_draw2DOpa()` (`:1933`); and a BlueWake-owned OS thread /
  message-queue / alarm service (F4). Roughly 19,000 retail lines with a
  Dusklight-like adaptation density of 2–5% of lines. SPECULATIVE on the
  density; OBSERVED on the unit list.

### F3 — Route A has no bounded speed path; its one architecture-scale lever is closed by the timing contract, not by CPU

- OBSERVED: full generated-edge loop: 62,038,491 → 22,048,210 turns
  (−64.5%), 26.61 → 10.60 user seconds, but zero DSP deliveries versus 10,281
  and no title-ready milestone (`PERFORMANCE.md:78-96`; `DECISIONS.md:19`).
  Selective semantic callback: turns −64.0%, first interrupt delivery moved
  117 cycles (`TECH_DEBT.md:205-210`). Generated-side intercept filter: 23.18
  vs 23.02 user s, no gain (`TECH_DEBT.md:247-250`). Chunk-table publication:
  turns −11.2%, pathological `-O1` compiles (`PERFORMANCE.md:236-253`).
  Partitioned `-O2`: −34.6% user CPU but diverges at delivery ordinal 244
  (`PERFORMANCE.md:1696-1720`). LLVM v27 on current host: 25.00 user s, no
  1.5× (`PERFORMANCE.md:3-15`). DSP idle skip: exact idle count zero on the
  reachable routes, removed (`:254-274`).
- INFERRED: the only way the −60% user CPU result becomes shippable is to
  relax bit-exact equivalence with BlueWake's own 256-cycle baseline in favor
  of milestone/behavioral oracles, and to compile per-address HLE intercepts
  into the generator. That is a policy reopening, not a shape repeat. Even
  then, correctness is unknown and the 4.27 h rebuild makes each attempt
  expensive.
- INFERRED: untried Route A items sum to well under 2×: GX frontend
  flush-per-store repair (never tried; est. 5–10% Outset), Zelda DSP HLE
  (policy never decided; 12–30% by scene), frame limiter (required; no
  speed). Route A cannot reach the 30 FPS / 99.5% speed gate by bounded work.

### F4 — Aurora does not provide OS threads, message queues, or alarms, and TWW's boot needs all three before the first frame

- OBSERVED: `ref/aurora/lib/dolphin/os/` contains only `OSAddress, OSAlloc,
  OSArena, OSBootInfo, OSInit, OSMemory, OSReport, OSTime`; no
  `OSCreateThread`, `OSInitMessageQueue`, or `OSSetAlarm` implementation
  exists anywhere under `ref/aurora/lib`. `ref/aurora/lib/dolphin/vi/vi.cpp`
  is 55 lines with no `VIWaitForRetrace`, `VIGetRetraceCount`, `VIGetDTVStatus`
  or XFB.
- OBSERVED: TWW's boot path creates the DVD thread
  (`m_Do_dvd_thread.cpp:41`, message queue at `:73`), the audio thread
  (`JASAudioThread.cpp:139`, queue at `:26,45,106`), and `JKRAram` threads;
  `JFWDisplay` polls `VIGetRetraceCount` (`JFWDisplay.cpp:333,358`).
- OBSERVED: Dusklight owns these: `ref/dusk/src/dusk/OSThread.cpp` (705
  lines), `OSMutex.cpp` (215), `OSContext.cpp` (72), plus message-queue and
  alarm implementations in `stubs.cpp:99-133,188,231` and retrace callbacks at
  `stubs.cpp:344-356`. Dusklight's repository license is CC0
  (`ref/dusk/LICENSE.md`); per-file provenance review is still required by
  PRD §10.
- INFERRED: this is a BlueWake-owned host service that must exist before
  `mDoDvdThd_mountXArchive_c::create` or `mDoAud_Create` can run. It is not
  in any ledger's next action.

### F5 — Audio: initialization is nearly complete source; playback is not

- OBSERVED: `mDoAud_Create` (`m_Do_audio.cpp:135-189`) → `JAIZelBasic::init`
  (`JAIZelBasic.cpp:840-878`) → `JAIBasic::initDriver/initInterface/…`
  (`JAIBasic.cpp:55-186`) → `TAudioThread::start` (`JASAudioThread.cpp:229`)
  → `BankWave::setWaveScene` (`JAIBankWave.cpp:67-77`) are real source.
  Phase zero's gate `isInitFlag() && checkFirstWaves()` (`d_s_logo.cpp:650`)
  resolves to `2 - wsLoadStatus[2]` (`JAIZelBasic.cpp:1506-1508`;
  `JAIBankWave.cpp:122-124`), which is 0 only after the DVD→ARAM load of
  wave-scene group 2 completes via `finishSceneSet` (`JAIBankWave.cpp:99-101`).
- OBSERVED: empty on the path to audible title audio:
  `JAIBasic::processFrameWork` (`JAIBasic.cpp:208`),
  `JAIZelBasic::zeldaGFrameWork` (`JAIZelBasic.cpp:153`),
  `startSoundVec/startSoundDirectID/startSoundBasic/makeSound`
  (`JAIBasic.cpp:213-322`), `bgmStart` (`:312`), `sceneBgmStart` (`:1465`),
  `load1stDynamicWave/load2ndDynamicWave` (`:1472,1483`),
  `SequenceMgr::checkEntriedSeq` (`JAISequenceMgr.cpp:121`). Census at the pin:
  140 empty bodies by the ledger's own heuristic (reproduced), 133 at upstream
  `94a5ead`, and 152 of 964 definitions by a stricter parse that counts
  marker-plus-orphaned-`OSReport` bodies. OBSERVED.
- OBSERVED: the retail gate tolerates uninitialized audio: if `isInitFlag()`
  is false, phase zero proceeds without waiting. INFERRED: the honest
  fail-closed contract is therefore "real `mDoAud_Create` runs each frame from
  the main loop as retail does; if any real precondition (AAF load, archive
  mount, heap, thread) is unavailable the flag stays false and the logo
  proceeds silently, with a logged ID". A callback returning "initialized"
  is prohibited; a callback returning "not initialized" is *also* fabricated
  unless it is the real code failing.
- OBSERVED: the DSP boundary is narrow: `dspproc.c:37,61,74`,
  `dsptask.c:496-516`, `osdsp_task.c:16`, `JASAiCtrl.cpp:35-93`,
  `JASSystemHeap.cpp:45-53`. Aurora implements ARAM (`AR.cpp:55-114`) but has
  no `ai.cpp` and no DSP. The in-tree Dolphin Zelda HLE lists Wind Waker's
  ucode CRC `0x86840740` with no quirk flags
  (`ref/recompcore/Source/Core/Core/HW/DSPHLE/UCodes/ZeldaUCodesTable.cpp:29-30`)
  and emits the `0xF355` handshake prefix (`Zelda.cpp:54`) that
  `JASDSPInterface.cpp:153` / `JASAudioThread.cpp:125` validate. Dusklight
  instead wrote a native mixer (`ref/dusk/src/dusk/audio/DuskDsp.cpp`, 779
  lines) driven by the game's channel blocks.

### F6 — Serialized-layout hazards are systematic, not per-model, and Dusklight's helpers are the template

- OBSERVED: `JSUConvertOffsetToPtr` is `(T*)((s32)ptr + offset)`
  (`ref/tww/include/JSystem/JSupport/JSupport.h:9`); `J3DModelLoader.cpp:406-574`
  stores `(u32)&… >> 4` into `mDiffFlag`; `J3DTexture.h:50-54` patches
  serialized offsets with truncated pointer differences; `JUTTexture.cpp:29-33`
  uses `(int)mTexInfo`; `JKRArchive.h:60-68` declares an on-disc
  `SDIFileEntry` with a `void* data` member that changes stride on LP64 and is
  written live at `JKRMemArchive.cpp:152`.
- OBSERVED: Dusklight resolves these with in-place `BE<T>` field types
  (`ref/dusk/include/helpers/endian.h:31-70`; `ResTIMG` declared field-by-field
  as `BE(u16)…` in its `JUTTexture.h:20-43`), `intptr_t` casts
  (`ref/dusk/libs/JSystem/src/JUtility/JUTTexture.cpp:23-27`), and an
  `OffsetPtr { BE<s32> }` with a relocated-bit guard
  (`ref/dusk/include/helpers/offset_ptr.h:8-60`). Only one J3D loader file
  needed endian edits (`J3DModelLoader.cpp`).
- OBSERVED: BlueWake already has `BigEndian<T>` and `RelativeOffset32<T>`
  (`route_b/include/bluewake/route_b/endian.hpp`, `offset_ptr.hpp`) but has
  applied them to no serialized resource struct yet. INFERRED: the
  `SDIFileEntry` stride hazard will bite the very first archive port and is
  the first place to apply them.

### F7 — Static REL strategy is sufficient through title and Outset, with one initializer hazard

- OBSERVED: `cDyl_Link/LinkASync/IsLinked/Unlink` all return success when
  `DMC[i] == NULL` (`ref/tww/src/c/c_dylink.cpp:518-597`); Dusklight exploits
  exactly that (`ref/dusk/src/c/c_dylink.cpp:22-23,901-902,921-922,970-973`) and
  keeps `DynamicModuleControl` behind `#if !TARGET_PC`. `f_pc_profile_lst` is
  a pure data REL (`f_pc_profile_lst.cpp:5-30`); `d_a_player` is in the DOL.
  GZLE01 has 416 REL modules and 430 name-table entries.
- INFERRED: the registry satisfies boot-to-Outset. Hazard: statically linked
  actor units run C++ static initializers at process start, before JKR heaps
  exist; any REL-level static that allocates or registers must be deferred
  to link time. SPECULATIVE on how many such statics exist; a grep census is
  cheap. Outset's first scene (`sea_T`, room 44) needs on the order of 20–40
  actor RELs. SPECULATIVE (not enumerated; needs `ActorDat.bin` and the DZR).

### F8 — Evidence policy is broken at the P0 level and published

- OBSERVED: 18 files force-added past `.gitignore:3` in five commits on
  2026-08-31 (`370d7f9`, `dc9e5eb`, `766d860`, `7b69573`, `0c9e600`), all on
  `origin/main`. Four `.card` files (98,412 bytes each, one SHA-256
  `6b43aabd…`) are "independent copies of the canonical seed card" per the
  directory README; eight `.log`/`.external-history.txt` files are execution
  traces. `docs/archive/PRD.md:398` requires saves and traces from gameplay to be
  private and ignored. `GATES.md:5` already records P0 as `REGRESSED`.
- INFERRED: whether a seed card counts as "derived game content" is a
  provenance question the user must answer, but the repository's own rule
  says it does, and history rewriting to remove it is a user decision
  (GOAL_LOOP §5 Step 11 forbids force-push without authority).

### F9 — The ledgers are honest in content and failing by volume

- OBSERVED: 133 banners and 63 "next smallest action" directives in one
  file, no authority rule. A resuming agent reading linearly sees 62 stale
  directives formatted identically to the live one. `WAITING_FOR.md` and
  `HANDOFF.md` carry stale "active"/"CURRENT" headings.
- OBSERVED: progress claims are carefully qualified in-line ("this does not
  present a native logo frame"), so the ledgers do not overstate in
  sentences; they overstate in *structure*, by recording each scaffolding
  tier as a "result" and a "bounded" answer.

### F10 — Route A human and visual acceptance is still zero and is cheap

- OBSERVED: live keyboard path exists and is overwritten by waypoints
  (08-30 review F7; `TECH_DEBT.md:426-429`). The hair fix has never been
  seen. A one-hour unscripted session costs nothing the loop is protecting
  and would give Route B its first product-level oracles (pacing feel,
  audio quality of the donor DSP path, camera). INFERRED.

---

## 4. Route A assessment

**Verdict: oracle only; no further speed work; promote it to an oracle
generator.** INFERRED from OBSERVED §2.2 and F3.

- Hypothesis 1 (no measured local optimization closes ~2× plus pacing
  headroom): **VERIFIED.** The largest remaining untried items are the GX
  frontend flush shape and the DSP HLE policy; their optimistic sum is well
  under the required factor. OBSERVED/INFERRED.
- Is there a realistic repair without reopening closed shapes? **Only by
  changing the acceptance contract**, which is a decision, not a shape. The
  full edge loop's −60% user CPU is the single largest measured lever in the
  repository. It failed because host per-address HLE interception and
  interrupt quantization live at turn boundaries. The generator-side
  intercept filter that should have preserved correctness without the
  callback cost showed no gain (23.18 vs 23.02 s), which is itself
  unexplained: an inline flag test should not cost the same as an indirect
  callback plus address classification. SPECULATIVE: that measurement may
  be confounded (per-edge state spill or cycle-domain flush inside the
  "filter"). One decomposition of *why* the inline filter cost the same
  would tell the project whether the −60% lever is recoverable. This is
  recorded as the named Route A fallback (§9 action 8), not as current work.
- What should Route A do now? (1) Nothing to its runtime. (2) Serve as a
  trace oracle for Route B: retrace-indexed milestones, DVD request order,
  DSP mailbox sequences, and per-function entry traces of the 133 empty
  JAudio1 bodies (retail code executes in Route A, so its call sequence,
  arguments, and memory effects can be logged as a reconstruction oracle —
  a diagnostic hybrid explicitly allowed by `PRD.md §9.4`). (3) One
  unscripted human session (F10).
- Measured condition for retiring Route A as oracle: when Route B reaches
  controllable Outset with a behavioral digest matching Route A's
  milestones. Until then keep the signed app buildable; do not let the
  composite tree rot (TD-010's provenance reconciliation stays open).

---

## 5. Route B assessment

### 5.1 Is Route B the best primary route?

**Yes, conditionally.** INFERRED.

- It is the only route that removes translated dispatch by construction,
  has a native performance ceiling, is maintainable, and has a shipped
  precedent (Dusklight over Aurora, CC0 game source, MIT Aurora). OBSERVED.
- Its cost is a porting program in three layers: (a) JSystem/machine-layer
  portability — bounded and donor-proven (Dusklight: 60 `TARGET_PC` sites
  in JKernel, 5 in J2D, 28 in J3DGraphBase, 14 in J3DGraphLoader, 51 in
  `m_Do_graphic.cpp`, 18 in `m_Do_ext.cpp`); (b) JAudio1/JAZelAudio
  control-layer reconstruction — 133 to 152 functions, external upstream
  progress of about 7 functions in two weeks; (c) content-layer empty bodies
  — 4,175 across the whole tree, mostly NPCs/menus, which gate the *campaign*,
  not the boot. OBSERVED.
- Hypothesis 2 (substantial native porting program, not a backend swap):
  **VERIFIED.** Hypotheses 3 and 4 (what Route B proves / does not prove):
  **VERIFIED as stated**, with the correction that "native bounded Yaz0/RARC
  object ownership" is not on the path to phase one because it cannot
  return resource pointers (F1).
- Hypothesis 5 (immediate work should be vertical): **VERIFIED in intent,
  overturned in content.** The listed first step ("default macOS service
  composition") composes scaffolding. The vertical path is F2/F4.
- Hypothesis 6 (more horizontal adapters or synthetic smokes are activity):
  **VERIFIED**, and it applies to today's Route B tiers as much as to the
  `f_op` leaves.

### 5.2 Credible hybrid architectures

- **Rejected correctly:** fine-grained calls between native objects and
  Route A guest memory (`PRD.md §9.5`). Nothing in the evidence suggests
  reopening it.
- **Overlooked and recommended:** (1) Dolphin Zelda-ucode HLE beneath native
  JAudio1 at the `dspproc.c` seam (F5), which avoids writing a mixer and
  preserves the retail channel-block protocol; the LLE interpreter is a
  second option but needs user-supplied DSP ROM/coef dumps
  (`runtime/host/src/dsp_adapter.cpp:243-246`). (2) Route A as a trace and
  oracle generator for reconstruction and for milestone digests (§4).
  (3) Route A's GX trace/replay tooling (`dolgx_replay`, `.dolt` captures) as
  a structural oracle for Route B's Aurora frames — same draw count, texture
  identity, TEV state — while accepting that pixels differ between GXCore and
  Aurora. INFERRED.
- **Not recommended:** porting Route A's GXRuntime/GXCore into Route B.
  Aurora GX is complete for the J2D/J3D surface (display lists, immediate
  mode as functions, indexed arrays, GD) and is what Dusklight ships on.
  OBSERVED (`ref/aurora/lib/gx/*`, `lib/dolphin/gx/GX*.cpp`).

### 5.3 Boot spine: ownership from phase zero to the first frame

Dependency order (each item needs the previous). INFERRED from OBSERVED unit
reads:

1. **Host SDK services (BlueWake-owned):** Aurora init with `mem1Size`
   and `mem2Size` (`ref/aurora/include/aurora/aurora.h:108-119`); OS
   threads / message queues / mutexes / alarms (F4); VI retrace counter,
   `VIGetDTVStatus`, reset code as constants with logged IDs; PAD via Aurora
   (`lib/dolphin/pad/pad.cpp:378`); DVD via Aurora (`lib/dolphin/dvd/dvd.cpp`,
   real async worker); ARAM via Aurora (`AR.cpp`).
2. **JKernel (original source):** `JKRHeap`, `JKRExpHeap`, `JKRSolidHeap`,
   `JKRDisposer`, `JKRThread`, `JKRArchivePub/Pri`, `JKRMemArchive`,
   `JKRAramArchive`, `JKRAram*`, `JKRDvdRipper`, `JKRDvdAramRipper`,
   `JKRDecomp`, `JKRFileFinder`, `JKRFileLoader`. Apply `BigEndian<T>` to
   `SDIFileEntry`/`SArcHeader`; `intptr_t` for base arithmetic.
3. **Machine layer (original source):** `m_Do_machine.cpp` (heaps),
   `m_Do_ext.cpp` (heap wrappers, `mDoExt_setSafe*`), `m_Do_dvd_thread.cpp`,
   `m_Do_graphic.cpp` with a host frame driver in place of `JFWDisplay`'s VI
   dependence, `m_Do_mtx.cpp`, `m_Do_controller_pad.cpp`, `m_Do_Reset.cpp`.
4. **Game state (original source):** `d_com_inf_game.cpp` (`g_dComIfG_gameInfo`
   ctor, `dComIfGd_init`), `d_resorce.cpp` (`dRes_control_c` — this is where
   `ObjectResources` is retired), `d_drawlist.cpp` (2D-opa path; guard the
   `J3DDrawBuffer` allocations).
5. **JUtility / JFramework / J2D (original source):** `JUTTexture`,
   `JUTPalette`, `JUTFader`, `JUTAssert`, `JUTGamePad`, `JUTVideo`/`JUTXfb`
   (or host), `J2DPane`, `J2DPicture`, `J2DOrthoGraph`, `J2DGrafContext`,
   `J2DPrint` (buffer only).
6. **Original `d_s_logo.cpp` with `TARGET_PC` guards** replacing
   `d_s_logo_native.cpp`; `phase_2` particle creation admitted only as far as
   `dComIfGp_particle_create` requires (JParticle manager construction,
   4,366 lines in `JParticle/`; loads nothing until `Delete`), or guarded
   behind a logged fail-closed ID until title work.
7. **Audio phase-zero contract:** original `mDoAud_Create` driven from the
   main loop; `JAIBasic` init graph; AI shim (SDL) and DSP boundary (HLE);
   real DVD→ARAM wave load so `checkFirstWaves()` reaches 0.
8. **First frame:** `nintendoInDraw` → `dComIfGd_set2DOpa` → host frame
   driver → `dComIfGd_draw2DOpa()` → Aurora `aurora_end_frame()`.

Which pieces are BlueWake-owned host services: item 1, the frame driver in
item 3, the AI/DSP shims in item 7, and the logged constants. Everything
else should be original TWW source with tracked patches.

Which boundaries are test scaffolding masquerading as progress (F1):
`ObjectResources`/`RarcArchive`, `LogoPlatform` + `d_s_logo_native.cpp`,
`LogoHostPolicy`, the smoke's `audio_initialized`/`first_waves_pending`
toggles, and the uncommitted `AudioInitData`. `ProcessManagerPlatform`'s
`matrix_init`/`paint` callbacks (`process_manager_platform.hpp:5-11`) are
acceptable substrate but should become `MtxInit`/`cAPIGph_Painter` once GX
links. `StaticRelRegistry`, `AuroraDisc`, the exact-width prelude,
`BigEndian<T>`/`RelativeOffset32<T>`, and the seven tracked patches are real
foundation and should be kept.

Does the current native object model preserve enough retail behavior for
later JKR/J3D consumers? **No** (F1): consumers need pointers into the
mounted big-endian archive image with retail entry stride. Only a ported
`JKRArchive` provides that.

### 5.4 When to declare Route B unbounded

Reopen whole-route selection if any of the following is measured:

- JKernel/J2D/JUT port needs more than about 10% of lines changed (twice
  Dusklight's observed density) or requires guest-address arithmetic that
  cannot be expressed with `BigEndian<T>`/`OffsetPtr`/`intptr_t`.
- The first native logo frame is not presented within roughly 150 further
  Route B commits or two calendar weeks of loop time, whichever comes first,
  with the dependency path of §5.3 followed in order.
- J3D model loading (title, Outset) needs per-model patches rather than one
  loader-level conversion.
- Audio reconstruction throughput, once measured over the first ten
  functions, projects more than three months for the boot-and-Outset set
  (`processFrameWork`, `zeldaGFrameWork`, `startSound*`, `makeSound`,
  `sceneBgmStart`, dynamic-wave loads, ~30 functions).
- Any milestone requires capture-derived state or success-returning stubs to
  pass.

---

## 6. Audio closure analysis

Separate the two questions the assignment poses:

**"Audio initialization permits boot."** OBSERVED contract (`d_s_logo.cpp:650`):
boot is permitted when either audio is uninitialized or wave group 2 has
loaded. The authentic sequence is: main loop calls `mDoAud_Execute` every
frame (`m_Do_main.cpp:464`); `mDoAud_Create` issues two DVD commands
(`JaiInit.aaf` to RAM, sequence archive mount; `m_Do_audio.cpp:137-150`),
waits for both, then runs `JAIZelBasic::init` which starts the audio thread,
initializes the AI DMA (`JASAiCtrl.cpp:35`), sets up the ARAM heap
(`JASSystemHeap.cpp:45-53`), boots the DSP task (`dsptask.c:496-511`), and
requests wave-scene group 2 (`JAIBankWave.cpp:67-77`), which completes via
`JKRDvdAramRipper::loadToAram` (`JASWaveArcLoader.cpp:110`) and sets
`wsLoadStatus[2] = 2` (`JAIBankWave.cpp:99-101`).

Smallest authentic contract: all of the above with real source, plus three
BlueWake-owned seams — AI DMA (SDL audio stream, output silent zeros is fine),
DSP task/mailbox (Dolphin Zelda HLE consuming the retail mail stream and
mixing nothing because no channel is active), ARAM (Aurora `AR.cpp`). No
readiness callback. If any seam is missing, `mInitFlag` stays false by the
real code path and the logo proceeds silently; log a unique ID.

**"Audible, correct game audio."** Requires the empty control layer (F5).
Fail-closed, measurable sequence:

1. Compile the JAudio1 tree as-is (12,801 + 3,137 lines) with `TARGET_PC`
   guards on hardware lines; empty bodies compile to their existing empty
   forms and are counted by `scripts/route_b_source_manifest.py`. Measure:
   number of empty bodies *reached* on the logo→title route (instrument the
   empty bodies with a logged ID and counter).
2. Land the three seams; acceptance: `checkFirstWaves()` reaches 0 on the
   private disc with the ARAM transfer size matching Route A's recorded
   ARAM transfer for the same group (Route A logged 4,295 ARAM transfers on
   the 700 tier; the per-transfer log is a usable oracle).
3. Reconstruct in dependency order using Route A traces as oracle:
   `JAIBasic::processFrameWork` → `JAIZelBasic::zeldaGFrameWork` →
   `startSoundBasic/makeSound/startSoundVec/startSoundDirectID` →
   `SequenceMgr::checkEntriedSeq` → `sceneBgmStart`, `load1stDynamicWave`,
   `check1stDynamicWave`, `load2ndDynamicWave`. Acceptance per function:
   Route A trace of the same call site shows the same DSP mailbox / ARAM /
   sequence-object side effects. Label each "equivalent" or "matched" per
   `GOAL_LOOP.md §7.2`; never "done".
4. Only after title BGM is audible under human review, promote the DSP seam
   decision (HLE vs LLE vs native mixer) to a recorded decision with a
   bit-exactness policy. The HLE is recommended first because it is
   protocol-compatible and needs no ROM dumps; the LLE is the exactness
   fallback; a Dusklight-style native mixer is the long-term maintainability
   option.

Do not: transplant JAudio2, bridge to Route A memory, or count a muted build
as audio progress. All three already prohibited in
`ROUTE_B_AUDIO_BOUNDARY_2026-09-01.md:33-38`; this review agrees.

---

## 7. Native rendering closure analysis

### 7.1 Smallest closure for one authentic frame

OBSERVED chain: `dScnLogo_Draw` (`d_s_logo.cpp:468`) → `nintendoInDraw`
(`:118`) → `dComIfGd_set2DOpa(nintendoImg)` → per-frame `mDoGph_Painter`
(`m_Do_graphic.cpp:1585-1940`) → `dComIfGd_draw2DOpa()` (`:1933`) →
`dDlst_2D_c::draw` (`d_drawlist.cpp:663-666`) → `J2DPicture::draw`
(`J2DPicture.cpp:335-395`) → `GXBegin(GX_QUADS…)` with 4 vertices and a TEV
setup from `setTevMode` (`:494-540`) → Aurora GX. The texture comes from
`Logo.arc` entry 3 (`nintendo_376x104`, per `ref/tww/assets/GZLE01/res/Object/Logo.h:10-17`)
via `JUTTexture` → `GXInitTexObj` (`JUTTexture.cpp:82-95`). The red tint is
`setWhite({0xDC,0,0,0xFF})` (`d_s_logo.cpp:760`) feeding `GX_TEVREG1`.

Unit list and ordering: §5.3 items 1–6, 8. Aurora provides every GX call on
this path (`GXBegin/End`, `GXPosition3f32`, `GXSetTevOrder`, `GXLoadTexObj`,
`GXSetVtxDesc`, display lists, GD) as functions rather than WGPIPE stores
(`ref/aurora/include/dolphin/gx/GXVert.h:32-60`; `lib/dolphin/gx/GXDispList.cpp:11-49`).
`GXCopyDisp` and the copy-config calls are no-ops; Aurora presents its own
EFB (`lib/aurora.cpp:264-336`). OBSERVED.

### 7.2 Hazards

- **ABI/layout:** F6 items; `ResTIMG` must become `BE<T>` fields;
  `mDoGph_gInf_c::create` hand-builds a host-endian `ResTIMG` for the
  framebuffer capture texture (`m_Do_graphic.cpp:96-107`) — under a `BE<T>`
  `ResTIMG`, that code writes swapped values and must be guarded.
- **Resources:** `dRes_info_c::loadResource` converts BMD/BDL/BMT/BCK via
  `J3DModelLoaderDataBase::load` (`d_resorce.cpp:144-374`) but leaves `DAT`
  (BTI) as raw pointers; the logo path only needs the raw case. Title and
  Outset need the J3D loader conversion, which is where the
  `mDiffFlag`/`JSUConvertOffsetToPtr` hazards live.
- **Renderer:** `dDlst_list_c::init` allocates fifteen `J3DDrawBuffer`s
  (`d_drawlist.cpp:1888-1902`); `mDoGph_Painter` constructs `JPADrawInfo` and
  calls particle 2D draws (`:1914-1916`); `j3dSys.drawInit/reinitGX` are
  invoked. Guard these with logged IDs for the logo tier, admit them for
  title.
- **Presentation:** `JFWDisplay` depends on `VIGetRetraceCount` and
  `VIFlush` (`JFWDisplay.cpp:117,333,358`), which Aurora lacks; a host frame
  driver that owns begin/end render and a retrace counter is lower risk than
  porting `JFWDisplay` (Dusklight's `VIWaitForRetrace` stub at
  `stubs.cpp:344-356` shows the minimal contract).
- **Threads:** the frame requires the DVD thread and audio thread to exist
  (F4) even though their completions are not consumed.

### 7.3 When native FPS becomes meaningful

Not at the logo (one quad). Not at title if the title's 3D content is
guarded. The first meaningful native FPS sample is the first Outset frame
with the retail actor set drawing through J3D; before that, "FPS" measures
Aurora's presentation loop. INFERRED. A frame limiter / pacing service
(retrace-derived, 30 Hz game logic) must exist before any FPS claim on
either route; neither route has one. OBSERVED.

### 7.4 Link hair

- Route A: fix landed at the GX contract with a regression; three capture
  predicates failed to present Link; visual status unadjudicated.
  OBSERVED (`BLOCKERS.md:475-480`; `CURRENT.md:504-524`).
- Route B renders through Aurora GX, not GXCore; the Route A defect class
  (single-texmap slot binding in GXCore's plan builder) does not exist in
  Route B's renderer. INFERRED. Whether Aurora reproduces Link's hair
  correctly is a separate, untested question that Dusklight's Link (a
  different model with similar material tricks) makes likely but does not
  prove. SPECULATIVE.
- Assessment: **likely fixed in Route A, impossible to adjudicate until a
  presented frame contains Link**; a one-hour unscripted session is the
  cheapest adjudication and also retires TD-002's "never played" caveat.

### 7.5 Reducing future native rendering risk before broad scene conversion

- Apply `BE<T>` to `ResTIMG`, `SArcHeader`, `SDIFileEntry`, and the J3D
  block headers in one pass with a compile-time size assertion per struct
  (Dusklight's approach), before the first BMD is loaded.
- Build the Route A `.dolt` trace → Aurora structural comparison
  (draw count, texture ids, TEV stage counts per draw) as a public
  game-data-free oracle so title/Outset conversions have a regression that
  is not a pixel hash across two renderers.
- Keep `-Werror=pointer-to-int-cast/shorten-64-to-32` on; do not add
  `-Wno-*` to admit J3D. The narrowing errors in J3D are the port's work
  list, not noise.

---

## 8. Finish-line dependency map

| # | Milestone | Depends on | Strongest acceptance test | Largest unknown |
|---|---|---|---|---|
| 1 | First native logo frame | §5.3 items 1–6, 8 | Original `d_s_logo.cpp` runs unmodified through `phase_2`; presented frame's texture identity and draw list match Route A's logo capture structurally; private PNG digest recorded | Adaptation density of JKernel/JUT/J2D under strict narrowing; OS thread service correctness |
| 2 | Native title screen | 1 + `dvdWaitDraw` completion (24 ARAM mounts, 4 RAM loads, particle common, fonts, message archives) + `d_s_title.cpp` + J3D loader/`J3DModel` + `dDlst` 3D path + `JKRAramArchive` | Title frame with ship/logo drawn via J3D; Route A retrace-indexed milestone parity for title-ready | J3D serialized-offset conversion (`J3DModelLoader`, `mDiffFlag`, `J3DTexture::setResTIMG`); `J3DModel::calcWeightEnvelopeMtx` is the one empty J3D body |
| 3 | Unrestricted title input | 2 + Aurora PAD → `mDoCPd_Read` → `JUTGamePad` → `CPad_*` | A human presses Start and reaches file select; input latency noted | `JUTGamePad` reset-callback and rumble paths; `m_Do_controller_pad.cpp` portability |
| 4 | First controllable Outset frame | 3 + file select (`d_menu_*`, J2D screens `.blo`, `dSv` save state), name entry (`d_s_name.cpp`), opening (`d_s_open.cpp` + JStudio cutscene engine, 14 headers), `d_s_play.cpp`, stage/room loading (`d_stage.cpp`, DZS/DZR parsing), `d_a_player` (DOL), 20–40 actor RELs, collision (`d_bg_*`), camera (`dCamera_c` and the deferred J3D extension) | Human moves Link on Outset; Route A digest parity for scene/actor creation order | JStudio opening cutscene (`JStudio/` is 0 `.cpp` files at the pin root; lives under `JStudio*` subdirs) and the count of empty bodies among Outset actors — not yet censused |
| 5 | Stable original-speed Outset | 4 + frame pacing service + profiling | 30 FPS presented, ≥99.5% speed, p95 ≤ 36.7 ms per `PRD.md NFR-001` over a fixed Outset route | Aurora Metal path cost on this hardware; no measurement exists |
| 6 | Audio acceptance | 1's seams + §6 reconstruction (~30 functions for boot/Outset) + human listening | Named events (title BGM start, sea ambience, SE) match Route A's DSP/ARAM event sequence; human review | Reconstruction throughput; DSP seam exactness policy |
| 7 | Save/load acceptance | 4 + Aurora CARD (`lib/card/*`) + `m_Do_MemCard*.cpp` + `dSv_*` | Copied-card hash unchanged when no save; save/quit/reload route matches Route A card digest | `.gci` interchange with Route A's HLE-written card |
| 8 | macOS packaging and external testing | 5,6,7 + minimal shell (import, settings, fullscreen, diagnostics) + reproducible bootstrap on a clean Mac | Second machine builds and launches from documented commands with its owner's disc | Nothing architectural; purely product work (TD-007) |
| 9 | Broad campaign compatibility | 8 + per-content empty-body census (4,175 bodies tree-wide) + REL address-reuse behavior under static linking | `PRD.md §13 P5/P9` catalog | Upstream decomp pace; this milestone is not fully under BlueWake's control |
| 10 | iOS/iPadOS | 8 stable | `PRD.md P7/P8` | Deferred; no inference from macOS |

Compile coverage (units linked), synthetic tests (six Route B CTests),
authentic resource validation (Aurora tier), visible product milestones
(none native yet), and playable-game progress (Route A scripted only) are
distinct categories; only milestones 1–4 above move the third and fourth.

---

## 9. Ranked next actions (maximum ten)

| # | Action | Information gain | Implementation value | Effort | Prerequisite | Acceptance test | Failure / reorientation | Direct path? |
|---|---|---|---|---|---|---|---|---|
| 1 | **Port JKernel heaps + archives + `JUTTexture` as original source over Aurora MEM1; make original `dRes_control_c` return the toon `ResTIMG*` from private `System.arc` via `AuroraDisc`** | High: measures true adaptation density on the largest bounded subsystem | High: retires `ObjectResources`; unblocks phase one | 2–5 loop-days | Aurora built with `mem1Size` | Original `phase_1` executes unmodified; `setToonImage` receives a valid BE header; `SDIFileEntry` stride asserted at 0x14 | >10% lines changed, or guest-address arithmetic unrepresentable → reopen route | Yes |
| 2 | **BlueWake-owned OS thread / message queue / mutex / alarm service** (Dusklight-informed, provenance-reviewed) | High: first real concurrency contract | High: required by DVD thread, audio thread, JKRAram | 1–3 loop-days | none | `mDoDvdThd` mounts `Logo.arc` asynchronously and `sync()` completes; `JKRThread` smoke under TSan | Alarm/queue semantics cannot preserve retail ordering → design a documented deviation, not a stub | Yes |
| 3 | **Host frame driver + JUT/J2D/dDlst 2D path over Aurora GX; original `d_s_logo.cpp` with `TARGET_PC` guards replaces `d_s_logo_native.cpp`; present the Nintendo logo** | Highest: first native product observable | Highest | 3–7 loop-days | 1, 2 | Milestone 1 acceptance; private PNG + public structural digest | Requires J3D/JStudio/JAudio admission to draw one quad → Route B is not bounded at the scene layer; reopen | Yes |
| 4 | **Audio phase-zero seams: compile JAudio1 tree as-is; SDL AI shim; Dolphin Zelda HLE at `dspproc.c`; Aurora ARAM; real wave group 2 load** | High: converts "audio is the blocker" into a measured count of *reached* empty bodies | Medium now, high later | 3–6 loop-days | 2 | `checkFirstWaves()==0` on the private disc; ARAM transfer count/size matches Route A's log for the same load; logo proceeds with `mInitFlag` true | Init graph needs more than the three seams, or HLE protocol mismatch → fall back to LLE with ROM dumps | Yes |
| 5 | **Title closure: `dvdWaitDraw` completion, J3D loader conversion with `BE<T>`/`OffsetPtr`, `d_s_title.cpp`, PAD input** | High: first J3D and first input | High | 1–3 loop-weeks | 3 | Milestone 2 and 3 acceptance | Per-model patches needed → reopen | Yes |
| 6 | **Audio control-layer reconstruction using Route A traces as oracle** (order in §6 step 3) | Medium per function; high aggregate | High for milestone 6 | ongoing; measure rate over first 10 | 4 | Per-function trace parity; title BGM audible under human review | Projected >3 months for the ~30 boot/Outset functions → reopen seam policy or accept muted milestones explicitly | Yes (audio) |
| 7 | **One unscripted human Route A session** (no `BLUEWAKE_PAD_*`, keyboard, listen) | High: pacing/latency/audio/hair observations no digest gives | Low code | 1 hour | none | Written observations labeled non-digest; hair adjudicated | none | Indirect (oracle quality) |
| 8 | **Record the Route A fallback decision**: relaxed-exactness edge loop with generator-side HLE intercepts, plus one decomposition of why the inline filter cost equaled the callback | Medium: closes an unexplained measurement | Low now; high only if Route B fails 5.4 | 1 loop-day for the decomposition | none | Decision row in `DECISIONS.md` with reopen condition | none | No (insurance) |
| 9 | **Evidence and CI repair**: resolve the 18 tracked `local-research` files with the user (policy or history), make CI build Route B and run its CTests, promote `/tmp` artifacts whose SHAs appear in ledgers, archive `CURRENT.md` history, add a `CURRENT.md` authority rule, banner `WAITING_FOR.md`/`HANDOFF.md`, fix stale numbers in `TECH_DEBT.md:15,252,258` and `CURRENT.md:141` | Low | Medium: protects everything else | 0.5–1 loop-day | user decision on the card files | `audit_repo.sh` passes; CI green with Route B tests | none | No |
| 10 | **Frame pacing / limiter service** (retrace-derived 30 Hz for both routes) | Medium | Required for any FPS claim | 1–2 loop-days | 3 | Presented frame cadence measured at 30 Hz on the logo with vsync on/off | none | Yes (for milestone 5) |

**Begin immediately:** action 1. **Do not attempt next:** the "default macOS
application composition" of `AuroraDisc` + `ObjectResources` +
`LogoHostPolicy`, any further `LogoPlatform`/`AudioInitData`-style
re-implementation of complete retail source, audio reconstruction before the
seams exist, or any Route A speed shape.

---

## 10. Stop / reorientation conditions

- Actions 1–3 exceed the §5.4 density or budget bounds → record Route B as
  long-horizon, adopt action 8's fallback as the active speed route with
  the exactness policy change recorded as a decision.
- Action 3 needs J3D/JStudio/JAudio admission to draw one quad → the scene
  layer is not decomposable; reopen route selection.
- Action 4 finds the JAudio1 init graph needs more than AI/DSP/ARAM seams →
  audio moves from "seam" to "reconstruction" for boot, and milestone 1
  proceeds silent with a logged ID (authentic per `d_s_logo.cpp:650`).
- Any tier introduces a success-returning readiness callback, captured
  state, or a bridge to Route A memory → stop the tier; the loop's own rules
  already say so.
- Three consecutive Route B tiers that each add a BlueWake-owned
  re-implementation of an existing complete retail unit → anti-stall
  trigger; the next tier must admit an original unit.

---

## 11. Documentation and evidence corrections

1. `GATES.md:9`: add "under scripted PAD waypoints; no unscripted human
   session recorded" to the P4+ cell. OBSERVED gap.
2. `TECH_DEBT.md:15-16` (470.60 s), `:252` (`-O1` 568,158,056 bytes),
   `:258-259` ("about 101 minutes"): superseded by 343.88 s and the 15,378 s
   hybrid `-O2` build (`PERFORMANCE.md:97-111`, `:1723-1726`). OBSERVED.
3. `CURRENT.md:141` ("25–35%"), `PORTING_HISTORY.md:318` (present-tense
   35–45%): reconcile to 25–30% or the revised figure in §12. OBSERVED.
4. `CURRENT.md`: add a top-of-file authority rule like `BLOCKERS.md:3-7`;
   move everything below the current banner to `docs/status/archive/`.
   The 08-25 review asked for this; the file has since grown from 2,332 to
   6,336 lines. OBSERVED.
5. `WAITING_FOR.md`: add a superseded banner or delete. `HANDOFF.md:12-16`:
   retitle the 2026-08-13 block as historical. OBSERVED.
6. `BLOCKERS.md:103-109` and `CURRENT.md:74-79` "next smallest action":
   replace with action 1 of §9 and record why the composition step is
   deferred (F1).
7. Ledger vocabulary: reserve "bounded" for a measured adaptation density or
   symbol count, not for "the smoke passed". Reserve "native archive
   ownership" for a service that can return retail resource pointers.
8. Evidence: the 18 tracked `local-research/` files (F8) need a user
   decision. Options: (a) `git rm --cached` and keep local (audit passes,
   history still contains them); (b) history rewrite with explicit user
   authority; (c) an audit-policy revision declaring seed-card images
   non-derived. Do not choose autonomously. Note that README-only tracking
   under `local-research/evidence/` is a reasonable policy if adopted
   deliberately.
9. Claims supported only by unavailable state: none found among the
   first-400-line `CURRENT.md` citations; every cited evidence directory
   exists. The Route A FPS figures (6.0 / 13.55) predate the current
   baseline and should be marked stale until remeasured. INFERRED.
10. `docs/archive/status/COMPATIBILITY.md` is 5 lines; `tests/coverage/catalog.json`
    is still not a coverage universe. Unchanged since 08-30. OBSERVED.
11. Commit cadence: today's 40 commits are coherent and pushed, which is
    good. Recommend continuing to push `main` after every green tier
    (current practice), pruning or pushing the 35 local `codex/*` branches,
    and adding one durable checkpoint tag at each of milestones 1–4 so a
    resuming agent can bisect a regression across a native milestone.

---

## 12. Revised progress and schedule estimate

- **Overall macOS stable-playable:** 20–30%. The upper bound credits Route A
  as a validated behavioral oracle, the disc/topology pipeline, GX contract
  knowledge, and the Route B foundation; the lower bound counts only work
  that survives into the shipped Route B artifact. Hypothesis 7's 25–30% is
  defensible under the former reading. INFERRED.
- **Route B implementation:** 3–7% today; 10–15% at milestone 1; 20–30% at
  milestone 4; audio and campaign dominate the remainder. INFERRED. The
  ledger's 5–10% is at the generous end.
- **Schedule (evidence-conditioned ranges, not promises, per `PRD.md §8.6`):**
  milestone 1 in 1–3 weeks of loop time if actions 1–3 stay within §5.4
  bounds; milestone 2–3 a further 1–3 weeks; milestone 4 a further 4–8 weeks
  (JStudio opening, J3D, actors, collision); milestone 5 unmeasurable until
  milestone 4; milestone 6 months, paced by reconstruction; milestones 8–9
  many months and partly external (upstream decomp). SPECULATIVE beyond
  milestone 2.

---

## 13. Unknowns and exact measurements needed

1. **Adaptation density of the JKernel/JUT/J2D port under strict
   narrowing** — lines changed ÷ lines compiled after action 1. Decides
   §5.4's first condition.
2. **Empty bodies reached on the logo→title route** — instrument every
   marker body with a counter; run the native route. Converts the 133/152
   census into a path-specific number.
3. **Outset actor REL set and its empty-body count** — parse `ActorDat.bin`
   and the `sea_T` room-44 DZR privately; report counts only.
4. **Why the generated-side intercept filter cost equaled the pure callback
   (23.18 vs 23.02 s)** — one `sample`/`perf` decomposition of the filter
   build. Decides whether Route A's −60% lever is recoverable.
5. **ARAM transfer parity for wave group 2** — Route A's per-transfer log vs
   Route B's `JKRDvdAramRipper` on the same disc.
6. **Aurora Metal presentation cost on this Mac** — measure at milestone 1
   with an empty scene so later FPS numbers have a floor.
7. **Static-initializer census of statically linked REL sources** — grep for
   namespace-scope objects with non-trivial constructors in
   `src/d/actor/*.cpp`; count those that touch JKR heaps.
8. **Hair adjudication** — one presented frame with Link in either route.
9. **Human observations** — pacing feel, input latency, audio quality: only
   action 7 answers these.
10. **Whether the four tracked `.card` images are derived from the user's
    retail save** — only the user knows; the README calls them seed cards.

---

## 14. Commands, files, and line references used

Commands (all read-only): `git status --short`, `git log`,
`git branch -vv`, `git branch -r --contains`, `git ls-files local-research`,
`git check-ignore -v`, `git diff --stat`, `bash scripts/audit_repo.sh`,
`ls /tmp | grep -c bluewake`, `wc -l`, `grep -n`, `sed -n`, `find`,
`stat -f %Sm build/route-b-*`, `cat build/route-b-*/Testing/Temporary/LastTest.log`,
`git -C ref/{tww,aurora,dusk} log -1`, `git -C ref/tww ls-remote`. No
build, no BlueWake process, no Simulator.

Documents read in full: `docs/archive/PRD.md`, `docs/GOAL_LOOP.md`,
`docs/archive/status/GATES.md`, `TECH_DEBT.md`, `REORIENTATION_2026-08-22.md`,
`REORIENTATION_2026-09-01.md`, `INDEPENDENT_REVIEW_2026-08-25.md`,
`INDEPENDENT_REVIEW_2026-08-30.md`, `EXECUTION_ROUTE_REVIEW_2026-09-01.md`,
`EXECUTION_ROUTE_CENSUS_2026-09-01.md`, `ROUTE_B_AUDIO_BOUNDARY_2026-09-01.md`,
`ROUTE_B_VERTICAL_BOOT_2026-09-01.md`, `FINISH_LINE.md`, `DECISIONS.md`,
`route_b/README.md`, `README.md`; `CURRENT.md` and `BLOCKERS.md` heads plus
delegated full-file audits; `PERFORMANCE.md` 08-30 to 09-01 entries.

Repository files cited: `route_b/CMakeLists.txt`,
`route_b/include/bluewake/route_b/{logo_platform,logo_host_policy,object_resources,static_rel_registry,native_prelude,aurora_disc,process_manager_platform,audio_init_data}.hpp`,
`route_b/src/{d_s_logo_native,logo_host_policy,aurora_disc,f_pc_manager_adapter}.cpp`,
`tests/route_b_application_smoke.cpp`, `tests/route_b_logo_host_policy_test.cpp`,
`patches/tww/0001..0007`, `scripts/prepare_route_b.sh`, `scripts/audit_repo.sh`,
`.github/workflows/audit.yml`, `.gitignore`, `config/route_b_boot_spine.json`,
`config/route_b_audio_startup_probe.json`, `runtime/host/src/main.c`,
`runtime/host/CMakeLists.txt`, `runtime/host/src/dsp_adapter.{h,cpp}`,
`local-research/evidence/route-b-source-manifest-v1-20260901/README.md`,
`local-research/evidence/return-census-v1-20260831/README.md`,
`local-research/evidence/donor-authentic-cycle-contract-v6-20260831/README.md`.

Pinned source cited: `ref/tww/src/d/d_s_logo.cpp` (`:90-115,118-132,418-466,468-501,633-681,684-724,737-958`),
`ref/tww/src/m_Do/m_Do_main.cpp:407-464`, `m_Do_audio.cpp:135-197`,
`m_Do_graphic.cpp:79-117,1585-1940`, `m_Do_dvd_thread.cpp:26-73`,
`ref/tww/src/d/d_resorce.cpp:56-66,144-374,378-466,640-653`,
`d_drawlist.cpp:655-666,1888-1936,2019-2025`, `ref/tww/include/d/d_com_inf_game.h:4012-4072`,
`ref/tww/src/c/c_dylink.cpp:518-646`, `f_pc_profile_lst.cpp:5-30`,
`f_pc_manager.cpp:266-300`, JSystem: `JKRArchive.h:60-68`,
`JKRMemArchive.cpp:62-64,152`, `JUTTexture.cpp:24-122`, `JUTTexture.h:14-37`,
`J2DPicture.cpp:111-134,335-395,494-540`, `JFWDisplay.cpp:36-358`,
`JSupport.h:9`, `J3DModelLoader.cpp:218-574`, `J3DTexture.h:50-54`,
`J3DMaterial.h:121`, `J3DPacket.cpp:375`; JAudio: `JAIBasic.cpp:55-322`,
`JAIZelBasic.cpp:153,312,840-878,1465-1508`, `JAIBankWave.cpp:37-124`,
`JASAudioThread.cpp:26-139,229`, `JASAiCtrl.cpp:35-93`, `JASSystemHeap.cpp:45-53`,
`JASWaveArcLoader.cpp:22,110`, `JASDSPInterface.cpp:153`, `dspproc.c:23-74`,
`dsptask.c:496-516`, `osdsp_task.c:16`, `JAIInitData.cpp` (121 lines, 0 markers).

Donors cited: `ref/aurora/include/aurora/aurora.h:70-140`,
`ref/aurora/lib/dolphin/{AR.cpp:16-114,os/*,vi/vi.cpp,pad/pad.cpp:378,dvd/dvd.cpp,card.cpp:156}`,
`ref/aurora/lib/dolphin/gx/{GXVert.cpp:37-94,GXDispList.cpp:11-49,GXFrameBuffer.cpp:158-210}`,
`ref/aurora/include/dolphin/gx/GXVert.h:12-60`, `ref/aurora/lib/aurora.cpp:264-336`;
`ref/dusk/src/dusk/stubs.cpp:99-133,188,222-231,344-358`,
`ref/dusk/src/dusk/{OSThread,OSMutex,OSContext}.cpp`,
`ref/dusk/src/dusk/audio/{DuskDsp.cpp:15-63,458,DspStub.cpp,DuskAudioSystem.cpp:47-135}`,
`ref/dusk/src/d/d_s_logo.cpp` (17 `TARGET_PC` sites; `:1333-1334`),
`ref/dusk/src/c/c_dylink.cpp:22-23,901-973`, `ref/dusk/include/helpers/{endian.h:31-70,offset_ptr.h:8-60}`,
`ref/dusk/libs/JSystem/src/JUtility/JUTTexture.cpp:23-27`,
`ref/dusk/libs/JSystem/src/JKernel/*` (60 `TARGET_PC` sites),
`ref/dusk/LICENSE.md` (CC0);
`ref/recompcore/Source/Core/Core/HW/DSPHLE/UCodes/{ZeldaUCodesTable.cpp:29-30,Zelda.cpp:54}`.

Pins verified: `ref/tww` at `03d27aa1` (2026-08-18) with the seven Route B
patches applied; `ref/aurora` at `8b690b60` (2026-08-20); `ref/dusk` at
`5f0f3d4e` (2026-08-21); upstream `zeldaret/tww` HEAD `94a5eadb` at review
time.
