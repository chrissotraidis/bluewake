# PLAYER collision / initial-state frontier — 2026-09-06

Historical failed replay, superseded by `ROUTE_B_PLAYER_SAVE_INIT_2026-09-06.md`.

Last stable checkpoint `a7d00f1`. BW-P4-0126 remains active; no P4 promotion.

## Measured progress

The private PLAYER execute target now composes full original `c_cc_s.cpp`
and `d_cc_s.cpp`, replacing its construction-only object and corresponding
abort fences. Both original units compile without additional upstream edits.
Other targets retain their fences and source composition.

Strict ASan/UBSan completes the first original PLAYER update, then original
`dCcS::Move` / `cCcS::Move`: area calculation, attack/target checks, contact
checks and registration retirement. All four registration counts become zero
through the original method. No diagnostic counter reset is used. This is
Link's lone collision set, not a multi-actor contact test or full frame.

Adding a second original update reproducibly aborts at
`BW-PLAYER-INIT-AUDIO-UNQUALIFIED getLinkVoiceVowel`. LLDB proves the caller:

`execute -> changeDeadProc -> dProcDead_init -> dProcDead_init_sub2 ->
voiceStart(22) -> getLinkVoiceVowel(22)`.

The diagnostic save has zero life/max-life. Original
`dSv_player_status_a_c::init` initializes both to 12, but no original save
initialization has run in this probe. This changes the next action: qualify
original initial save state before treating the death branch as the normal
second-frame requirement. Do not silence voice methods or set life by hand.

## Next source-owned action

Connect original `dSv_info_c::init` and its complete savedata/player/memory/
zone/temporary-state owners before PLAYER phase two. The original player
config initializer calls OS sound-mode lookup and the game audio interface:
`setOutputMode -> JAIGlobalParameter::setParamSoundOutputMode -> Driver and
StreamLib output modes`. Existing audio construction currently lives only
inside the phase-three helper, so its lifetime must move earlier without
replacing construction or introducing a null receiver. Race globals and any
other retained owners must be resolved explicitly. Keep this in-memory
initialization distinct from card serialization, file selection and boot.

Then replay two updates with original collision Move, followed by actual
input, scene progression, PLAYER camera and draw toward a sustained session.
The pending Room0 request remains unprocessed; do not claim full frame
progression merely because collision registrations are retired.

## Verification / working state

- Strict first-update plus collision Move succeeds.
- Current focused two-update probe fails intentionally at the voice fence;
  second-frame acceptance and broad regression are pending.
- No upstream source or patch changed this increment. CMake and two test
  files are WIP. Do not publish this as a fully verified checkpoint.
- Logs: ignored `/tmp/bluewake-player-collision-sanitize-build.log`,
  `/tmp/bluewake-player-collision-sanitize-run.log` and
  `/tmp/bluewake-player-collision-lldb.log` (batch `-k bt` for crash stack).
- No app/Simulator launched, no save file touched, no external blocker.
