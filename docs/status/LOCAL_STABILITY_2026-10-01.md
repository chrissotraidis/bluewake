# Local stability follow-up, October 1, 2026

Work on PR #12, stacked on PR #10. The iPad and Parallels are untouched.
Tests use synthetic data on the M3 Max Mac; no personal module is loaded unless
explicitly identified below. Compilation and mocks do not prove gameplay.
Public releases remain paused. Smooth Motion remains experimental and off by default.

## Save data

- 1.1: reproduced the remove-and-retry path with injected rename failure (regression
  assertion failed before fix). Deleted the fallback. The same ASan/UBSan regression
  passes and reopening the card returns its previous bytes. Registered on Mac and
  native Windows CI. Runtime patch 0115. Real in-game save-failure UI is unverified.
- 1.2: fflush then F_FULLFSYNC (Apple), fsync (other POSIX), or _commit (Windows)
  precedes replacement; POSIX directory sync follows it. Flush errors return save
  failure. Injected fflush failure prevents rename and preserves the old save;
  sanitizer regression passes. Runtime patch 0116. Power-loss durability and forced
  reboot on Windows/iPhone remain unverified. A directory-sync failure after rename
  reports failure although replacement may already be visible; no durability claim.
- 1.3: nonblocking session lock (flock/LockFileEx), retained lock-file inode,
  process-specific temporary file. The sanitizer regression rejects a second handle,
  then reopens after close. Restore/import now suspends card dispatch under a mutex,
  waiting for in-flight writes before replacement; failure resumes dispatch. A lock-count
  regression checks 10,000 ordinary guest dispatches take no card lock, both closed-card
  SDK probe contracts return to the guest, and suspended writes return BUSY. The first
  expectation incorrectly treated a handled SDK probe as unhandled, then as a result-code
  variant; corrected to check boolean CARDProbe and result-code CARDProbeEx. Runtime
  patch 0117. iOS concurrency and Windows two-process behavior need platform checks.
- 1.4: read-only dol_card_validate uses the complete loader and its checksums;
  restore/import reject invalid replacements and retain them. Startup recovery on
  Apple offers valid local Backups/.bak or a new card, preserving the original at
  a unique .corrupt path only after the player chooses. Unreadable existing cards
  are never mistaken for missing cards. Loader size is bounded and short-read
  cleanup closes the stream. Truncation/bit-flip sanitizer tests pass and prove
  rejected bytes are unchanged. Runtime patch 0118. A follow-up also flushes and validates the staged restore,
  using unique names and retaining both source and failed stage. UIKit recovery needs simulator
  and physical-device interaction checks; no device was accessed.
- 1.5: flushed atomic .bak copy before live replacement, plus seven fixed UTC-day
  slots (one snapshot per day, rotated weekly). Backup failure refuses the save.
  Sanitizer regression loads .bak after two writes and verifies the first write's
  bytes. Runtime patch 0119. Physical save/reload and backup recovery remain unverified.
- 1.7 SRAM: staged, checked writes/flush/close/rename preserve the previous file
  on failure. Synthetic rename-failure/retry test passes under ASan/UBSan.
- 1.7 desktop settings: checked staged writes; clear dirty only after successful
  replacement. Windows retries no sooner than one second after a failed attempt.
  Shared synthetic dirty/failure/retry regression passes under ASan/UBSan. Actual
  options-menu persistence, permissions errors and Windows settings remain unverified.
- 1.7 save states: write to a per-process staged file; publish only after successful
  gzip close and disk flush. Invalid chunk requests poison the writer instead of
  publishing a partial state. Extended ASan/UBSan state test passes and verifies an
  interrupted/rejected second save leaves the first loadable. Gameplay state restore
  on the candidate remains unverified.
- 1.7 iOS card location: new live cards use Application Support/BlueWake; exports
  and Backups remain in Documents. Startup copies legacy storage byte-for-byte,
  retaining the original and any existing target. Migration failure logs and uses
  the preserved legacy path. Synthetic copy/existing-target test passes. tvOS stays
  in Caches; 1.6 cloud storage is deferred (requires Apple TV/cloud entitlement work).
  Physical iOS migration and Files-app isolation remain unverified.
## Crashes

- 2.1: Restart saves successfully, requests SDL quit, and relaunches only after host
  shutdown/card close. No exit from an overlay frame. Normal quit returns zero.
  Synthetic request failure/consumption and stop-status regression passes under
  ASan/UBSan; registered on Mac/Windows. Real Windows Restart with HLE/LLE, worker
  teardown and subsequent save/reload remain unverified.
- 2.2: Windows launch.pending is flushed just before host entry; clean exit or 600
  rendered frames clears it. A pending marker or --safe-mode backs up settings and
  resets restart-only choices, forces HLE/mods-off session defaults, and shows a
  recovery banner. --hle-audio is explicit. Marker/backup/clear sanitizer test passes
  and is registered on Mac/Windows. Real crash/relaunch recovery needs Windows.
- 2.3: saved/session/launch settings are separate. Only fields explicitly changed
  in the menu/hotkeys are copied to saved preferences; launch window placement is
  the baseline, not an edit. HLE/LLE and option overrides now appear in the session
  UI. Review reproduced an option-reset persistence failure; explicit edits now
  persist, while the session-only `none` baseline is displayed and never saved. Synthetic scale/fullscreen/LLE/Smooth Motion and option isolation test passes
  under ASan/UBSan, registered on Mac/Windows. Actual Windows UI still needs hardware.
- 2.4: UIKit close actions serialize card close with dispatch, flush logs, then use
  _exit, bypassing static renderer destruction. Host failures now display an alert
  instead of a silent process exit. Synthetic child-process test proves card-close
  callback runs and static/atexit callback does not. Fixed a compile-time missing
  card-result declaration found by the local host build. Simulator crash-log and
  physical Restore/Import close checks remain unverified.
- 2.5: pipeline end flag is published under the predicate wait mutex before
  notification. Actual pipeline-cache init/shutdown test passes 1,000 cycles in
  1.01 seconds on Mac, with a 45-second CTest timeout and an empty synthetic cache.
  Runtime patch 0120. No GPU/game is needed; rare pre-fix hang was not reproduced.
- 2.6: separate raw WriteFile crash log, bounded stack walk with null-call caller
  recovery, terminate/SIGABRT hooks, abort policy and startup stack guarantee.
  Allocation-free hex formatter sanitizer test passes; crash driver cross-compiles
  and links with LLVM-MinGW on Mac. Native Windows child-process null/abort/terminate
  checks pass on the native Windows CI runner (20-test run at fcb265c);
  no local Windows game or GPU is involved. __fastfail bypasses
  in-process handlers; external WER/minidump collection remains needed for that case.
  Also moved normal log writes before console output, added PID to session names,
  checked the hotkey hook and released it/timer resolution on exit (2.7 subset).
## Audio

- 3.1: retry unavailable output every two seconds and on device-added events; show
  an audio-unavailable overlay. Actual sink test fails SDL initialization with a
  nonexistent driver, verifies backoff, then recovers with the dummy driver and
  avoids reopening an existing stream. Mac CTest passes (2.18 s), registered on
  Windows. Runtime patch 0121. Output-disabled/enabled Windows and iOS speaker
  recovery remain unverified; dummy-driver PCM is not speaker acceptance.
- 3.2: summary prints dropped pushes and frames, including unavailable output;
  opt-in DOL_AUDIO_SINK_CAPTURE writes raw interleaved S16 native-endian L,R bytes
  only after successful SDL submission, after dropping/stretching. Refuses existing
  capture files; sample-rate changes log byte offsets. Actual dummy-sink sine test
  proves discard and full-queue drops/counts, and verifies capture bytes/length.
  Mac CTest passes (0.64 s). Runtime patch 0122. Captured sink input still does not
  prove speaker playback; iOS/Windows intro guest-versus-sink comparisons are pending.
- 3.3: iOS reactivates AVAudioSession on interruption end and app activation,
  checks errors and retries while active. Successful activation requests host-thread
  sink recovery. Runtime checks paused/lost devices on a bounded 250 ms interval,
  resets playing, and resumes after prebuffering. Extended actual dummy-sink pause/
  resume test passes (0.58 s). Runtime patch 0123. Siri/call/Music and backgrounded
  interruption checks remain unverified on iOS; no iPad access.
- 3.4 source check: personal USA DOL SHA-256
  `680e347ef8c77739165908cc6a375e5af714f354494daca17a1c34e53c275bec`:
  `0x80232C78 = 0x48000F6D` (`bl 0x80233BE4`),
  `0x80232C88 = 0x4082000C` (`bne 0x80232C94`). Recorded originals in the
  option comments; both NOP replacements match Better Wind Waker commit 4501481's
  [skipintro assembly](https://github.com/WideBoner/betterww/blob/4501481/asm/patches/skipintro.asm)
  and patch diff. The 15-option specification still parses and skip remains off.
  The completed private option module was then checked in sequential native Metal
  runs, mods off then only skip on, as recorded below. Speaker checks remain
  unverified. No DOL/module/capture is published.
## Controls

- 4.1: Mac/Windows Swap A/B and X/Y settings apply to each controller's restored
  default targets, preserving its native/NSO layout. Settings persist and reapply
  on port or SDL attachment-ID changes, including a replacement at the same port index.
  Actual SDL virtual-controller disconnect/reconnect at port zero verifies a new ID
  and reapplies the South-to-B swap. Shared pure-C regression passes exhaustive 16-bit combinations,
  involution/non-face preservation and three default orders under ASan/UBSan;
  registered in both CMake files and Windows CI. Real Switch Pro/Xbox/hot-plug UI
  acceptance remains unverified; no controller hardware has been assumed.
- 4.2: Windows enables keyboard/gamepad navigation, routes controller Back through
  the existing event observer, and calls PADBlockInput for the menu. Actual SDL
  virtual-controller CTest passes on Mac (0.53 s): remap roundtrip, neutral buttons/
  stick while blocked, held A swallowed after close, and new A accepted after release.
  Registered on Windows. Real menu/controller navigation and Link movement still need
  Windows/Switch Pro/Xbox acceptance; synthetic input is not hands-on gameplay.
## Performance diagnostics

- 5: BLUEWAKE_GX_FLUSH_FROM controls the per-retrace census start (default 13,800;
  zero includes startup). Exit prints all-call totals/max, including presents. Fixed
  nanosecond rollover arithmetic that could manufacture a huge flush time.
  DOL_AURORA_PRESENT_LOG emits actual surface-present completion timestamps;
  frame_tail.py --kind retrace|display separates VI delivery from shown-frame
  intervals, reports p50/p95/p99/rate/stalls, and no longer calls VI gaps rendered
  acceptance. C rollover/parser sanitizer test and two Python timing fixtures pass.
  Runtime patch 0124. No QoS, cache, worker, frame-slot or quality optimization is
  made without a matched measurement. The 30 Hz game rate stays distinct from
  nominal 60 VI retraces/s and selected 30/60/120 presentation.

## Additional reproduced race

- 2.7 / 6: the existing five render-worker tests under ThreadSanitizer reproduce a
  read/write race on `g_workerThreadId`. Replaced the shared ID with a thread-local
  worker marker. The identical five tests pass with no sanitizer report, plus a
  bounded 1,000-cycle actual-worker identity regression passes in CTest. Patch 0125.
  This does not establish that all renderer races are fixed.

## Validation and device evidence

Candidate source `3d7b5a0`, RecompCore `6699be9c9e48ec27e78730a4c1e089a71d50fdb7`
(patches 0115-0125), built on the M3 Max Mac. No iPad/device installation, device
backup commands, Parallels control, public build upload or release was performed.

- All **238** registered Mac CTests pass, including 42 BlueWake tests and the runtime
  suites. Three runtime targets excluded from the default build were explicitly
  built before the complete run. Initial missing-test executables were corrected;
  no tests were disabled. ASan/UBSan regressions and the five worker TSAN tests pass.
- Mac host, iOS and tvOS application targets compile/link. Executable SHA-256:
  - Mac: `f4f377b4478975b90902d98e3c3e27ffc5531f68096e5492fd2f5d51d5a51993`.
  - iOS: `d1461c63bba43c03717a6a93beb2aac119c28a60307e0c9eff0a590c4bc138ae`.
  - tvOS: `e60ed9f97c0f20a0ddd084bdc918a8ab492fd251a93c59b817cfb2c6c3576bf1`.
  Builds remain private. These hashes identify compilation only.
- [Native Windows source-only CI at 3d7b5a0](https://github.com/chrissotraidis/bluewake/actions/runs/36859176055)
  passes host compilation/linking and all **21** regressions, including the actual-worker,
  card-dispatch lock and same-port replacement-controller checks. Earlier source-only
  runs at fcb265c/c0adef9 passed 20 tests. The first card test missed a compatibility
  include path; the first dispatch-lock test expected the wrong SDK probe contract.
  Both failures were corrected and the complete target list passed; no tests were disabled.
- Earlier Mac physical-machine run: native Metal host at `fcb265c`, executable
  `3beb82a5ff91a85702a6adb21e6aab4a4ae31b5abf299aaa70220731afe51b9e`,
  with the known personal module
  SHA-256 `9f3dec4ded4c7516bda6f5d9f5224f1a46af97ccbb0ccb5bb5797d3143eff0ed`
  (ABI 3 / CPU ABI 6), HLE, mods off, Smooth Motion off, 960x720, 1x scale,
  normal transitions, fresh isolated card/SRAM and settings disabled. Local fixtures
  were backed up first; original saves/preferences were not used or changed.
  At 2,400 retraces it exits normally; Opening.arc is reached at retrace 916.
  Guest WAV is 1,278,984 frames at 32 kHz; post-sink PCM is 5,145,628 bytes.
  Output reports 159,873 pushes, zero dropped pushes/frames, 453 starvation events
  and 5,089 stretch events. In the 30-32 second windows, comparison with the private
  decoded `1tale.afc` gives stereo/mono correlations 0.999997/0.999998 for guest PCM
  and 0.991281/0.991089 for sink PCM (independent best offsets 11.449906/11.217937 s
  in the reference). This identifies the expected music in the actual SDL submission
  path despite stretching; it does not prove speakers or stutter-free playback. A single UI screenshot was black and the later capture
  timed out; correct intro pictures and audible speakers are not accepted by this
  observation. Neither PCM nor an opening-resource milestone proves the whole intro.
- Counter smoke run: same host/module/settings, restored Outset `sea` room 44,
  600 retraces after state retrace 1,001, no live input. This runs while the personal
  module compiles and other testing consumes CPU; it is **not a matched benchmark**.
  Retrace intervals: p50/p95/p99 17.26/22.34/27.27 ms, 58.31/s, >50/>100 ms stalls 2/1.
  Surface-present intervals: 33.30/35.19/58.16 ms, 29.67/s, stalls 5/0. Audio drops 0,
  starvation events 175. Flush summary: 299 calls, 84,211 us total, 49,956 us max,
  explicitly includes presentation. The game remains a 30 Hz update simulation;
  these two interval streams do not change its update rate or prove an FPS gain.

- A separate bounded restored-Outset run renders Link on the bridge to the house;
  native UI observation confirms the scene and F1 menu open/close/resume. The menu
  shows Smooth Motion off. Automated tab clicks and Tab/Right do not visibly select
  Controls; no menu-tab or real-controller acceptance is claimed. Need independent
  real input to distinguish automation failure from an existing menu input bug.
  Settings remain disabled, no player card/preferences are edited, and the bounded
  run exits normally. Menu redraws near 120/s are UI redraws, not game updates.

## Sequential option-module intro checks on the physical Mac

Source `3d7b5a0`, runtime `6699be9`, temporary developer-tracing host executable
SHA-256 `d19f802f55a7f8fb680df813a7145ba884880f69ab4ab28c53410140d18da679`.
The private option-enabled module SHA-256 is
`1fe3e36cb1c589263877fe0b2c8a332d09a9ee8f8e6093839726f96a7f794e83`;
its three option-control exports and 15-option count were verified locally.
Both runs use this exact module, HLE/Metal, 960x720, 1x scale, Smooth Motion off,
normal transitions, wall pacing, fresh isolated card/SRAM, disabled saved preferences,
and `BLUEWAKE_TRACE_BGM_STREAM=1` with developer tracing compiled on.
The normal host was rebuilt afterward with tracing OFF.

| Check | Mods off (`intro-hle-dgkWuw`) | Better Wind Waker, only skip_intro_movie on (`intro-hle-NQE5F6`) |
| --- | --- | --- |
| Options log | none | skip_intro_movie |
| Native UI observation | Two different opening storyboard pages render | Link renders in the Outset opening cutscene |
| Scene trace | Opening requested at 916, completes at 14,002, play scene at 14,062 | Opening requested at 916, play scene at 1,043; story-completion marker is bypassed |
| BGM stream trace | 1tale.afc ready at 1,009, play at 1,110, state 4 at 1,111; handle clears at 14,052 after the opening | No intro-stream prepare/play event; no premature stop of an already-playing intro stream was observed |
| Normal bounded exit | 15,000 retraces, 11,339,463 guest blocks | 5,000 retraces, 3,812,700 guest blocks |
| Guest capture | 7,998,984 stereo frames, 32 kHz | 2,665,648 stereo frames, 32 kHz |
| Actual SDL-submitted sink capture | 32,208,056 bytes | 11,029,336 bytes, 5,049,957 nonzero S16 samples |
| Audio pushes / dropped pushes / dropped frames | 999,873 / 0 / 0 | 333,206 / 0 / 0 |
| Starvation / stretch events | 370 / 44,337 | 263 / 62,675 |

For mods off, the same 30-32 second comparison against private decoded 1tale.afc
identifies the expected track in guest PCM (stereo/mono 0.999997/0.999998) and
SDL-submitted sink PCM (0.985071/0.985065), with independent alignments and stretching.
With skip on, the sampled window does not identify that intro track (absolute best
correlation below 0.056); the trace never prepares/plays it and the scene has already
advanced. Other PCM still reaches SDL. This verifies the bounded Mac skip path and
sampled opening pictures, not audible speakers, pixel equivalence, controller play,
long-session stability or the affected reporter's platform/settings. The two runs
have different stop limits and are not performance comparisons. A screenshot request
at the end of the mods-off run timed out; play-scene entry there is log evidence.
The original fixture and backup hashes were read back unchanged; captures, module,
app and cards remain private ignored build files. No iPad was accessed.

## Pending checks and deliberate deferrals

- The sequential option-module Mac checks above are complete within their stated
  bounds. Hear the intro through real speakers and verify clean interruption/device
  recovery on each accepted platform with the reporter's exact settings; compare
  guest and sink audio there. No iPad use is allowed during this work;
  iPhone/Windows/Apple TV hands-on checks remain unverified.
- Save/reload UI, power-loss durability, two Windows processes, Apple recovery/
  migration/Restore/Import close, Siri/call/Music and background interruption,
  Windows HLE/LLE Restart/F11/safe-mode settings recovery, real Switch Pro/Xbox
  navigation/swaps/hot-plug all need the named platforms/controllers. Back up saves
  and preferences before those checks and install in place when applicable.
- Qualified opening-bird/Outset/busy-area matched serial performance runs need a
  quiet Mac/device, recorded thermal conditions, a verified route/state for the busy
  area and consistent settings/module/cache state. No QoS, cache, getenv, frame-slot,
  FIFO architecture or quality change is justified by the loaded-Mac smoke run.
- tvOS cloud mirroring (1.6) needs Apple TV/cloud entitlement and conflict/recovery
  design; compiling tvOS does not address purgeable Caches. Full 2.7 worker-counter/
  failure-flag race and trace-writer review remains separate from the reproduced
  worker-ID fix. External WER/minidumps for fast-fail, Linux CI and broader GPU TSAN
  coverage are not claimed. These were not required to validate the ordered fixes
  and need their own bounded regressions/platform evidence.
- Release readiness is not established; releases stay paused. Existing broader
  gameplay, progression, long-session and pixel-equivalence gates still apply.
