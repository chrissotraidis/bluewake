# Technical debt and known issues

**The ranked work list is now [PRIORITIES.md](PRIORITIES.md).** This page keeps the investigation notes
behind it, the support and delivery debt, and the logging priorities. The priority table that used to be
here moved there on October 7, 2026, with the issues opened since. Today's evidence is in
[the October 7 triage](status/TRIAGE_2026-10-07.md) and
[the October 7 fix pass](status/FIXES_2026-10-07.md); the October 6 notes below still apply.

Owner: Chris. Put detailed findings in dated `docs/status/` records and link them from PRIORITIES.md.
A hypothesis, passing CI and a reporter-confirmed fix are different states. Do not count a request for
logs as a completed investigation or repeatedly ask for evidence already supplied.

## Investigation notes: October 6

### Flickering: inspect the release delta first (#136)

0.5.0 includes donor rendering changes imported in #50: transform-state reuse (patch 0152),
water/indexed-mesh UV and HUD interpolation (0153), replacement-texture mip sampling (0154),
and restricting vertex blending to written meshes (0155). They are candidates, not diagnosed causes.
Start with the same sea/cloud view and no texture pack at original 30 FPS versus Smooth Motion 60/120.
If only intermediate frames fail, inspect matching and UV wrap/blend; if base frames fail too,
inspect transform reuse/depth and texture sampling. Only test HD mip behavior with replacements enabled.
The original 0.4.0 download came from Wind Waker Recomp; BlueWake's v0.2.0 is not a valid proxy baseline.
The dungeon-map patch 0157 landed after 0.5.0 and cannot have caused this reported regression.
The source comparison also includes pixel-colour interpolation, cloth blending and the D3D12-only
ubershader fallback; see [the October 6 investigation](status/TRIAGE_2026-10-06.md).
The follow-up Mac control used the regular host capture path with interpolation off: all 13 common
retrace captures are pixel-identical to the on run. Six sampled intermediate-frame pairs show motion
without obvious cloud disappearance. This is a limited Metal/title-view result, not a negative test
of the NVIDIA sea report or a historical 0.4.0 comparison. Existing bounded capture/trace facilities
are sufficient for the next probe; do not add continuous per-draw logging before locating a bad frame.

### Audio: separate file presence, stream lifetime and audible output (#65/#97)

The [Windows 0.5.0 log](https://github.com/chrissotraidis/bluewake/issues/97#issuecomment-5996307585)
shows `Audiores/Stream/1tale.afc` preparing and reaching state 4 at retrace 2081, then state 0 at
2083, roughly 25 ms of wall time later. This is stronger evidence than a missing-file guess.
The `cues=1 sounds=1` demo summary belongs to `sea_T`, before name input; it does not validate the
history intro. A mixed-output peak can also be ambience with missing music. Trace the stop/read/decoder
path and annotate short-lived playback; do not declare audio fixed from a state-4 sample or cue counts.
The Mac M4 attachment lacks the detailed diagnostics and exact source revision, so its report is
relevant but not a matched reproduction. Earlier option testing changed several variables at once.
Disc file presence alone does not establish successful reads, correct decoding or sustained playback.
The first diagnostics change extends stream-state lines with stop/play flags, decoded/playback sample
counts and DVD/buffer state, and teaches the triage script to flag observed short playback spans.
A bounded Mac run that skips title music reaches and retains playing state. The next pass reproduced
the failure by waiting at the title: the host completes the asynchronous read callback inline, before
the caller sets its pending flag. The flag remains set, blocking the next stream's initialization.
The opt-in deferred-completion candidate now keeps the intro playing on Mac and restores nonzero
captured audio. A bounded follow-up reaches the track's full decoded sample count and the next Outset
scene; later bird cues and a subsequent streamed track remain untested. It handles ordinary async
archive reads through the same queue and preserves accepted
work in save states. A physical M2 iPad matched pair now reproduces the silent intro with the flag
off and sustained intro output with it on (3600 retraces each). A later listening launch also logs full intro-track completion and the next sea event.
This checks captured output/stream state, not speaker quality or all later music. It remains off by default: Windows hardware, later transitions
and broader loading checks are unverified. Keep #65's other missing cues separate. [Reproduction and candidate limits](status/TRIAGE_2026-10-06.md#deferred-completion-candidate).

### Performance (#137/#59/#86)

The [new Outset log](https://github.com/chrissotraidis/bluewake/issues/137#issuecomment-6000997715)
has zero new pipelines but 70/83 watched intervals below the requested display target. Of those,
35 say `game below full speed`, 23 `frames not interpolated` and 12 `presents late`. The latter
35 retain 98–102% game speed: all 70 must not be described as game slowdowns. Cause classifications
are mixed: game thread 27, GX worker 8, Smooth Motion paused 21, unclear 14. The log confirms native
accelerators are on and used. First compare the same Outset camera with Smooth Motion off versus
60/120; retain settings and warmed caches. Do not prescribe lower resolution or copy the Linux
optimization set, which already matches Windows. Counters identify where to profile, not proof of
a shared root cause. [Breakdown and next probe](status/TRIAGE_2026-10-06.md#performance--linux).

### Controller dead zone (#138)

Source and local GZLE01 inspection confirm the default host cutoff stacks with the guest's own clamp.
The first positive axial value is 31 at the host and 16 after the guest subtracts 15, or 22% of its
maximum 72. This matches the reported jump. Axial saturation is around 68% of SDL range, not the
reporter's estimated 56%, because the guest subtracts the dead zone before limiting the value.
The host backend does not call Aurora's separate PADClamp implementation; do not change that unused
clamp expecting it to fix BlueWake. [Input-chain evidence](status/TRIAGE_2026-10-06.md#controller-dead-zone-138).
No input behavior changed in this pass; a candidate curve still needs controller testing.

### Linux support (#107/#56)

Latest [patch/log review](status/LINUX_REVIEW_2026-10-06.md): the full log is now available,
but still ends in an allocator abort. The proposed patch applies to current runtime source;
its pinned-build integration, post-fix exit evidence and manual checks remain pending.


[Follow-up sent](https://github.com/chrissotraidis/bluewake/pull/107#issuecomment-6006055403): keep one PR,
attach the full log, CPU, commit and settings; diagnose shutdown `double free or corruption (!prev)`;
pass Linux and Windows checks; review main/pin/builder parity; check a packaged native build with a short
launch, save/reload, controller, settings, Outset/sailing, dungeon-map and clean-quit pass. Run the release
audit before distribution. A quoted performance summary is not a completed hardware gate.
The source-only CI runs for `9bec954` were reviewed and approved; audit and attribution passed.
Linux then failed before host compilation because SDL's XTEST dependency is absent. The
[specific follow-up](https://github.com/chrissotraidis/bluewake/pull/107#issuecomment-6007552494)
requests `libxtst-dev`, current-main integration and the shared DVD queue test in Linux CI.
Official support follows accepted implementation and hardware/package evidence, with Chris approving
publication. No date promised. Requested an optional gameplay clip and permission to reuse it publicly;
a promotional recording is not a merge gate. No Discord message was posted.

## Support and delivery debt

- #126: original reporter confirmed Molgera's floor and closed the issue; retain as a renderer regression check.
- #92: Spanish-text workaround confirmed; disc-size question remains informational, outside this bug loop.
- Discord-only mouse rebinding: implemented in both menus, checked on Mac; Windows physical mouse test remains.
- Tingle Tuner: unsupported and documented; implementing GBA communication is outside this stability loop.
- PRs #46/#100: Windows PadMint and iPhone-module builder paths remain parked, not superseded by a ready-made download.
- PRs #89/#106: window placement/FPS position depend on shared runtime review and remain separate concerns.
- Avoid stale `fixed-in-main` labels implying an unreleased fix when it shipped in 0.5.0; reconcile labels in a support pass.
- [Windows checklist](WINDOWS_TASKS.md) distinguishes shipped checks from changes after 0.5.0. A new release must
  include the source/runtime pin, package audit, hashes and accurate platform testing limits.
- Raw logs can contain personal paths. Keep them and all game inputs out of Git; commit only short relevant findings.

## Logging priorities

1. Make short-lived streamed playback visible, including track, start/end retrace, state and scene. Never label
   general sound activity as proof of audible music; distinguish an intentional skip from an unknown stop.
2. If rendering remains unreproduced, record bounded per-scene changes in dropped/unresolved draw counters.
   Avoid per-draw disk logging and record enough settings/build context to compare releases.
3. Reuse existing per-second performance and shutdown summaries; add a probe only for a specific unanswered
   question. Remove or gate expensive diagnostic traces. Missing diagnostics must be reported as unknown, not zero failures.
