# BlueWake on Windows

**Most players should download the ready-made Windows build** from the [Releases page](https://github.com/chrissotraidis/bluewake/releases/latest): unpack it,
run `BlueWake.exe` and choose your own disc image (`.iso` or `.gcm`).

If your browser or Windows says the download is a virus, see
[If Windows says it's a virus](#if-windows-says-its-a-virus).

This page is about building it yourself. BlueWake builds as a native Windows x86-64 program from your own disc:
the game's code is translated from that disc during the build, so **the folder you build is yours alone: never
share or upload it.**

The Windows build is the same static recompilation as the iOS app: the same translator, the same pinned runtime
(RecompCore, GXRuntime, Aurora) and the same generated game source, verified against the same digest. Only the
host around it is different: Direct3D 12 through Dawn instead of Metal, SDL3 input and audio, and a Windows entry
point in place of the iOS app shell.

## Status

Windows is BlueWake's ready-made download. Since 0.5.0 (October 5) every Windows release is built from `main`
on a maintainer's PC from their own disc, checked there, and published by Chris; the latest is 0.9.0. What each
build was checked for, and what still needs a Windows PC, is in [Windows tasks](WINDOWS_TASKS.md); the release
records are `docs/status/WINDOWS_BUILD_*.md`. The CI host build and its tests run on every pull request. The
early history of the port (Elliott Tate's `windows-release` commits and the October 1 to 4 CI runs) is in
[status/WINDOWS_SOURCE_PORT_2026-10-04.md](status/WINDOWS_SOURCE_PORT_2026-10-04.md).

## What you need

- Windows 10 or 11 on an x86-64 PC. The game module is compiled for `x86-64-v3` by default (AVX2, FMA, BMI2,
  MOVBE: Intel Haswell, AMD Zen or newer); the builder drops to an older level on older CPUs.
- A GPU with Direct3D 12
- [Visual Studio 2026](https://visualstudio.microsoft.com/) (Community or Build Tools is fine), version 18.10 or
  newer, with the **Desktop development with C++** workload and the **C++ Clang Compiler for Windows**
  component. Visual Studio 2022 also builds: its clang (19) can't read the app's optimization profile
  (`windows/pgo/app.profdata`, recorded with clang 22), so the builder says so and builds the app without it,
  which leaves its graphics thread a little slower ([#153](https://github.com/chrissotraidis/bluewake/issues/153)).
  `--no-app-pgo` does the same on any version.
- [Python 3.10+](https://www.python.org/), [Git](https://git-scm.com/), and CMake 3.25+ and Ninja
  (`pip install cmake ninja` works)
- Your disc image of *The Legend of Zelda: The Wind Waker*, GameCube USA (`GZLE01`, revision 0). An `.iso` or
  `.gcm` works directly. A Dolphin `.rvz` (or `.wia`, `.gcz`, `.ciso`, `.nfs`) is converted to an ISO with
  [nodtool](https://github.com/encounter/nod), which the builder compiles from crates.io the first time; that needs
  [Rust](https://rustup.rs). You can instead convert it in Dolphin (right-click the game, **Convert File...**,
  format ISO).
- About 15 GB of free disk space (the converted disc, the generated source and the compiled module)

## Build

From a normal terminal in the checkout:

```bash
python scripts/windows/build.py "D:\Games\The Legend of Zelda - The Wind Waker (USA).rvz"
```

The builder finds Visual Studio itself (no developer prompt needed), fetches the pinned RecompCore and DolRecomp
into `ref/`, checks and converts the disc, extracts and translates the game, checks the generated source against
the verified digest, adds the mods, records a local optimization profile, compiles the game module and the app, and writes the app folder
`build\windows\BlueWake`. Each stage prints its progress; full logs are in `build\windows\logs`. Rerunning the
same command reuses finished work.

Options (`--help` lists all):

| Option | |
| --- | --- |
| `--source-only` | Stop after generating the source: checks your tools, disc and translation in a few minutes |
| `--no-train` / `--no-pgo` | Explicitly skip local training and compile without a profile |
| `--retrain` | Record a new local profile instead of reusing a matching one |
| `--host-only` | After pulling a change to the app's own code (`runtime/host/src`, `windows/src`): keep the last build's optimization profile instead of training again, so only what the change reaches recompiles |
| `--no-mods` | Skip the mods (widescreen 16:9, 16:10 and 21:9, Better Wind Waker's options) |
| `--conservative` | Build the plain translation. By default the builder prepares the same optimizations as Wind Waker Recomp's builds (prepaid blocks, fixed CPU and RAM storage, inline floating point and memory access, direct calls, register inlining, and the certified native J3D, vector, game-math, skinning and matrix functions); with `--conservative`, each `--...` option adds one back |
| `--no-lean-blocks` | BlueWake's conservative prepaid block copies instead of Elliott Tate's lean ones (on by default since October 10, as Wind Waker Recomp's builds make them). The module is about a quarter smaller and compiles faster, and the game runs 5 to 12% slower ([PERFORMANCE.md](PERFORMANCE.md#results)) |
| `--native-entries` | Elliott Tate's second and third native sets; off by default. They apply only where the translation matches what his builder produces, so they need the lean copies (the default) |
| `--no-lean-memory` | Loads and stores in the prepaid copies as before Elliott Tate's lean memory (on by default since October 12), and so without lean deadlines either: about 5% slower. Lean memory changes nothing with `--no-lean-blocks` |
| `--no-lean-deadlines` | Keeps the deadline test after every load and store. Lean deadlines (on by default since October 12) test it only on an access's way out of RAM, where the deadline can move: About 4 to 5% faster on a Mac, the same game state ([PERFORMANCE.md](PERFORMANCE.md#results)). `--native-entries` declines its natives with it, since their hashes cover the copies, so use `--no-lean-deadlines` with `--native-entries` |
| `--no-behaviour-timing` | Checks the turn's budget after every block, as before behaviour timing. Behaviour timing (on by default since October 14) runs every block of game code from its prepaid copy, so a timer or an interrupt lands at the start of the next block instead of on its exact instruction, and the budget is checked where the code can loop. It was 7 to 9% faster on a Mac, 7 to 11% on two Intel desktops and 9% on the Steam Deck, with a module 8 to 15% smaller and a quicker build. The game state isn't identical to a build without it, but the 468-place stage select sweep and the sound came out the same, and it played normally on the Deck and on desktops ([the note](status/BEHAVIOUR_TIMING_2026-10-12.md), #215) |
| `--no-cold` | Compile code the training never ran for speed too; slower in every test so far, so off |
| `--jobs N` | Parallel compile jobs (default: the cores, as far as free memory allows) |
| `--march LEVEL` | CPU level for the game module (default `x86-64-v3`) |
| `--console` | Build `BlueWake.exe` as a console program |
| `--out DIR` | Build directory (default `build\windows`) |

A rebuild recompiles only the parts of the game module whose source changed. After a change to the app alone,
such as a new setting or mod hook, a normal rebuild still trains again (the training plays the game with the
app), which takes most of the time; `--host-only` skips that. Run it from the same checkout and `--out` as the
build it updates.

Local profiles stay inside the private build directory. Training uses new isolated
data folders, original 30 Hz gameplay and interpolation off; it never uses the
player's normal card or a bundled Apple profile. Both reaching player control and
recording executed game functions are required. Failed attempts remain available
for diagnosis and do not replace a previously validated profile. Reuse requires
matching prepared source, runtime, compiler, CPU level, selected options and host
playback code. A profile hash in its filename makes changed counts invalidate
Ninja's compile inputs.

The independent `--native-j3d`, `--native-vec`, `--native-math`, `--native-skin` and `--native-game-math` options certify
the original function bodies before other rewrites. `BLUEWAKE_NATIVE_J3D=1`,
`BLUEWAKE_NATIVE_VEC=1`, `BLUEWAKE_NATIVE_MATH=1`, `BLUEWAKE_NATIVE_SKIN=1` and `BLUEWAKE_NATIVE_GAME_MATH=1` enable their respective
supporting host/module paths on the Mac (the Windows and Linux hosts use them unless one is set to `0`)
through versioned handshakes; absent support, disabled selection,
unsupported inputs and observed boundaries retain translated execution.
Matrix-array workers remain opt-in on macOS (`BLUEWAKE_NATIVE_WORKERS=1` through
`8`); Windows keeps Elliott's serial path. Both paths require qualification,
and worker selection is separate from rendering interpolation.

The direct-call options retain the ordinary module ABI and are selected separately
from fixed CPU/RAM storage. The Windows and Linux hosts use them whenever the module
supports them (`BLUEWAKE_DIRECT_CALLS=0` turns them off); older hosts use ordinary dispatch.
The host must approve each skipped boundary, including scene observations,
feature hooks and pending jump input. Register inlining also verifies the
translated helper hashes and declines aliases, MMIO and write journals.
Changing host watch sources or either option invalidates prepared-source reuse.
These options are being qualified; no performance gain or gameplay acceptance
is claimed yet. Personal generated source and modules stay local.

The `--inline-fp` option leaves the module ABI unchanged and can be selected
independently of the storage/block experiments. It substitutes generic interpreter
operations only; it does not add native game routines, direct calls or memory
batching. Source preparation records the option and helper hashes, and disabling
it regenerates ordinary calls. Mac instruction-level comparisons pass; this does
not establish a speed improvement. Current bounded module results and their
limits are tracked in the [reconciliation ledger](status/FORK_RECONCILIATION_2026-10-02.md).

The `--gather-pipe` option also leaves the module ABI unchanged. It can be used
with or without `--inline-fp` and records its helper hashes in build provenance.
The Windows and Linux hosts use direct FIFO writes with such a module unless `BLUEWAKE_GATHER_PIPE=0` is set.
With that enabled, `BLUEWAKE_GATHER_PIPE_BATCH=1` additionally requests batching
on a backend that supports byte writes. Both require a module prepared with
`--gather-pipe`; older modules keep their ordinary MMIO path. FIFO diagnostic
tracing and edge-census builds retain the ordinary handler. Renderer batching,
whole-module optimized comparisons and performance remain unaccepted until the
reconciliation ledger records their specific qualification.

## Play

Run `build\windows\BlueWake\BlueWake.exe`.

| | |
| --- | --- |
| Control stick | W A S D |
| C-stick | T F G H |
| D-pad | arrow keys |
| A, B, X, Y | J, K, U, I |
| L, R, Z | E, R, Q |
| START | Return |
| Camera | Click the game, then move the mouse (Esc releases it) |
| Settings | F1 or Esc after releasing the mouse |
| Jump / Sprint | Space / Shift; turn on Jump & Sprint in settings (Gameplay) |
| HD look | F2 |
| Save state / load the latest | F5 / F8 |
| Stage select | F7, once it's on in settings |
| Frame rate | F9 |
| Smooth Motion | F10 |
| Fullscreen | F11 or Alt+Enter |

A few words at the bottom left say what each key did.

Game controllers work through SDL (Xbox, PlayStation, Switch Pro and others). The title screen wants A to reach
the file menu. To choose which controller button presses each GameCube button, open settings and use
**Controls › Controller button mapping**. The optional controller zoom is
also under **Controls**: when enabled, hold the right-stick click and move that stick up or down; a tap still
enters or leaves first person. The mouse turns the game's own camera around Link and tilts it, and a left click is A; a
cutscene, door, Z-target or first-person view takes the camera back.
**Controls › Mouse button mapping** sets what the left, middle, right and side buttons press, and **Keyboard mapping**
changes the GameCube buttons' keys. A controller SDL doesn't
recognise works once its mapping is in a `gamecontrollerdb.txt` (from
[SDL_GameControllerDB](https://github.com/mdqinc/SDL_GameControllerDB)) beside your saves; the session log's
`[pad] N controller mappings` line shows it was read.

Command-line options (`BlueWake.exe --help`):

| Option | |
| --- | --- |
| `--widescreen` | 16:9: the widescreen mod (a wider camera, culling and HUD) with a 16:9 picture |
| `--aspect 16:10` | 16:10 instead, or `--aspect 21:9` for ultrawide (`4:3` is the game's own) |
| `--smooth` | Smooth Motion (experimental, off by default): 60 FPS, the renderer drawing a blended frame between each of the game's 30 |
| `--betterww` | Better Wind Waker's settings at their defaults (Swift Sail, instant text, faster climbing...) |
| `--options LIST` | Change them: `name,-name,...`, or `none,name,...` (names in `mods/betterww/options.txt`) |
| `--fullscreen` | Start in fullscreen |
| `--window WxH` | The window's size |
| `--scale N` | Render at N x 480 lines (0: the window's own pixels) |
| `--fps` | Show the frame rate |
| `--stretch` | Fill the window instead of keeping the game's aspect ratio |
| `--no-mouse-camera` | Keep the mouse out of the camera |
| `--lle-audio` | Run the DSP's own microcode instead of the high-level Zelda ucode |
| `--disc FILE` | Read another copy of the disc |

The mods need no extra files: they are compiled into your game module from your disc (see [MODS.md](MODS.md)).
The environment variables `scripts/mac/run_host.sh` documents (`BLUEWAKE_*`, `DOL_*`) work the same way, for
example `BLUEWAKE_MOUSE_SENSITIVITY` and `BLUEWAKE_MOUSE_INVERT_Y`.

## Your saves and logs

Everything that is yours lives in `%APPDATA%\BlueWake`, outside the build, so rebuilding or deleting the build
never touches it:

- `GZLE01.card`: the memory card with your saves
- `sram.bin`: the console's settings (sound mode and the like)
- `logs\session-*.log`: the newest eight sessions, one line a second of speed and timing plus anything that went
  wrong. Attach the relevant one to a bug report. If BlueWake crashes, the log says where.
- Aurora's pipeline cache, so later launches start drawing sooner

**Portable mode:** create an empty file named `portable.txt` next to `BlueWake.exe`, and BlueWake keeps all of
this in a `user` folder beside it instead. To bring existing saves
along, copy the contents of `%APPDATA%\BlueWake` into that `user` folder first. `scripts\windows\build.py --portable`
puts `portable.txt` in the app folder for you; a later rebuild leaves the `user` folder and `portable.txt` alone. An `.iso` or `.gcm` disc image
is read where it is in either mode; only Dolphin's compressed formats are unpacked to a copy.

## How the port works

The Windows host is `windows/`: a CMake project that compiles the unchanged host (`runtime/host/src`), GXRuntime
with Aurora (prebuilt Dawn and SDL3 packages, as Aurora fetches them), and Dolphin's DSP from RecompCore, with
clang (GNU driver, MSVC ABI) from Visual Studio.

- **POSIX calls.** The host uses a handful: threads, clocks and sleeps, the environment, `dlopen`, directory
  listing. `windows/compat` provides them on Win32 (sleeps use a high-resolution waitable timer, since `Sleep`
  rounds up to the 15.6 ms tick). The header is force-included into BlueWake's own sources and GXRuntime's C
  runtime only, never into third-party code.
- **The game module** is `gGZLE01_recomp.dll`, built by the same `cmake/composite` project as the iOS dylib, and
  loaded the same way. On Windows it exports its entry points explicitly.
- **The same source, byte for byte.** Windows' C runtime and Python write text files with CRLF line endings,
  which would change the generated game source and its verified digest. The translator is linked with MSVC's
  `binmode.obj` (binary file mode by default), and the generators write `\n` explicitly. The source this builder
  generates has the same digest as the macOS builder's.
- **Compile time.** Each translated chunk is one very large function, and two LLVM passes are superlinear on
  them with clang 22 for x86-64 (measured with `-ftime-report`). The SLP vectorizer took 92 percent of a
  typical large chunk's time, and the largest chunks took over half an hour each; `-fno-slp-vectorize` brings
  them to a minute or two. The register coalescer took 95 percent of the worst remaining chunk's 44 minutes,
  joining copies into the context pointer's function-long live range again and again; capping that per range
  (`-mllvm -large-interval-freq-threshold=10`) brings it to about two minutes.
- **Memory.** A large chunk takes 1 to 3 GB in clang, so the builder runs as many compile jobs as free memory
  allows, not one per core, and retries a chunk that ran out of memory with fewer jobs.
- **The DSP** runs Dolphin's interpreter and its high-level Zelda ucode, as on iOS; the x64 DSP JIT is not built.
- **Floating point.** GXRuntime maps the guest's rounding and non-IEEE modes onto the x86 MXCSR, as it does onto
  the arm64 FPCR.
- **Stack.** Translated code recurses on the host stack as the game does on the GameCube's; the executable
  reserves 64 MB for the main thread (Windows' default is 1 MB).

- **Better Wind Waker's REL sites.** DolRecomp names a REL's option sites by its file name after the last `/`,
  and on Windows it joins a folder and a file with `\`, so the sites in `d_a_ship` and `d_a_agbsw0` would not
  match. The builder names the RELs' folder with `/` and a trailing `/` for that translation, which gives the
  15 option chunks the Mac build has, not 11.
- **Windows' own costs.** The C runtime's `getenv` locks and scans the whole environment, and GXRuntime reads
  a trace switch on every guest exception (4 percent of the game thread in a profile): its C sources and the
  module's runtime remember each call site's answer (`windows/compat/bw_getenv_cache.h`). Aurora paces frames
  with short sleeps, which Windows' default 15.6 ms timer tick would stretch; the app asks for 1 ms.
- **Profiling.** `BLUEWAKE_HOST_PROFILE=FILE` samples the game thread every millisecond and writes where it
  was, by module and offset, charging time in system code to the BlueWake function that called it.

The Windows overlay saves display, camera, mod and audio choices in `settings.ini`; mods and audio mode
apply after restart. The file uses the same `KEY=VALUE` names as the Mac and Linux one (`BLUEWAKE_ASPECT=21:9`,
listed in `runtime/host/src/settings_table.h`). A file from before that (`aspect=`, `option.NAME=`...) is read
and converted once, and the old one is kept beside it as `settings.ini.before-shared-menu`. Launch options and
variables set before BlueWake starts still win for that session. Touch-layout editing and save-management screens remain iOS-specific.
What each Windows release build was checked for is in [Windows tasks](WINDOWS_TASKS.md).

The prepaid-block candidate has bounded correctness evidence on an M3 Max:
30,000 cases across 14 SDK/J3D entries compare every CPU/RAM byte between two
privately generated, uninstrumented O0 modules, including deadline observation
suffixes and partial stops. Deliberate suffix/RAM mutations fail the fixture.
A conservative transform also passes a controlled 6,000-retrace headless boot
comparison: 1,050 canonical records, resulting cards and 600 guest-state samples
match, with zero scheduling drift. It retains every PC store and leaves blocks
that refund cycles or use unsupported prepaid-state forms untouched. Earlier
less conservative candidates failed the existing route comparator. This is
bounded Mac O0 correctness; optimized x86, Windows timing and gameplay
acceptance remain open.
The builder records the optimization selection and script hash, rejects stale
generated-tree reuse, and safely re-prepares after an interrupted stage. Changing
the selection regenerates the appropriate source; it does not change game logic,
Smooth Motion preferences, saves or the module ABI.

The separate `--fixed-cpu` experiment puts guest CPU storage in the module and
requires the host and translated code to use that same state. Its module uses
BlueWake ABI 4; ordinary modules remain ABI 3. Build the app and module together.
Older apps reject ABI 4 before starting the game. To return to an older app,
rebuild without `--fixed-cpu`. The new desktop host still accepts ordinary ABI 3
modules with the matching CPU layout, but rejects older undeclared fixed-CPU
or fixed-MEM1 modules with a rebuild message. This option
does not enable Smooth Motion or 60 Hz simulation. Windows correctness, matched
timing and gameplay qualification remain open; donor measurements do not establish
a BlueWake speedup.

The separate `--fixed-mem1` experiment requires `--fixed-cpu` and declares ABI 5.
The app validates both storage getters, requires an aligned 32 MiB RAM buffer
separate from the CPU state, and adopts that buffer without freeing it at
shutdown. Older apps reject ABI 5; rebuild without `--fixed-mem1` to return to a
fixed-CPU app, or without either storage option for an ordinary ABI-3 app.
The pinned runtime also fixes aliases in the host's extra 8 MiB of RAM; this
fix applies to ordinary modules too. Sanitizer checks cover the memory contract,
but full module correctness, Windows timing and gameplay are still pending.

## When the disc goes missing

If the prepared game files or the disc image BlueWake used before are gone, the
launcher asks for your own USA revision-0 disc (GZLE01) again. It checks the
image (its game ID and `main.dol`), prepares `main.dol` and the 415 RELs once,
and remembers where the disc is. An `.iso` or `.gcm` is used where it is. A
Dolphin image (`.rvz` and the other compressed formats) needs `nodtool.exe`
beside the app: a folder you build yourself gets the builder's pinned nodtool
when the builder has made one, and the ready-made release leaves it out because
it embeds Wii keys, so with the release convert the image to an ISO in Dolphin
first. The launcher prepares the
disc's files; the translated game module comes from the builder or the
ready-made release.

Each conversion and preparation goes into a new `disc-import-*` folder in the
data folder, and earlier folders stay. `disc.txt` remembers the disc and
`game.txt` the finished preparation. A failed preparation keeps the last good
files, and if a REL is missing the disc is prepared again. Saves and settings
stay where they are. This builds on Elliott Tate's disc picker and importer
(`windows/src/win_disc.c`); the Windows CI runs `bluewake_windows_disc_test`
with stand-in importer results.

**Smooth Motion** (experimental, off by default, F10): the Display tab offers
60 FPS, 120 FPS and Match the display (up to 240 FPS). The game's logic stays
at 30 Hz. A display slower than 120 Hz, or one whose rate isn't known yet, uses
60 FPS and keeps the saved choice. BlueWake checks the display every second, so
moving the window to another screen takes effect within a second. When the PC
can't keep up, the shown rate can drop. `--smooth` turns it on at 60 FPS for
that session and leaves a saved rate alone unless you change it in the menu.

## If Windows says it's a virus

BlueWake isn't code-signed yet, so every new release is a program Windows has never seen, and antivirus
programs sometimes block a brand-new one. 0.8.0 was flagged by Microsoft Defender as `Tecabans.STV!cl` on its
first day. When Chrome or Edge says "Failed – Virus detected", it's Windows' antivirus that blocked the file.

1. Get BlueWake only from the [Releases page](https://github.com/chrissotraidis/bluewake/releases/latest).
   There are no mirrors, and nobody from the project sends download links in messages.
2. Check the zip. In PowerShell, in the folder you saved it to, run
   `Get-FileHash .\BlueWake-vX.Y.Z-windows-x64.zip` (with the real file name). The result must match the zip's
   line in that release's `SHA256SUMS`. If it doesn't, delete the zip and don't run anything from it.
3. If it matches, you can wait while we ask Microsoft to clear it, keep playing the version you have, or open
   Windows Security › Virus & threat protection › Protection history, pick the BlueWake entry and choose
   **Allow on device** (or **Restore**). Your saves in `%APPDATA%\BlueWake` work with every version.
4. Tell us the detection name it showed, on Discord or in an
   [issue](https://github.com/chrissotraidis/bluewake/issues). It helps us get the release cleared.

Please don't turn your antivirus off. If it finds something outside the BlueWake folder, such as a program that
keeps coming back in `AppData`, that didn't come from BlueWake: run a Microsoft Defender Offline scan
(Windows Security › Scan options) and change your passwords from another device.
