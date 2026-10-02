# BlueWake on Windows

BlueWake also builds as a native Windows x86-64 program. As on the Mac, you build it yourself from your own disc:
the game's code is translated from that disc during the build, so **the folder you build is yours alone: never
share or upload it.**

The Windows build is the same static recompilation as the iOS app: the same translator, the same pinned runtime
(RecompCore, GXRuntime, Aurora) and the same generated game source, verified against the same digest. Only the
host around it is different: Direct3D 12 through Dawn instead of Metal, SDL3 input and audio, and a Windows entry
point in place of the iOS app shell.

## Status

Experimental in BlueWake. The source port and settings overlay come from elliotttate's
`windows-release` commits `4b01c6b` and `4fbcc7f`; the original Git author is retained for the port.
The fork's initial PC measurements are recorded in its
[Windows port documentation](https://github.com/elliotttate/Wind-Waker-Recomp/blob/4b01c6bfd7e80e6c44880052d808c6e6b0ab7b39/docs/WINDOWS.md).
They are the contributor's results, not independent Windows gameplay evidence for this integration.

On October 1, the Win32 compatibility layer, entry shim, settings overlay and shared C host sources
were checked with a native macOS LLVM-MinGW cross-compiler. The source-only runtime/app also compiled
and linked on Windows with clang, the MSVC toolchain and Dawn in
[CI run 36799881615](https://github.com/chrissotraidis/bluewake/actions/runs/36799881615), including the
BlueWake settings theme and save-state sources. Neither check is Direct3D gameplay, a complete
personal-module build or controller/audio acceptance. Parallels is not needed for these checks.

The October 1 stability follow-up also passes native app linking and the 16-reader environment-cache
regression at `60be199` in [CI run 36805880425](https://github.com/chrissotraidis/bluewake/actions/runs/36805880425).
The source-only workflow opts into `BLUEWAKE_WINDOWS_REGRESSION_TESTS`; native app linking and both
the cache and shared FPS classifier/worker-counter checks pass at `830bf7d` in
[CI run 36808815002](https://github.com/chrissotraidis/bluewake/actions/runs/36808815002).
The pinned runtime's existing interpolation matching/blending/pacing suite also passes natively,
with the full app link and the other two tests, at `ec6e797` in
[CI run 36810248709](https://github.com/chrissotraidis/bluewake/actions/runs/36810248709).
The subsequent monotonic-clock initialization fix, its concurrent-clock regression, the other
three tests and the full app link also pass natively at `74da8ee` in
[CI run 36810799124](https://github.com/chrissotraidis/bluewake/actions/runs/36810799124).
These checks do not imply Windows gameplay or audio acceptance.

The reconciliation candidate includes generic prepaid-block and fixed-address
CPU transforms, module-owned RAM and inline floating-point helpers as separate
builder opt-ins. Other register/native preparation and experimental
60 Hz simulation remain separate work. Windows PGO training, matched Windows
measurements and full Windows gameplay remain acceptance gates.

## What you need

- Windows 10 or 11 on an x86-64 PC. The game module is compiled for `x86-64-v3` by default (AVX2, FMA, BMI2,
  MOVBE: Intel Haswell, AMD Zen or newer); the builder drops to an older level on older CPUs.
- A GPU with Direct3D 12
- [Visual Studio 2022 or newer](https://visualstudio.microsoft.com/) (Community is fine) with the
  **Desktop development with C++** workload and the **C++ Clang Compiler for Windows** component
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
the verified digest, adds the mods, compiles the game module and the app, and writes the app folder
`build\windows\BlueWake`. Each stage prints its progress; full logs are in `build\windows\logs`. Rerunning the
same command reuses finished work.

Options (`--help` lists all):

| Option | |
| --- | --- |
| `--source-only` | Stop after generating the source: checks your tools, disc and translation in a few minutes |
| `--no-mods` | Skip the mods (widescreen 16:9 and 16:10, Better Wind Waker's options) |
| `--prepared-blocks` | Opt into generic prepaid-block optimization; off by default, Windows timing/gameplay pending |
| `--fixed-cpu` | Opt into experimental fixed-address CPU storage; off by default, requires the matching app |
| `--fixed-mem1` | Also use module-owned RAM; requires `--fixed-cpu` and the matching app, off by default |
| `--inline-fp` | Opt into inline floating-point helpers; off by default, module/gameplay/performance qualification pending |
| `--jobs N` | Parallel compile jobs (default: the cores, as far as free memory allows) |
| `--march LEVEL` | CPU level for the game module (default `x86-64-v3`) |
| `--console` | Build `BlueWake.exe` as a console program |
| `--out DIR` | Build directory (default `build\windows`) |

The `--inline-fp` option leaves the module ABI unchanged and can be selected
independently of the storage/block experiments. It substitutes generic interpreter
operations only; it does not add native game routines, direct calls or memory
batching. Source preparation records the option and helper hashes, and disabling
it regenerates ordinary calls. Mac instruction-level comparisons pass; this does
not establish whole-module correctness or a speed improvement.

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
| Jump / Run | Space / Shift; enable them in settings |
| Fullscreen | F11 |
| Smooth Motion | F10 |
| Frame rate | F9 |

Game controllers work through SDL (Xbox, PlayStation, Switch Pro and others). The title screen wants A to reach
the file menu. The mouse turns the game's own camera around Link and tilts it, and a left click is A; a
cutscene, door, Z-target or first-person view takes the camera back.

Command-line options (`BlueWake.exe --help`):

| Option | |
| --- | --- |
| `--widescreen` | 16:9: the widescreen mod (a wider camera, culling and HUD) with a 16:9 picture |
| `--aspect 16:10` | 16:10 instead (`4:3` is the game's own) |
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
apply after restart. Touch-layout editing and save-management screens remain iOS-specific.
Windows optimization training and full end-to-end build/play verification remain pending.

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

## Disc recovery in the reconciliation candidate

The launcher now accepts a replacement owned USA revision-0 disc when the
prepared files/disc are missing. It validates and prepares a selected ISO/GCM,
then remembers its location. Compressed inputs require nodtool.exe beside the
app; the Windows builder copies its pinned nodtool when it is available. An
ISO-only build can instead use an ISO converted locally in Dolphin. A missing
player-generated module still requires the source build; choosing a disc alone
does not create translated game code inside the launcher.

Conversions and preparation use unique `disc-import-*` folders under the data
directory. Failed/previous inputs and prepared files remain there. `disc.txt`
remembers the disc and `game.txt` the completed preparation. Failed preparation
does not overwrite the last prepared files; a missing REL invalidates the
cached preparation. Saves/settings keep their existing paths and behavior.
This retains Elliott Tate's disc UI/import foundation while replacing the
fork's overwrite/delete recovery paths. No donor compiled distribution is used.

Native source-only CI builds the launcher and a Win32 filesystem/process
regression with synthetic importer results. Real owned-disc picking/conversion,
Unicode paths, interruption, gameplay and physical Windows save/reload remain
acceptance gates until recorded against this candidate.

The Display tab offers 60 FPS, 120 FPS and Match the display (up to 240 FPS)
for experimental Smooth Motion. Game logic stays at 30 Hz; Smooth Motion stays
off until explicitly enabled. A slower or unknown display temporarily uses
60 FPS and keeps a saved 120/display preference. Moving the window rechecks
the display within a second. Overload pacing may reduce the displayed rate.
`--smooth` explicitly selects 60 for that session without replacing a stored
rate unless the player edits it. These settings are source-integrated; actual
window movement and gameplay acceptance remain pending.
