# Stability and slowdown plan — October 3, 2026

BlueWake now contains Elliott Tate's Wind-Waker-Recomp work through his v0.4.0 release, so reports
filed against his Windows builds (0.2.1 to 0.4.0) mostly describe BlueWake's code as well: the
runtime, renderer, audio and camera are shared, and Apple builds run the same runtime with Metal.
This plan sorts those reports, records what the attached session logs show about the "random FPS
slowdowns", and sets the order of work. The [goal loop](../../GOAL_LOOP.md) runs it.

## What the logs show about slowdowns

Three logs are attached to Wind-Waker-Recomp issues (#5, #19, #27). The longest, 104 minutes on a
Ryzen 9 5900HX laptop with 0.4.0, summarized with `scripts/triage_session_log.py`:

- 97 seconds fell below the selected FPS. 91 of them were the game itself running below full speed
  (median 89%, lowest 67%). 89 of the 97 were at the Forsaken Fortress exterior (stage `MajyuE`).
- In 83 of the 91, the GX worker, the thread that converts the game's drawing commands for the GPU,
  was at least 85% busy (mostly above 92%) and the game waited on it roughly 400 ms of every second. The GPU waited about 1 ms.
  The slowdown is CPU work on that thread, not the graphics card, which matches the #6 comment
  ("GPU usage is extremely low, but the FPS counter says 60").
- Smooth Motion lowered its in-between frames 71 times. After a quick relapse it waits up to
  120 seconds of calm before coming back, so a short overload can look like "30 FPS for a while,
  then 60 again".
- 1,587 slow GX batches happened while pipelines compiled or textures loaded: compile and upload stutter, a
  second, separate cause.

Two mechanisms therefore need separate fixes: real overload of the GX worker in heavy scenes, and a
pacing rule that drops in-between frames for one hitch (an 8-frame average) and then holds them off.
The other logs show the Windows audio crash at address zero that BlueWake already fixed (#5) and a
short Pictobox session whose black screen left no error in the log (#19).

## Elliott's newer work, not yet in BlueWake

After v0.4.0, Wind-Waker-Recomp's `windows-release` gained about 20 commits (`9921398` to
`13355b8`, October 2) aimed at exactly these problems. His measurements, not yet repeated here:

| Change | His runtime patch | Result he reports |
| --- | --- | --- |
| Dawn device lock | 0121 | Crash after loading a save state: 3 in 24 runs to 0 in 80. He notes the Mac needs it too. |
| Slow-game detector: median of 60 frame gaps, ignores gaps of 150 ms+, two slow windows | 0122 | Single hitches no longer drop the in-between frames |
| Pipelines compiled on several threads, 513-pipeline seed, "compile shaders before playing" | 0123 | Seed compiles in about 1.1 s instead of 4 s |
| Ubershader for draws whose pipeline is still compiling (D3D12 only) | 0128-0129 | Skipped draws after warm-up cut by 80-96% |
| GX worker: attribute arrays found once, vertex constants reused | 0124, 0127 | GX worker CPU per frame down about 12% |
| Smooth Motion for cloth, sails, colours | 0125-0126 | Flags and sails blended (needs a rebuilt module) |
| Training tours ten places; app PGO and ThinLTO; tiered compile | builder | 425 instead of 288 functions trained; GX worker CPU 16.9 to 14 ms; build 39 to 15 minutes |
| Nine more certified native functions | builder | Outset on four E-cores 37.5 to 44.5 game FPS, with the above |

BlueWake's own runtime patches 0121-0139 use the same numbers for different changes (save safety,
the Pictobox fix, post-texture state), so imports are renumbered. BlueWake's card safety and Pictobox
patches are not in his branch; they must survive every import.

## The reports, applied to BlueWake

| Report | Applies to BlueWake | Status | Next step |
| --- | --- | --- | --- |
| Slow bird scene (#6), slow abduction (#23), Forsaken Fortress dips (#27 log) | Yes, shared runtime | Not measured on BlueWake in these scenes | Measure on Mac and iPad, then import the batch above |
| Missing music in intro and scripted scenes (#1, #12) | Likely, shared audio | Not reproduced | Reproduce on Mac with audio capture and the audio logs |
| Camera left/right flips in water (#24) | Likely, shared camera code | Suspect: the stick camera hands control back to the game's C-stick in some modes, which turns the other way | Mac stick test while swimming |
| Dungeon map drawn wrong (#25), pirate flag without texture (#20) | Likely, shared renderer | Not reproduced | Reproduce on Mac from saves; compare captures |
| Soft lock after a Moblin falls (#27) | Unknown | Log attached | Try to reproduce; may be the original game's behaviour |
| Pictobox black screen (#19, BlueWake #13) | Yes | Fixed in BlueWake, passes on Mac | Windows confirmation |
| Crash at launch with Exact audio (#5) | Was | Fixed in BlueWake | Windows confirmation |
| Freeze after disc import (#16), exe does not open (#28) | Possibly | Windows only, no BlueWake data | Elliott's Windows testing; ask reporters for session logs |
| Button layout and remapping (#2, #14), 8BitDo modkit (#8) | Partly | BlueWake has A/B and X/Y swaps; no full remap | Remapping as a feature |
| Disable sprint and jump (#22) | Answered | BlueWake's "Jump and Run" is off by default | Point the reporter to the setting |
| Linux, PAL, Switch, Android, Wii U UI, portable mode, ultrawide, language (#3, #18, #7, #9, #26, #4, #10, #11, #21, #29) | Feature requests | | Migrate as feature requests |

## Logging: what is there and what is missing

Already written by the shared host on every platform: a once-a-second `[perf]` and
`[fps]` line; `[fps-dip]` for each second below the selected FPS, with the reason, game
speed, waits, each worker's CPU and Link's stage, room and position; `[interp-pace]` when Smooth
Motion changes; `[render-slow]` and `[present-slow]`; crash reports. iOS shares the log from
Help & Feedback; Windows keeps the newest eight in `%APPDATA%\BlueWake\logs`.

Missing or misleading:

1. `[gx-slow]` fires for every batch over 20 ms: 33,712 lines in the 104-minute log, written from
   the overloaded thread itself. Aggregate it per second; print single batches only at 50 ms or when
   a pipeline or texture was made.
2. `[fps-dip]` says "game below full speed" without naming the saturated part. Add the
   classification the triage script makes: game thread, GX worker, GPU/presentation, or compiling.
3. Settings are not beside the dips. Log Smooth Motion steps, render scale, HD textures, mods, present
   mode and ubershader at start and on every change.
4. No end-of-session summary. Add one `[perf-summary]`: dips by cause and stage, Smooth Motion
   drops, worst frames, shader compiles.
5. iOS logs show "macOS" and an unknown CPU. Log the device model and OS. (Thermal state is
   already in the iOS `[fps]` line.)
6. A capped module-call trace is labelled `[panic] vcall-after` in normal sessions. Rename it.

## Order of work

1. **Logging** (items above), checked with a scripted stall and a real heavy scene so each cause is
   named correctly, and the log stays small.
2. **Measure before changing**: fixed scenes on the Mac and iPad, original 30 Hz and Smooth Motion
   60: Outset, the opening bird, Aryll's abduction, the Forsaken Fortress exterior, open sea. Record
   game speed, dips by cause, frame-time tails and audio for each.
3. **Import Elliott's batch** as focused, credited runtime PRs on `bluewake-next`, in this order:
   device lock, slow-game detector, pipeline threads and seed, GX worker changes, Smooth Motion cloth
   and colours, wider training and tiered compile, natives, then the ubershader (D3D12 first; Metal
   only after its own captures). Each: regression tests, Mac capture comparison, Windows CI, a note
   on whether old modules still work, and the step-2 scenes again.
4. **Specific bugs** from the table, each reproduced before it is changed.
5. **Windows confirmation** through Elliott's [checklist](WINDOWS_ACCEPTANCE_2026-10-04.md), extended with the
   new batch and the slow scenes.

Defaults stay: original 30 Hz logic, Smooth Motion off, experimental 60 Hz off.

## Progress

**Step 1, logging (October 3, branch `codex/stability-logging`, runtime `d55ee01`).**

- Elliott's `BLUEWAKE_TEST_STALL` hook is imported with his authorship (his `a6f8ea9`,
  which also raises the Windows process priority).
- `[fps-dip]` now ends with `pipelines=N cause=... reason=...`. Causes: `gx-worker`,
  `shader-compile`, `gpu-present`, `render-worker`, `interp-helper`,
  `game-thread` (the game ran slow without waiting on the others), `unclear`.
- `[smooth-motion]` records Smooth Motion at start and on each change; `[device]` gives
  the Apple model and OS version; `[perf-summary]` every ten minutes and at exit gives slow
  seconds by cause and place; the capped REL trace is now `[rel-vcall]`.
- Routine 20-50 ms GX batches are summed into `[gx-slow-sum]` every ten seconds (runtime
  patch 0140); single lines remain for 50 ms or more.
- `scripts/triage_session_log.py` reads all of these, and still reads older logs.

Checked on the M3 Max with the frozen personal module, a copied Windfall save and Smooth Motion
60 (the machine was busy with other builds, so these are not performance numbers): two scripted
holds of 150 and 900 ms were each logged as `cause=game-thread` (speed 85% and 48%), and one
natural GX hitch as `gx-worker`; the summary line and triage report agree. In the same minute
Smooth Motion dropped its in-between frames three times: twice for the scripted holds and once for
a single 64 ms GX batch. The single-hitch drop described above therefore happens on BlueWake too.
With the pipeline seed removed, Metal compiled 125 pipelines without a single slow second: on the
Mac a missing pipeline skips its draw rather than holding the game, so `shader-compile` is
covered by the regression and needs the Windows run to be seen live. The copied save and seed were
unchanged after each run.

Merged October 3: BlueWake [#39](https://github.com/chrissotraidis/bluewake/pull/39) and RecompCore
[#5](https://github.com/chrissotraidis/RecompCore/pull/5); Windows CI builds the host and passes the
regressions, including the new causes.

**Step 3 started: the first two imports.** Elliott's slow-game detector (his `0bb1fef`, patch
0141 here) and his Dawn device lock (`31401e5`) apply cleanly with his authorship, and the
Smooth Motion and FPS regressions pass on the Mac. A before/after comparison was not possible:
the machine reached a load average of about 208 on 16 cores (a VM and other builds), and four
alternating lock-off/lock-on runs of the same host were all slow (13 to 69 slow seconds a
minute, against 2 to 3 earlier). Under that load the device lock runs looked worse, but the
lock-off runs were slow too, so nothing is concluded. The detector went up as a draft (measured on the iPad below); the device
lock waits on its own branch (`codex/bluewake-device-lock-candidate`) because on Metal the GX
worker may take Dawn's lock more often than on D3D12, and that must be measured on a quiet Mac
or the iPad before it lands.

**The detector, measured on the iPad.** The Mac stayed overloaded (load average up to 380), so
the comparison ran on the authorized physical iPad Pro M2 (iPadOS 27.0), unaffected by the Mac's
load. Two app-only builds, `main` `55bd3e7` (runtime `d55ee01`) and the PR branch
`2d6a590` (runtime `3f16e46`), were each combined with the same completed personal module,
signed with the existing development identity and installed in place after a backup of the
app's Documents and Library (7,167 files). Each ran twice on a copied save in an isolated folder,
with Smooth Motion 60 set for that launch only and holds of 150, 64, 150 and 80 ms ten seconds
apart:

| Build | Drops of the in-between frames | Seconds without them, after the first hold |
| --- | --- | --- |
| Before | 4 per run: 2 while loading, 1 after the 150 ms hold, 1 after the 64 ms hold | 35 of 40, both runs |
| After | 0, both runs | 0 of 40, both runs |

In the old build each later hold arrived before the calm period ended and restarted it, so a
hitch every ten seconds kept Smooth Motion off for the rest of the run. That is how players can
sit at 30 for minutes, as in the 120-second penalties in the Forsaken Fortress log. Afterwards the
build the iPad had was reinstalled, all ten save and settings files matched the backup byte for
byte, and BlueWake was relaunched normally. The four small test folders remain in the app's
Documents.

Two logging gaps showed up in these runs:

- **The FPS count reads 60 with Smooth Motion off.** Each game frame is presented twice, so the iOS
  `[fps]` line and the on-screen counter stay at 60 while the picture moves at 30. That matches
  the #6 comment ("FPS counter says 60 still"). `[fps-dip]` only fires below 95% of the target
  shown, so it missed these seconds. Next: report a second whenever Smooth Motion is on and fewer than
  90% of game frames were interpolated, and show the real state in the overlay.
- **Time held for a menu or the background counts as a frame.** One player session's `[perf]` line
  reported a 2,213-second worst frame after the app sat in the background. Next: have the runtime
  count held time so `[perf]` and `[fps-dip]` leave it out.

Merged October 3: BlueWake [#40](https://github.com/chrissotraidis/bluewake/pull/40) and RecompCore
[#6](https://github.com/chrissotraidis/RecompCore/pull/6).

**Both gaps fixed (runtime `886e138`, patch 0142).** `[fps-dip]` now counts a second whenever
Smooth Motion is on and fewer than 90% of game frames were interpolated, and names the case where
none were `cause=smooth-motion-paused`. The runtime counts held time; `[perf]` leaves it out of
gaps, hitches and the rate, and `[fps-dip]` skips a second with more than 0.1 s held. On the
overloaded Mac, a one-minute run logged 17 such seconds (for example "shown=60.9
interpolated=0/30") that the old rule could not see, beside 24 seconds of real GX-worker overload.
The on-screen FPS counter still counts repeated frames; showing the real state there is next.
