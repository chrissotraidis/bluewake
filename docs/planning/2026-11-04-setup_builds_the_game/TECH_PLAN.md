# Setup builds the game: a release that carries no game code

Status: plan, not started. Written against `linux-setup-wizard` (#525), whose setup window is the
place this would grow from.

## The idea

A BlueWake release currently ships the translated game module: the 0.10-era AppImage carries
`gGZLE01_recomp.so` at 413 MB inside a 233 MB image, and the folder it is packaged from carries the
disc too. Everything about that is the project's own posture ("the APK you build is yours alone"),
but it means the artifact can only be produced on a machine that has the disc, so a release can
never come out of CI.

The alternative, already shipping in a sibling project, is to publish the *builder* and let the
player's machine produce the game once, at install.

TotK's README says exactly that:

> 1. Download `TotK-Setup-<version>-windows.zip` from Releases and unpack it.
> 2. Start `TotK-Setup.exe`. ...
> 3. Pick your game files, the language and an install folder, and press *Start*. The setup builds
>    the game once.
> 4. Play `totk.exe` from the install folder.

with 36 GiB free during setup and 20 GiB after. The release contains the installer, the recompiler,
the runtime and the tools; no game files, and nothing translated from them.

BlueWake is closer to this than it looks: the whole pipeline already exists as one command.

## What already exists

`scripts/linux/build.py` (and its Windows twin, `scripts/windows/build.py`) runs these steps in
order, each logged under `OUT/logs`:

| step | what it does | needs the disc |
| --- | --- | --- |
| `dependencies` | fetches the pinned RecompCore (`RECOMPCORE_SHA` in `scripts/builder/profiles/bluewake.sh`) | no |
| `disc` | reads the image's header, checks `GZLE01` | yes |
| `build_dolrecomp` | builds DolRecomp, the static recompiler | no |
| `extract` | `main.dol` and the 415 RELs out of the disc | yes |
| `translate` | `main.dol` → C chunks | yes |
| `generate` | assembles `composite-src`, verifies it against `COMPOSITE_DIGEST` | yes |
| `build_mods`, `prepare_blocks` | fast_blocks, direct_calls, inline_gpr, fp_checks, the native passes, lean_memory, return_ranges | yes |
| `configure_app` | the host: GXRuntime, Aurora/Dawn, SDL3 | no |
| module compile | 850 chunks of generated C into `gGZLE01_recomp.so` | no |
| `package` | the app folder: host, module, `dsp/`, `stage_select/`, provenance | no |

Two properties make this safe to hand to a player: the translation is checked against
`COMPOSITE_DIGEST` (so a player's tree is the tree the project verified), and the module's build is
recorded in `BuilderProvenance.json` with the composite digest and the source commit.

#525 then makes the app folder itself disc-free: `package()` no longer links the disc image in nor
copies `main.dol`/`rels/`, and records `containsGameFiles: false`, because the first run prepares
those from the player's own disc. #540 adds the Cancel that stops that preparation and removes what
it wrote.

So the setup window already owns "extract the disc's data and show the stages". This plan extends
the same window over the rest of the pipeline.

## What is missing

1. **A toolchain on the player's machine.** The module build wants clang 22/23 with lld and the
   project's flags (`-mllvm -large-interval-freq-threshold=10`, `x86-64-v3` for the desktop host,
   `cortex-a78` for the phone), plus cmake and ninja. Today that is the `bluewake-linux-ci` image on
   a dev box or a configured desktop. TotK's installer bundles its compiler; BlueWake must bundle
   clang+lld+ninja (or fail early with one clear message saying what to install).
2. **Python.** Every prepare pass is a Python script under `scripts/windows/` and `scripts/mods/`.
   Bundling a CPython is the cheap answer (a few MB, and the scripts are already portable), a C++
   port is not.
3. **A window that survives a long job.** #525 shows stages, #540 cancels, but neither drives a
   compile of 850 translation units or restarts into one. Needed: a resumable stage model (the
   builder already writes per-step logs and a receipt — `prepared-blocks.json`, `composite-*.digest`
   — so a restart can pick up where it stopped), progress in the window, and a "keep the logs"
   button for when it fails.
4. **Budgets.** Measured on a 5900X here: 13 GB of prepared tree, ~1.7 GB of generated source,
   ~1.3 GB of objects, a 413 MB module, 27 minutes at 9 parallel compiles. A four-core laptop is
   nearer two hours. The setup must state the requirement up front (TotK says 36 GiB during setup;
   BlueWake needs about 20 GB free plus a two-core floor) and must not start on a disk that cannot
   hold it.

## Shape of the change

One setup program per desktop platform, containing: the host binary (already game-free), the
DolRecomp source or binary (game-free), `scripts/` and `cmake/`, a bundled clang/lld/ninja and
CPython, and the first-run window. On *Start* it runs the table above from `disc` to `package`,
with the digest check reported in the window. The output is an install folder that plays, and
nothing in the downloaded artifact came from the game.

Stages, and where each already lives:

- **Check** — disc present, revision, free space, toolchain. (`Builder.disc()`, `check_tools()`)
- **Extract** — `main.dol`, 415 RELs. (`extract()`; #525's window already runs this shape)
- **Translate + generate** — 5 minutes; prints `composite source digest … the verified tree`.
- **Prepare** — the accelerator passes; the receipt decides whether they re-run.
- **Compile** — the long pole; jobs from RAM as `default_jobs()` already does.
- **Package + verify** — the app folder, then a smoke run (`--help`) as the release does today.

## Platform matrix

| platform | who builds the module | why |
| --- | --- | --- |
| Windows | the setup, on the player's machine | the builder already exists (`scripts/windows/build.py`); closest to TotK |
| Linux | the setup, on the player's machine | `scripts/linux/build.py` is that pipeline already; the AppImage would ship the builder instead of the game |
| macOS | the setup, if a clang is bundled | same, untested |
| Android | a desktop, pushed to the phone | no clang, no NDK on-device, ~20 GB and two hours of thermal budget |
| iOS | a Mac, pushed to the device | same |

The phone and the iPad stay "built on a PC, installed over adb/Xcode". That is today's flow and it
is the only one that stays inside a phone's power and storage.

## Decisions to take before building it

- **Training.** The default profile trains on two headless Outset playbacks plus a tour (~17
  minutes, reads the disc). A first release would ship untrained and offer training as a second,
  optional stage — otherwise setup doubles in time.
- **The DSP ROMs.** `package()` copies `dsp_rom.bin`/`dsp_coef.bin` from
  `ref/recompcore/Data/Sys/GC`. If those are Nintendo dumps, an on-the-fly release must not carry
  them: either extract them from the player's disc or default to `BLUEWAKE_DSP_MODE=hle`, which the
  Android build already uses.
- **Bundled compiler version.** Pinning clang in the setup fixes the module's codegen to a known
  compiler; the digest only covers the *source*, so record the compiler in provenance (it already
  does) and state the pin in the release notes.
- **Update flow.** TotK's model — run a newer setup on the same folder, saves and settings kept —
  fits BlueWake: the install folder is `~/.config/BlueWake/install.txt`'s target, and saves live
  outside it in `~/.local/share/BlueWake`.
- **Download size vs build time.** 50–100 MB of builder against 233 MB of AppImage, paid for with
  one to two hours of the player's CPU. That trade is the whole point of the model and should be
  said plainly in the release notes.

## What it buys

- Releases that contain no game-derived code, which is the only shape in which **CI can build and
  publish them**: with the module produced on the player's machine, the release job needs no disc
  and tests stay able to assert `Unexpected personal game module`.
- A player's build is verifiable: `COMPOSITE_DIGEST` for the translation, `module_sha256` and the
  compiler in provenance.
- One less thing to keep in sync: the release stops shipping a 413 MB blob that must be rebuilt for
  every RecompCore or prepare-pass change.

## Acceptance

- On a clean Windows 11 VM without a dev environment: unpack the setup, pick the disc, press Start,
  and get a folder that plays — no git, no clang, no Python installed by hand.
- The window reports `composite source digest 54f54434…: the verified tree` for a stock GZLE01 rev 0
  disc, and the resulting module matches the digest a maintainer's build produces from the same
  tree.
- Failure paths are honest: wrong revision, truncated image, less than the required free space, a
  cancelled compile leaves nothing half-written (#540's rule), and the log is offered.
- A restart after an interrupted compile resumes rather than starting over, proven by the receipt.
- The setup itself contains no `.dol`, no REL, no disc image, no module: the check is the same grep
  the CI fixture job does today.

## Out of scope

- On-device builds for Android and iOS.
- Shipping a prebuilt module as an *optional* companion download. It would defeat the point and the
  licence posture.
- Replacing the existing developer build (`scripts/linux/build.py`) — the setup calls the same
  steps, it does not fork them.

## Rough work breakdown

1. Bundled toolchain and CPython in a Windows setup payload; a dry run that only checks them. Days.
2. Drive `translate → generate → prepare → compile` from #525's window, with the receipt-based
   resume. The largest piece.
3. Packaging and the smoke run at the end of setup; provenance with the compiler pin.
4. The same on Linux, as an AppImage whose payload is the builder.
5. Release notes, requirements text, and the `docs/` page that replaces "build it yourself with a
   container" for players who only want to play.
