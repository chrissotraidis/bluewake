# BlueWake stability suggestions

October 1, 2026. A code review of `codex/fork-parity` plus [PR #12](https://github.com/chrissotraidis/bluewake/pull/12), done in a
cloud session with no disc, game module or test hardware. Every item below was found by reading
the source and, where possible, by a synthetic test. **None has been checked in the running game.**
Each item says what still needs a real device.

Ground rules for whoever implements these:

- Stability and save data come first. Keep each fix small, with its own commit and a synthetic test.
- 60 FPS Smooth Motion is an experimental toggle, off by default (PR #12). Don't change that.
- Code under `ref/recompcore` is the pinned runtime. To change it, commit to
  `chrissotraidis/RecompCore` branch `bluewake-next`, export the patch to `patches/recompcore/`, and
  update the pin in `scripts/builder/profiles/bluewake.sh`, `config/dependencies.lock.json` and
  `docs/status/DEVICE_BUILD.md`, as PR #12 does.
- Releases stay paused (AGENTS.md). Never commit or upload discs, modules, saves or builds.

Implementation progress and exact validation limits are recorded in
[the local October 1 follow-up](status/LOCAL_STABILITY_2026-10-01.md). This review remains
the work list; source fixes and synthetic passes do not close hardware checks.

Paths starting `GXRuntime/` are inside `ref/recompcore/`.

---

## 1. Save data (highest priority)

### 1.1 A failed save can delete the memory card (data loss, confirmed)

`GXRuntime/src/memory_card.c:205-212`: when renaming `GZLE01.card.tmp` over the card fails, the
fallback deletes the real card and renames again. If that second rename also fails, line 213
deletes the `.tmp` too. Both copies are gone, and the next launch creates an empty card. A
synthetic run with a failing `rename` shim reproduced it: `card=-1 tmp=-1`, then a fresh 40-byte
card on relaunch. On Windows, `rename` already maps to `MoveFileExA(REPLACE_EXISTING|WRITE_THROUGH)`
(`windows/compat/bw_posix_compat.c:53`), and POSIX `rename` replaces too, so the fallback only
adds risk. Antivirus, the search indexer or OneDrive briefly holding the new `.tmp` open is a
plausible Windows trigger.

- **Fix:** delete the remove-and-retry fallback. On failure, keep the old card, keep or remove the
  `.tmp`, and return an error so the game shows its normal save-failed message.
- **Test:** a shim that fails `rename` once, then check the card still holds its old bytes.

### 1.2 Saves are never flushed to disk

There is no `fsync`, `F_FULLFSYNC` or `FlushFileBuffers` anywhere in the card path. After a power
loss or forced reboot, the renamed card can be empty or partly written.

- **Fix:** in `memory_card.c` `write_container`, before the rename call `fflush` and then
  `fcntl(fd, F_FULLFSYNC)` on Apple, `fsync` elsewhere, or `_commit(_fileno(f))` on Windows.
  After the rename, fsync the folder (POSIX).
- **Device check:** force a reboot right after an in-game save on an iPhone and on Windows.

### 1.3 Two processes can overwrite each other's saves

Each process loads the whole card into memory and rewrites the whole file on every save. Both
use the same `.tmp` name and there is no lock. Two cases are confirmed from code:

- Two instances.
- Windows "Restart now", which starts the new process before the old one exits
  (`windows/src/win_settings.cpp:358-375`).

On iOS, Restore/Import replaces the card while the game is running
(`apple/ios/src/BWGameOverlay.mm:1027`, `:1212`); the next in-game save can write the old card back.

- **Fix:** hold an exclusive lock for the whole session. Use `flock(LOCK_EX|LOCK_NB)` on
  `GZLE01.card.lock`, or `LockFileEx` on Windows, and refuse a second instance with a clear
  message. Use a per-process `.tmp` name.
- **iOS:** block card writes before replacing the card, then quit (see 2.4).
- **Test:** open two card handles on one path and check that the second is refused.

### 1.4 A corrupt card makes iOS quit silently on every launch

If the card fails to open, `runtime/host/src/main.c:6884-6887` returns 1 and iOS exits with no
message (`apple/ios/src/ios_entry.m:335`). That looks like a crash loop, and a player who
reinstalls to fix it deletes their saves. Restore checks only the `DOLCARD1` magic
(`BWGameOverlay.mm:968-970`), so a truncated backup can become the live card and start that loop.

- **Fix:** add `dol_card_validate(path)` built on the existing container loader, and call it
  before every restore or import.
- **At launch:** on a bad card, rename it to `GZLE01.card.corrupt-<time>`, never delete it, and
  show an alert offering Backups or a new card.
- **Test:** validate a truncated card and one with a single flipped byte.

### 1.5 Only one copy of the card is kept

Every save replaces the only card. Backups exist only after a manual Restore or Import.

- **Fix:** before each rename, keep the previous card as `GZLE01.card.bak` (a hard link, or a
  copy), plus one copy per day for the last few days.
- **Test:** after two saves, `.bak` loads and holds the first save.

### 1.6 Apple TV stores the only card in purgeable Caches

tvOS keeps the card in `Library/Caches` (`ios_entry.m:173`, `docs/status/TVOS_BUILD.md`), which
the system can delete.

- **Fix:** after each successful save, mirror the card to iCloud key-value storage or CloudKit
  (a card is small). Restore it from there if the local card is missing at launch.
- **Device check:** a real Apple TV.

### 1.7 Smaller save-safety items

| Where | Problem | Fix |
| --- | --- | --- |
| `runtime/host/src/ipl_sram.c:36-40` | SRAM is written in place; `fwrite`/`fclose` unchecked | `.tmp` + rename, check results |
| `runtime/host/src/settings_menu.cpp:141-166` (Mac) | Settings written in place; `g_dirty` cleared even on failure | `.tmp` + rename; keep `g_dirty` on failure |
| `windows/src/win_settings.cpp:178-183` | `MoveFileExA` result ignored; `g_dirty` cleared anyway | Check it; keep `g_dirty` and back off |
| `runtime/host/src/save_state.c:53` | Save states written in place, so a failed write destroys the old state | Write `path.tmp`, rename only on success |
| `apple/ios/Info.plist.in:28-29` | The live card and its `.tmp` are editable in the Files app | Move the live card to Application Support; keep exports and Backups in Documents |

---

## 2. Crashes

### 2.1 Windows: "Restart now" crashes, and closing shows an error box

- `windows/src/win_settings.cpp:375` calls `std::exit(0)` inside a frame while the GX worker
  thread is still running. That ends in `std::terminate` (0xC0000409).
- `runtime/host/src/main.c:15395` returns 1 for a normal quit, which the Windows app reports as
  an error.

**Fix:**

1. `restart()` saves settings, sets a `restart_requested` flag and pushes `SDL_EVENT_QUIT`, the
   same way the Mac "Quit the game" button does (`settings_menu.cpp:625-627`).
2. Move the `CreateProcessW` code to a new `bw_settings_relaunch()`.
3. Call `bw_settings_relaunch()` from `windows/src/win_entry.c` only after the host has returned,
   so the GX worker is joined and the card is closed.
4. Return 0 when the stop reason is `"quit"`.

**Device check:** Restart from the menu on Windows with LLE audio on and off; there should be
no crash and the card should still load.

### 2.2 Windows: a bad saved setting crashes every launch

`settings.ini` saves restart-only choices (LLE audio, mods, options) and applies them on every
launch (`win_settings.cpp:149`, `:174`, `:860`). One bad combination means the game never starts
again. The Exact-audio crash fixed in PR #12 worked exactly this way.

- **Fix: a launch marker.**
  - Write `%APPDATA%\BlueWake\launch.pending` just before the host starts (win_entry.c, about line 634).
  - Clear it after about 600 frames of play, or on a clean exit.
  - If it is still there at the next launch, back up `settings.ini` and reset the restart-only
    settings to defaults. Force `BLUEWAKE_DSP_MODE=hle` and log `[safe-mode]`.
  - Show a short banner.
- **New flags:** `--safe-mode` and `--hle-audio`.
- **Test:** create the marker by hand and check the reset and the backup file.

### 2.3 Windows: command-line options end up saved

`bw_settings_apply_launch` copies command-line and environment overrides into the saved settings
(`win_settings.cpp:833-890`, for example `g_saved.smooth_motion = getenv(...)`). So `--scale 4` or
`--fullscreen` become permanent after any later save. `--lle-audio` is never read back, so the
menu shows Fast audio while LLE is running.

- **Fix:** keep session-only overrides in a separate struct that `save_file` ignores.

### 2.4 iOS: "Close BlueWake" calls `exit(0)` while the game is running

`apple/ios/src/BWGameOverlay.mm:1044` and `:1264` call `exit(0)` from an alert handler. At that
point the FIFO worker and render worker threads are still joinable, so static destruction calls
`std::terminate`. This is a likely crash report on every Restore/Import.

- **Fix:** close the card runtime (flush), then `_exit(0)`, or set a quit flag and let the host
  return normally.
- **Device check:** a simulator crash log before and after.

### 2.5 Pipeline-cache shutdown can hang

`GXRuntime/graphics/aurora/lib/gfx/pipeline_cache.cpp:1186-1189` sets `g_pipelineThreadEnd` and
notifies **without** holding `g_pipelineMutex`. The worker checks that flag inside a predicate
wait under the mutex (`:1003-1004`). A notify can land between the check and the block, so the
`join()` at `:1189` waits forever. It is a lost wakeup at quit or restart; rare, but it would
present as a frozen game.

- **Fix:** set the flag while holding `g_pipelineMutex`, then notify.
- **Test:** an init/shutdown loop with a timeout.

### 2.6 Windows crash reports miss most crashes

`windows/src/win_entry.c:397-423` installs only `SetUnhandledExceptionFilter` (`:623`):

- `abort()`, `std::terminate` and `__fastfail` are never reported. Aurora's fatal log calls
  `abort()` (`GXRuntime/backends/aurora/aurora_backend.cpp:156`).
- A jump to address 0 prints `?+0x0` and no stack. That is why the fork's crash log had no trace.
- The handler uses `fprintf` and `MessageBox` inside a crashed process.

**Fix:**

1. Add `std::set_terminate`, a `SIGABRT` handler and `_set_abort_behavior`.
2. When RIP is 0, take the caller from `[RSP]` and walk about 16 frames with `RtlVirtualUnwind`;
   `profile_caller` at `:271-293` already does this walk.
3. Write with `WriteFile` to the log handle.
4. Call `SetThreadStackGuarantee` at startup.

**Test:** a debug-only environment variable that calls through a null pointer.

### 2.7 Smaller crash and robustness items

- **Windows log pump.** It writes to the console before the file (`win_entry.c:112-115`). A
  console paused by QuickEdit can block stderr and freeze the game thread. Write the file first.
- **Windows hotkey hook.** `SetWindowsHookExW` is unchecked (`:630`). Log a failure, because
  without it F1/Esc never open the menu.
- **Windows session logs.** Names have one-second resolution (`:195`), so a fast restart can
  collide. Add the pid.
- **Data races ThreadSanitizer would flag:**
  - `g_shadow_frontend_failed` is a plain bool (`GXRuntime/backends/aurora/aurora_backend.cpp:63`),
    written by the worker and read unlocked by the game thread.
  - `g_core_submitted` and `g_core_rejected` (`aurora_graphics.cpp:129-131`).
  - `g_workerThreadId` (`GXRuntime/graphics/aurora/lib/gfx/render_worker.cpp:28`).

  Make them relaxed atomics.
- **Debug traces.** Trace capture can start the FIFO worker while tracing is armed
  (`aurora_graphics.cpp:647`), and both threads then write the trace. Refuse the worker whenever
  tracing is armed. That could also explain the failed GX replay capture in the stability ledger.

---

## 3. Audio, including the narrated-intro report

The intro track (`1tale.afc`) is decoded by the DSP and mixed into the normal audio DMA like
every other sound; it does not use the disc-stream path. Mac captures contain it. So missing intro
music most likely means **all** sound was lost on that platform, or the guest was told to stop it.

### 3.1 A failed audio device open means silence for the whole session (confirmed)

`GXRuntime/backends/aurora/aurora_backend.cpp:515-523` opens the output once at startup. On
failure it only logs `[audio] SDL output unavailable` and never retries; every later push returns
immediately (`aurora_audio.cpp:53`). A Windows PC with no default output at launch (a Bluetooth
headset not yet connected), or an iOS session that a call holds, gets silence until restart.

- **Fix:** retry the open on `SDL_EVENT_AUDIO_DEVICE_ADDED` and on a timer, and show the player
  that audio is unavailable.
- **Device check:** start Windows with outputs disabled, then enable one.

### 3.2 The intro-music check can't see what reaches the speaker

The WAV capture (`runtime/host/src/main.c:4690-4701`) is taken before the sink's drops:
fast-forward discard, the 250 ms no-throttle drop, and the time stretcher
(`aurora_audio.cpp:79-151`). `g_audio_dropped_count` is counted but never printed
(`aurora_backend.cpp:677`).

- **Fix:** print `dropped=` in the audio summary line.
- **Fix:** add an opt-in sink capture (for example `DOL_AUDIO_SINK_CAPTURE=path`) that writes
  exactly what goes to `SDL_PutAudioStreamData`.
- **Test:** push a known sine with discard on and with a full queue; check the drop count.
- **Device check:** compare the guest WAV with the sink capture during the intro on iOS and Windows.

### 3.3 iOS audio interruptions

`BWGameOverlay.mm:2078-2083` pauses on an interruption and resumes only on the matching "ended"
notice, which iOS often doesn't send (an app backgrounded during a call, Music taking the
session). `g_audio_playing` is never reset (`aurora_audio.cpp:154`), so a resume where SDL fails
to restart output runs silently.

- **Fix:** also resume on `UIApplicationDidBecomeActive` and check that `setActive:YES` succeeds.
- **Fix:** reset `g_audio_playing` when SDL reports the device paused or lost.
- **Device check:** Siri, a phone call, or Music.app playing mid-intro.

### 3.4 Better Wind Waker "Skip the opening movie" is untested with music

The two `skip_intro_movie` sites (`mods/betterww/options.txt:138-139`) replace instructions with
no-ops inside `dScnOpen_c::execute`, the opening scene that can call `mDoAud_bgmStop`. Unlike
every other site in the file, they don't record which instructions they replace. The option is
off by default, and the intro tests were all run with it off.

- **Fix:** disassemble both addresses from a personal DOL, record the original instructions, and
  check them against Better Wind Waker commit 4501481.
- **Device check:** run the narrated intro with the option on, with `BLUEWAKE_TRACE_BGM_STREAM=1`
  on the Mac, and look for a stream stop.

### 3.5 Ruled out

- **iOS silent switch:** SDL 3.4's default iOS category is Playback with mix-with-others, so the
  switch doesn't mute it.
- **AI volume register / disc stream:** Wind Waker doesn't use the disc-stream path.
- **DSP microcode selection:** the same on every platform.

What's still missing is the reporter's platform, build and settings.

---

## 4. Controls

### 4.1 Switch Pro A/B and X/Y are swapped (confirmed)

Aurora maps face buttons by position. Its Switch Pro table
(`GXRuntime/graphics/aurora/lib/dolphin/pad/pad.cpp:143-147`) sends the bottom button (labelled B)
to the GameCube A. iOS has a manual remap menu; Mac, Windows and Apple TV have nothing.

- **Fix:** add "Swap A and B" and "Swap X and Y" settings that swap the controller's own default
  mapping rather than assuming positions. The NSO GameCube pad has its own table.
- **Apply:** right after `PADRestoreDefaultMapping`, in Windows `apply_controller()`
  (`win_settings.cpp:304`) and in a new port-change check on the Mac.
- **Save:** as `controller_swap_ab` and `controller_swap_xy` in Windows `settings.ini`, and as
  `BLUEWAKE_PAD_SWAP_AB` and `BLUEWAKE_PAD_SWAP_XY` in the Mac settings `kKeys`.
- **Test:** a pure C helper with a unit test registered in both CMake files and in the target
  list in `.github/workflows/windows-host.yml`.
- **Device check:** a real Switch Pro and an Xbox pad.

### 4.2 Windows menu: no controller navigation, and the game still gets input

`win_settings.cpp` never enables `ImGuiConfigFlags_NavEnableGamepad`. The menu opens only from
F1/Esc. While it's open, controller input still reaches the game.

- **Fix:** in `set_menu_open` (`:342`) call `PADBlockInput(open)`. It also ignores a button still
  held when the menu closes (`pad.cpp:1722`).
- **Fix:** in `draw_menu`, enable keyboard and gamepad navigation, as the Mac does at
  `settings_menu.cpp:558-561`.
- **Fix:** let the controller's Back button toggle the menu.
- **Device check:** Link must not move while the menu is open, and the A that closes the menu
  must not reach the game.

---

## 5. Performance (measure before changing anything)

The ledger's 47.6% "waiting in `shadow_frontend_flush`" includes presents, because the drain calls
`aurora_backend_present()` itself (`aurora_graphics.cpp:796-801`). The next step is better
measurement, not a guess.

1. **Make the existing counters usable.**
   - `BLUEWAKE_GX_FLUSH_CENSUS` prints only after retrace 13,800 (`main.c:9310`), and its
     totals are never printed.
   - Make the threshold an environment variable and print a summary at exit.
   - Then record `BLUEWAKE_FRAME_TIMING` with `scripts/frame_tail.py` (p50/p95/p99), retraces per
     second, fps_watch `gx=`/`present=` waits and worker CPU %.
   - Run the opening bird scene, Outset and one busy area, on the same device, settings and
     thermal state, serially (`one_game_guard`).
2. **Thread priority (low risk).** No thread sets a QoS class. Set user-interactive QoS on the
   FIFO, render and interpolation threads (iPad efficiency cores), then measure.
3. **First-visit stalls.** Run the opening-to-Outset route cold (empty user cache) and warm,
   compare `[gx-slow] pipelines= textures=`, and if it helps, regenerate the bundled starter
   cache from that route.
4. **`getenv` in hot paths.** `getenv` runs on every guest exception (`GXRuntime/src/core/cpu_exception.c:29`)
   and every HLE context restore (`GXRuntime/src/hle/hle_core.c:97`). Cache it in a static, as Windows already does,
   and check instructions per retrace with `bench_instructions.sh`.
5. **Smooth Motion cost (experimental mode only).** It has two frame slots
   (`lib/gfx/common.cpp:64`) and re-encodes each in-between frame. Try three slots, measured
   only with Smooth Motion on. Don't make it the default.
6. **Larger worker changes** need a pixel-equivalence check first. These include splitting the
   FIFO worker so the game waits only for guest-memory reads, or letting translation lag one frame.

Don't report an FPS gain without matched before/after runs. Keep the 30 Hz game rate separate from
60/120 Hz displayed frames.

---

## 6. Tests that need no game

- **ThreadSanitizer.** Build the runtime's existing `GXRuntime/graphics/aurora/tests/render_worker_test.cpp`
  with `-fsanitize=thread`; it should flag `g_workerThreadId`. Add a pipeline-cache init/shutdown
  loop for 2.5.
- **Save regressions.**
  - The rename-failure shim (1.1).
  - Two handles on one card (1.3).
  - Truncated and bit-flipped cards (1.4).
  - `.bak` rotation (1.5).
- **Mac launcher test.** `tests/test_mac_launcher.py` fails on Linux (exit 127; it is macOS-only).
  Skip it off macOS, so the Python checks can run in a Linux CI job.
- **Linux CI.** A source-only Linux build of the host and its tests is the base for future Linux
  support and catches portability bugs early. In this cloud session it stopped at Aurora's
  abseil download (blocked by the network policy here), not at a code error.

## Hardware and private-input checks nobody can do from the cloud

- The narrated intro heard through speakers on each platform, with the reporter's settings.
- Windows gameplay: Exact audio, F11, Restart, controllers.
- Save durability under forced reboot.
- Apple TV storage.
- Matched performance runs on the opening, Outset and one busy area.
