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
  waiting for in-flight writes before replacement; failure resumes dispatch. Runtime
  patch 0117. iOS concurrency and Windows two-process behavior need platform checks.
- 1.4: read-only dol_card_validate uses the complete loader and its checksums;
  restore/import reject invalid replacements and retain them. Startup recovery on
  Apple offers valid local Backups/.bak or a new card, preserving the original at
  a unique .corrupt path only after the player chooses. Unreadable existing cards
  are never mistaken for missing cards. Loader size is bounded and short-read
  cleanup closes the stream. Truncation/bit-flip sanitizer tests pass and prove
  rejected bytes are unchanged. Runtime patch 0118. UIKit recovery needs simulator
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
  UI. Synthetic scale/fullscreen/LLE/Smooth Motion and option isolation test passes
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
  checks are registered with a timeout, but not yet run locally. __fastfail bypasses
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
