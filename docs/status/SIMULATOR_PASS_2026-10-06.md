# Targeted iPad simulator validation — October 6, 2026

Chris requested simulator-only validation instead of using the connected hardware iPad.
No physical device was queried, installed or launched in this pass.

## Build and method

- Host source: `a95ec788d79a807d047db865ae220d8d5be9d237`.
- RecompCore: `d5b92fbb7d1d2ccd05d72d448e512dc06120efc9`, clean checkout.
- Release/arm64 iOS simulator host built with `cmake --build build/ios-sim --target BlueWake -j 8`.
- Existing BlueWake iPad Pro 12.9 simulator, iOS 26.5, running on the M3 Max Mac.
- Metal rendering, Smooth Motion off, no game mods or replacement textures. Private prepared
  disc/module and copied saves; simulated input. No physical controller validation.
- Full simulator Documents/Library backup before in-place installation. Test settings, cards,
  SRAM, states and caches use separate scratch paths. The three pre-existing card/SRAM/ImGui
  files match their backup hashes after the tests. Unrelated simulators were left alone.

The simulator uses the existing documented simulator dependency/module retagging approach.
It exercises the UIKit/shared-host/Metal path on the Mac; it does not establish physical iPad
performance, speaker routing, touch behavior or Windows/D3D12 results.

## Pictobox (#13)

One executable, same private regular-camera checkpoint and two different views in one process:

- Fallback off: second preview incorrectly repeats the first view of the bridge; stale-texture
  diagnostics identify the 152x104 texture at guest address `00A8B6A0`.
- `BLUEWAKE_CACHE_FLUSH_FALLBACK=1`: second preview correctly changes to the ladder/sea view.
  No stale-texture diagnostic appears (269 with the flag off, zero with it on). The checkpoint restores without FORCE, with no missing
  or mismatched host fields. First captures are pixel-identical across the regular-camera pair;
  second captures change only with the fallback enabled. Captures at retraces 5000 and 6501
  were inspected. A separate assisted Deluxe-camera fixture also produces the distinct second
  view in color with the fallback enabled; no Deluxe-off comparison was repeated.
- Scripted left-stick selection and A confirmation accept the photo; the camera reports two
  remaining slots and gameplay resumes with a photo count of one. This proves the simulated
  logical input path, not a hardware controller or touch gesture.

The first combined save automation pressed START while the camera prompt was open. Separating
photo confirmation, putting the camera away, and opening the normal Save menu reached the
save-complete/continue-playing prompt. The scratch card has valid checksums, a regular camera
item (`0x23`) and photo count 1, compared with no camera/count 0 in the initial card. A fresh
process loaded this card through file select without a debug checkpoint. The item menu shows
the camera with a count of one. The assisted checkpoint had granted the item and live X cache
without setting the persistent X assignment; the first direct X attempt after reload therefore
did not open it. This is a fixture limitation, not evidence of remapping or persistence failure.
After assigning the camera to X through the item menu and switching to album mode, the fresh
process displays the saved ladder/sea photograph and reports one recorded pictograph. Thus
regular-photo normal save/relaunch/reload passed with the fallback enabled. Deluxe persistence,
physical controllers and touch input were not tested. The cursor script needed a short horizontal
pulse to avoid skipping the camera slot; this was corrected in the private test sequence.

## Dungeon map (#74)

A copy of the existing community Dragon Roost save, already adjusted to restart in the cavern,
was imported into a scratch card. Starting normally from the title, the test loaded slot 1
and opened the large dungeon map. The captured 1F map visibly contains its grid and green
visited-room shape; the reported blank map is not present in this scene. This fixture has not
collected the dungeon map, so it does not establish all unvisited rooms/all floors.

Both experimental host flags were off. `unsupported_texgen=0` and `tev_stages_over=0` in the
renderer shutdown summary. This adds simulator visual evidence for the already-merged runtime
repair; it does not put that repair into the published 0.5.0 package or confirm Windows behavior.

## Intro audio (#97)

Two fresh scratch profiles, 3600 retraces each, same host/settings and one changed flag:

| Deferred DVD completion | Last ten seconds of captured stereo PCM | Stream evidence |
| --- | --- | --- |
| Off | 0 of 640000 samples nonzero; peak 0 | `1tale.afc` enters playing at retrace 1959, then idle at 1961 with stale pending state |
| On | 639118 of 640000 samples nonzero; peak 12178 | `1tale.afc` enters playing at 1953 with `dvd_pending=0` and remains active |

Both reached the history intro normally. Each capture spans about 60 seconds including startup.
The result agrees with the earlier Mac/physical-iPad captured-output evidence. It is not a
subjective listening test, a complete intro/later-scene test, or proof that every #65 cue is fixed.
The earlier full-track/next-event run was not repeated.

## Remaining gates

Both experimental host flags remain off by default. No runtime code, setting default, release,
issue state or official platform-support status changed. Keep Windows checks in
[WINDOWS_TASKS.md](../WINDOWS_TASKS.md); physical iPad checks are deferred per Chris's instruction.
Private app/module, saves, checkpoints, audio and captures remain local.
