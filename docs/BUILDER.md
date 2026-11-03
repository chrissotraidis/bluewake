# The Builder

The Builder turns a player's own game disc into their own app, on their own Mac. Nothing it produces is
published: the game code is translated from the player's disc during the build and stays on their Mac
and device. This is how BlueWake is distributed. Players get the source and build their personal IPA
themselves ([Build your own BlueWake](BUILD_YOUR_OWN.md)).

```sh
scripts/builder/build.sh "/path/to/The Legend Of Zelda The Wind Waker.iso" --ipa build/BlueWake.ipa
```

`scripts/ios/build_device.sh` is the same command with the BlueWake profile preselected; it takes the same
options. `--help` lists them.

## Pipeline and profile

The Builder has two parts:

- **The pipeline**, `scripts/builder/build.sh`, is the same for every game. It parses options, checks the
  Mac's tools, runs the game's steps in order with a log per step, embeds the game module in the app,
  signs it (ad hoc, or with the player's identity and provisioning profile), writes the unsigned IPA,
  and installs with `devicectl` when asked. It refuses to write an IPA anywhere inside the repository
  that git could commit.
- **A profile**, `scripts/builder/profiles/NAME.sh`, holds everything specific to one game: which disc it
  accepts, the pinned translator and runtime, how the translated code is generated and checked, the mods,
  and the app target. `--game NAME` selects it; the default is `bluewake`.

The steps are:

| Step | Who | What |
| --- | --- | --- |
| 1 tools | pipeline + `profile_check_tools` | Xcode, iOS SDK, CMake 3.25+, Ninja, plus the game's extras |
| 2 dependencies | `profile_dependencies` | Fetch pinned sources (commit hashes, checksums) |
| 3 extract | `profile_extract` | Read the disc; refuse the wrong game or revision |
| 4 translate | `profile_translate` | Translate the game's code to C |
| 5 generate | `profile_generate` | Assemble the source to compile; compare it with the verified digest |
| 6 mods | `profile_mods` | Optional code mods (skipped with `--no-mods`) |
| training | `profile_train` | Default (`--no-train` skips): local instrumented Mac build and playback; records a private game profile |
| 7 compile | `profile_compile` | Compile the game module (the long step); set `module` |
| 8 app | `profile_build_app`, then pipeline | Build the app, set `app`; the pipeline embeds and signs |
| 9 package | pipeline | `--ipa` and `--install` |

`--source-only` stops after step 5, so a player can check their disc and tools in minutes before the long
compile. Compatible compilation work and completed matching profiles are reused. Interrupted training
playback starts again; source-only does not exercise that stage.

## Writing a profile for another port

A profile is a shell file sourced by the pipeline. It sets:

| Variable | Example (BlueWake) |
| --- | --- |
| `PROFILE_NAME`, `PROFILE_TITLE` | `bluewake`, the game title and disc revision |
| `PROFILE_APP_NAME`, `PROFILE_BUNDLE_ID` | `BlueWake`, `dev.bluewake.BlueWake` |
| `PROFILE_MODULE` | `gGZLE01_recomp.dylib`: the file name the app loads from `Frameworks/` |
| `PROFILE_DEFAULT_OUT` | `build/device` (must be git-ignored) |
| `PROFILE_HAS_MODS` | `1` or `0` |

and defines the required `profile_*` hooks in the table above. `profile_train` is required unless
players always pass `--no-train` or `--no-pgo`. Hooks may use the pipeline's helpers
(`run LOGNAME cmd...`, `die`, `pgo_flags FILE`) and variables (`root`, `out`, `logs`, `jobs`, `iso`,
`opt_level`, `device_cpu`, `composite_pgo`, `host_pgo`, `accept_new`, `mods`). `profile_compile` must
set `module` and `profile_build_app` must set `app`.

A port's profile has to answer three questions, which are also its safety checks:

1. **Which disc?** Refuse anything but the exact game and revision the port was verified with (BlueWake
   checks the disc ID and the executable's hash).
2. **Which translator?** Pin every source by commit or checksum, so every player's build is the same.
3. **Is the result the verified one?** Compare the generated source with a recorded digest before
   compiling, so a wrong disc or translator fails in minutes instead of producing a broken app hours later.

A port whose app builds differently only changes its `profile_build_app`.

## Progress and build time

The terminal reports the active stage and elapsed time, with available compiler progress. A
`logs/progress.jsonl` event stream under the output directory records stage state for future PadMint
integration. Individual command logs remain under `logs/`; a failed stage reports its log path.
Measured on an M3 Max with 16 jobs on 2026-09-28, from a fresh clone of the public repository, the
whole default build took 104 minutes: about 2 minutes for tools, dependencies, disc checks,
translation and mods, 23 minutes of local training (its Mac test build plus playback), 79 minutes
compiling the game module at `-O2`, and under a minute for the app and IPA. Smaller Macs take longer.
`--no-train` saves the 23 minutes. A rerun with nothing changed reuses everything and took 31 seconds.
The build needs about 25 GB and writes a 161 MB IPA.

Training is reproducible: the fresh clone's profile matched an earlier independent run to within 84
counts out of 469 billion, and its compiled game module was byte-identical to that run's module.

A lighter `-O1` build was measured the same day as a faster option: it compiled in 47 minutes, but on an
iPad Pro (M2) at the Outset Island pier it averaged 26.1 FPS with the CPU at 99 percent (121 one-second
samples, none at 29 FPS or above), where the owner's build holds 30. It was dropped: it saves about half an
hour and loses the game's frame rate.

Since October 11 the build is **tiered**, as the Windows and Linux builders have been since October 2: only
the chunks the optimization profile never ran (504 of 813) compile at `-O1`; everything training ran stays
at `-O2`. On the same M3 Max the game module compiled in 61 minutes instead of 79. On an iPad Pro (M2), with
scripted 100-second runs from copied cards, Dragon Roost Cavern (code the training never runs) and the sea
by Outset both kept full speed, and the game thread was as busy as before: 52.1 and 52.2% against 51.9
and 51.8% in the cavern, 80.9% against 81.3% at Outset. `--no-tiered` compiles every chunk at `-O2`.

## Optimization profiles

The game module and the app are compiled with LLVM profile-guided optimization (PGO) when profiles are
available. The BlueWake profile bundles two in `scripts/builder/profiles/bluewake/`, both trained on the
macOS host: `composite-rt.profdata` (the runtime's dispatch, memory and CPU helpers) and `host.profdata` (the
app's host code: renderer glue, input, DSP). They contain profiling metadata for runtime and host
functions, including third-party code, rather than executable instructions. Both pass the automated asset scan; that scan alone is not a
provenance or licensing determination. `--no-pgo` skips them.

The developer's third profile records translated game functions and stays private. The player
build generates a replacement locally from the player's disc, by default (`--no-train` skips it),
through a separate instrumented Mac build and headless playback. It requires
reaching player control and executing translated functions before accepting a profile. The optional
`--training-save FILE` uses a copy of the player's BlueWake memory card; the default creates a new one.
The resulting `OUT/pgo-local/composite.profdata` stays local and is added to the device build. The
bundled host profile remains in use because headless training does not cover the renderer.
Measurements on 2026-09-28 used an iPad Pro (M2) at the Outset pier and the same scripted route:

| Build | FPS | CPU |
| --- | --- | --- |
| No profiles | 26.0 | 99% |
| The two bundled profiles | 27.5 | 99% |
| All three (the developer's build) | 29.9 | 83% |

A locally trained game module has not yet been measured on the iPad. On the Mac, four alternating
runs of the same saved-game route (developer, local, local, developer) produced identical game-state
records, and the local module averaged 13.26 ms per frame against the developer module's 13.62 ms.
That is CPU evidence from a headless test, not an iPad frame rate.

## PadMint

[PadMint](https://github.com/chrissotraidis/padmint) is the shared builder for the Pad projects and the
easiest way to build BlueWake for iPhone and iPad: a download for Windows, Mac and Linux that opens its
page on the player's own computer (building BlueWake needs an Apple Silicon Mac). It takes the app
without game code from BlueWake's latest release and adds the game module made from the player's disc.
Players' reports are in [#104](https://github.com/chrissotraidis/bluewake/issues/104).

Each game backend owns disc validation, translation, mods and optimization training. See the
[PadMint handoff](PADMINT_HANDOFF.md) for the interface and validation status. Local profile
generation and matched-device performance verification remain release requirements.

## What is public and what is not

Public source must exclude translated or decompiled game code, disc data, signing keys and saves.
`scripts/release/check_public_assets.sh` scans candidate public artifacts and fails closed; its
heuristics supplement manual provenance review. Gameplay screenshots illustrate the documentation.
The IPA the Builder writes contains the player's translated game module (`gGZLE01_recomp.dylib`): it is a personal build
and is never uploaded, attached or shared (AGENTS.md).
