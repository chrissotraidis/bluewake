# PadMint Python 3.9 player check — October 29, 2026

[Issue #496](https://github.com/chrissotraidis/bluewake/issues/496) reported a Mac build failure in the default
`combined-v1` module preparation. Apple's Python 3.9 does not support the `newline` argument to
`Path.write_text`. [PR #499](https://github.com/chrissotraidis/bluewake/pull/499) changes the four writes in
`native_game_math.py` and `native_skin.py` to `Path.open(..., newline="\n")`, preserving explicit LF output
and atomic replacement. CI now runs their existing regression tests with `/usr/bin/python3`.

## What passed

The tested code was BlueWake `830449afe8526c77ac7532f647e4a556db5d2507`. PadMint's current `main` was
`d6f0f59e7f9376b0c7c85607301b8ac808485cd0` (0.5.24). Its `scripts/player-check.sh` ran on an Apple Silicon Mac,
using a clean copy of that BlueWake checkout, an empty PadMint folder, the compatible published 0.9.0 iOS
app and two build jobs. Apple's Python 3.9.6 ran PadMint; `/usr/bin` was first in `PATH`, and the training
Python child processes were checked to use Apple's 3.9 interpreter too. An earlier stopped run that selected
a newer child interpreter is excluded from this evidence.

Run `a837156afadc4b878fc8bdb6f6fdf91a` completed with recorded exit code 0 and the player script's success
message. PadMint measured **8,775.93 seconds** from build start to `build_completed` (2 hours 26 minutes).
This is build elapsed time, not a game performance measurement.

- Default game-math and skin preparation completed; both training and final iOS compilation completed all
  860 steps. Headless local training admitted player control at retrace 20,255, completed its 23,000-retrace
  route normally and produced the merged profiles. Its receipt says `performance_verified: false`.
- Module/app signing and signature verification completed. The final record says `status: completed`,
  `checkout_check: before-record-passed` and `package_validation.check: minimal-ipa-structure-and-provenance`.
  Its iOS slice has minimum OS 17.0 and a defined scene-configuration callback; `runtime_launch: not-tested`
  and `backend_validation: not-established-by-runner` remain explicit.
- The 140,091,537-byte personal IPA passed ZIP integrity checking. Its recomputed SHA-256 matched the final
  record, and the embedded module's SHA-256 matched `BuilderProvenance.json`. Provenance identifies the
  tested commit, `source_modified: false`, local training and `containsTranslatedGameCode: true`.
- The separate, unmodified 0.9.0 app used as the base matched its release checksum and passed PadMint's
  content audit with zero address-named functions. This is the app without the player's module.
- All reported PR checks passed, including the [macOS audit](https://github.com/chrissotraidis/bluewake/actions/runs/40828984461),
  [Windows host](https://github.com/chrissotraidis/bluewake/actions/runs/40828984403),
  [Linux host/AppImage](https://github.com/chrissotraidis/bluewake/actions/runs/40828984429), native-pool sanitizers
  and attribution checks.

The personal IPA's **publication gate is `FAIL`**, as expected: it contains translated game code and
793,618 address-named symbols. Its publication label is `personal build, not publishable`; passing the
private package checks does not make it a public release artifact. Logs, receipts and the personal output
are retained locally and are not included in this record or uploaded. The largest observed scratch size
was 19.88 GiB; free disk was about 50.5 GiB at completion. These are observations, not a measured peak or a
new minimum-space requirement.

## Remaining acceptance

This verifies the Python 3.9 build fix through the fresh-player route. It does not establish iPhone/iPad
installation, launch, gameplay, audio, controller behavior or performance. The reporter has not confirmed
the fix. This work has not changed the public 0.9.0 release, and this documentation follow-up does not
change the code tested above.
