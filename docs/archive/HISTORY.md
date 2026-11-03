# BlueWake project history

This is the README as it stood before the source preview (September 27, 2026), kept verbatim as a record of how the project got here. It is historical: the current state is in the [README](../../README.md) and [docs/archive/status/CURRENT.md](status/CURRENT.md). Relative links below were written for the repository root.

---

# BlueWake

**An Apple Silicon static recompilation experiment for the original GameCube release of *The Legend of Zelda: The Wind Waker*.**

[![Repository Audit](https://github.com/chrissotraidis/bluewake/actions/workflows/audit.yml/badge.svg)](https://github.com/chrissotraidis/bluewake/actions/workflows/audit.yml)

> [!IMPORTANT]
> BlueWake is a source-only research prototype. It requires a legally obtained
> US revision 0 `GZLE01` disc image and does not include Nintendo code or
> assets. It currently reaches real gameplay, but it is not yet a normal-speed,
> visually correct, distributable port.

BlueWake translates the retail `GZLE01` DOL and all 415 REL modules ahead of
time, links them into a native arm64 module, and runs the result in a signed
macOS application. It does not use a runtime PowerPC JIT. The compatibility
runtime is Dolphin-derived and includes an interpreter fallback, so this is
static recompilation, not an "emulation-free" claim.

As of **August 30, 2026**, the original game boots through title and file
selection, creates and reloads a save, reaches authentic gameplay on Outset
Island, accepts player and camera input, climbs the lookout ladder, opens the
door, and transitions through the retail scene machinery into Omasao. This is
real gameplay, not a renderer demo or a host-constructed scene.

It is also not release-ready. Visible play is currently much slower than the
original game, and graphics correctness still has obvious defects, including
Link's hair/material rendering. Broader visual, audio, controller, and
long-duration acceptance remain open.

As of **September 23, 2026**, the same retail route also runs on **iPadOS in the
iOS simulator**: the iPad app boots the user's disc through title, file select,
the prologue and the Outset play scene, and admits player control at the same
guest retrace as macOS, with Metal rendering, DSP audio and on-screen touch
controls. Outset play now runs about 58-59 of the 60 retraces a second authentic speed needs (56-57 in its densest view)
(simulator heavy view and macOS headless benchmark on an M2), up from 18-27 at the start of the iPad work; physical
iPad testing has not started.

As of **September 24, 2026 (night)**, the iPad simulator app plays Outset, its interiors and the
village at 58-60 retraces a second (the densest view at 51-54), in stereo, with saves, touch,
keyboard and controller input, and matches Dolphin across the title, menus, text, the walk, the
village and gameplay audio. The device build compiles and is ready to sign; the next step is a
physical iPad run. Where it stands and how to install it:
[docs/archive/status/IPAD_STATE_2026-09-24.md](status/IPAD_STATE_2026-09-24.md).

As of **September 25, 2026**, a fresh checkout builds the device app from the user's own disc image
with one command, `scripts/ios/build_device.sh DISC.iso`, using only published sources: the
RecompCore and DolRecomp forks and the pinned Dawn package. Prerequisites, signing and installing:
[docs/DEVICE_BUILD.md](../DEVICE_BUILD.md).

As of **September 26, 2026**, BlueWake runs on a **physical iPad Pro (M2)** at a steady 30 FPS at full
game speed after loading a save, in Outset's heavy pier view and while walking the village (the 15 FPS
phases after loading were frames the XFB exchange discarded, not a slow CPU). The in-game menu has a
**Mods** section with the 16:9 widescreen code, Dolphin-format HD texture packs and Better Wind Waker;
code mods are compiled into the app as variant chunks, since a statically recompiled game cannot take
code patches at runtime. See [docs/MODS.md](../MODS.md).

## Status

**September 23, 2026 reorientation (active):** iPadOS is the product target
now, hosted on Route A, tested one simulator at a time. Route B continues as the
long-term speed track and does not gate iPadOS. See
[the iPadOS reorientation](status/IPADOS_REORIENTATION_2026-09-23.md), the
[v56 loop](GOAL_PROMPT_V56_2026-09-23.md) and the
[iOS build instructions](../../apple/ios/README.md).

**September 9, 2026 reorientation:** Route B is the active source-native route,
now prioritizing full original scene/stage owners, observed J3D rendering, live
world input, and original scheduler integration. Route A remains an oracle.
Recorded Route B evidence includes native 2D presentation and headless Link
frames; a playable native 3D world remains unqualified. The active plan is
[the whole-unit integration loop](status/REORIENTATION_2026-09-09.md).
The phase-two summary below is historical and superseded by the live ledger.

**Native Route B update — September 6, 2026:** active source-port work now
passes Link's original phase-two initialization in headless Debug, Release
and sanitizer diagnostics with real model/animation resources. This is not
complete actor creation or playable native gameplay: phase three, continuous
player/camera/collision integration, drawing and input remain open. The table
below records the older Route A baseline, not Route B acceptance. See the
[phase-two evidence](status/ROUTE_B_PLAYER_PHASE_TWO_2026-09-06.md).

| Area | State | Current evidence |
|---|---|---|
| Disc preparation | Pass | Validates user-owned `GZLE01`; inventories 1 DOL + 415 RELs deterministically |
| Static translation | Pass | 416/416 executables; about 1.95M labels; zero unknown instructions |
| Native composite | Pass | 748 translated chunks in an arm64 dylib; ABI and lifecycle tests pass |
| Authentic macOS boot | Pass | Title, file select, new game, opening, and Outset gameplay |
| Interaction and persistence | Pass, bounded | Movement, camera, ladder, door, save, quit, and reload |
| Scene transition | Pass, bounded | Retail Outset-to-Omasao transition and signed indoor residence |
| Performance | Open, improving | Exact shipping replay: Outset median 6.0 fps, Omasao median 13.6 fps; full-route wall time is 22.4% below the prior exact-eight build |
| Graphics correctness | Open | Link's hair/material state is visibly wrong; wider GX fidelity still needs qualification |
| Product acceptance | Open | Physical controller, human audio review, unrestricted play, and endurance |
| iPadOS (simulator) | Pass, bounded | First run, title to Outset play, saves, stereo audio, touch/keyboard/controller; 58-60 retraces a second (densest view 51-54); matches Dolphin across the checked scenes |
| iOS / iPadOS hardware | Next | One-command device build from the user's disc, verified from a clean checkout; install needs the user's signing identity ([build and install](../DEVICE_BUILD.md), [what to check](status/IPAD_STATE_2026-09-24.md)) |

The live engineering record is in [CURRENT.md](status/CURRENT.md), the
gate ledger is in [GATES.md](status/GATES.md), and the prioritized work
remaining is in [FINISH_LINE.md](status/FINISH_LINE.md).

## Architecture

```text
User-owned GZLE01 disc image
            |
            v
  Validate and inventory
  main.dol + 415 REL modules
            |
            v
  DolRecomp ahead-of-time translation
  PowerPC/Gekko -> portable C / native objects
            |
            v
  BlueWake composite module
  748 translated chunks + REL lifecycle tables
            |
            v
  Native macOS host
  +-------------------+-------------------+
  | CPU / time / OS   | DVD / CARD / SI   |
  | donor DSP core    | GXCore / Aurora   |
  +-------------------+-------------------+
            |
            v
     Signed arm64 BlueWake.app
```

The retail scheduler, events, actors, collision, and scene logic remain game
code. BlueWake supplies the native execution contract around that code:
memory and CPU state, authentic guest timing and exceptions, dynamic REL
lifecycle, disc and save services, input, DSP audio, and GX translation.

## What Static Recompilation Means Here

GameCube instructions are translated before launch into native-buildable code.
A simplified example looks like this:

```c
// Gekko / PowerPC
//   addi r3, r4, 0x20
//   lwz  r5, 0x10(r6)

// Ahead-of-time translated representation
ctx->gpr[3] = ctx->gpr[4] + 0x20;
ctx->gpr[5] = guest_read_u32(ctx->gpr[6] + 0x10);
```

That removes the need to generate PowerPC translations while the game is
running. It does not remove the need to reproduce the console's timing,
exceptions, memory model, operating-system services, audio DSP, and graphics
pipeline accurately. Most of BlueWake's difficult work lives at those
boundaries.

## Requirements

- Apple Silicon Mac
- macOS 14 or newer
- Xcode command-line tools and a C/C++ compiler
- CMake 3.27+, Ninja 1.11+, and Python 3.10+
- A legally obtained, correctly dumped US revision 0 disc image (`GZLE01`)

No Nintendo executables, assets, disc images, generated translated game code,
saves, or private captures are distributed by this repository.

## Developer Setup

Clone the pinned public dependencies:

```bash
./scripts/bootstrap.sh
```

Validate and inventory a user-owned disc image. Output remains local and is
gitignored:

```bash
python3 scripts/prepare.py /path/to/GZLE01.iso
```

The complete private translation/composite procedure is intentionally kept in
the implementation record because it depends on generated, non-redistributable
inputs. Start with [HANDOFF.md](HANDOFF.md) and the proven P1-P3 evidence
in [GATES.md](status/GATES.md). Once those local outputs exist, build the
DSP-enabled macOS host and run its tests with:

```bash
./scripts/build_macos_dsp_host.sh
```

Launch the signed developer app against the local composite and user-owned
inputs:

```bash
BLUEWAKE_DOL=generated/full/main.dol \
BLUEWAKE_DISC=/path/to/GZLE01.iso \
BLUEWAKE_RELS_DIR=generated/full/rels \
build/runtime-host-dsp/BlueWake.app/Contents/MacOS/BlueWake \
build/composite-lib/gGZLE01_recomp.dylib
```

The current project is an engineering prototype, not a packaged end-user
release. Exact signed acceptance commands, card identities, and bounded input
routes are recorded in the status ledger rather than presented as normal play
instructions.

Normal builds compile developer trace probes out of the hot dispatch loop. To
produce a diagnostic build that honors the `BLUEWAKE_TRACE_*` environment
switches, use a separate build directory:

```bash
BLUEWAKE_HOST_BUILD_DIR=build/runtime-host-dsp-tracing \
BLUEWAKE_ENABLE_DEVELOPER_TRACING=ON \
./scripts/build_macos_dsp_host.sh
```

Do not use traced timings as shipping-performance evidence. The exact matched
correctness route must agree before a trace-off speedup is accepted.

## Engineering Principles

- Preserve the authentic retail route before optimizing it.
- Promote required runtime behavior as default-on subsystems, not opt-in fixes.
- Assign algebraically incoherent symptoms upstream to CPU, clock, ABI, or
  translator ownership before tracing farther downstream.
- Separate correctness, performance, and endurance evidence: success in one
  does not imply success in the others.
- Keep all game-derived and user-private material outside Git.
- Prove every iPadOS change in one simulator at a time before any hardware run;
  macOS remains the reference host for traces and timing.

These rules and the anti-stall mechanism are formalized in
[GOAL_LOOP.md](../GOAL_LOOP.md).

## Project Map

| Path | Purpose |
|---|---|
| `runtime/host/` | Native macOS host, device services, and runtime tests |
| `scripts/` | Preparation, audits, generation, verification, and builds |
| `cmake/composite/` | Native composite module build |
| `config/` | Pinned dependency and product configuration |
| `tests/` | Public, game-data-free preparation and composite tests |
| `docs/status/` | Current state, blockers, gates, performance, and decisions |
| `docs/research/` | Architecture research, experiments, provenance, and dependency audits |

## Documentation

- [Finish Line](status/FINISH_LINE.md): prioritized path from working prototype to credible macOS build
- [Current Status](status/CURRENT.md): newest evidence and next action
- [Mods](../MODS.md): widescreen, HD texture packs and Better Wind Waker, and how code mods are built
- [Blockers](status/BLOCKERS.md): owned blocker ledger and reproductions
- [Performance](status/PERFORMANCE.md): benchmarks and optimization evidence
- [Porting History](PORTING_HISTORY.md): chronological engineering narrative
- [Product Requirements](PRD.md): scope, gates, platform policy, and definition of done
- [Goal Loop](../GOAL_LOOP.md): autonomous implementation and anti-stall protocol
- [Legal and Provenance](../research/LEGAL_AND_PROVENANCE.md): source and data boundaries

## Legal

BlueWake is an independent research project. It is not affiliated with or
endorsed by Nintendo. *The Legend of Zelda*, *The Wind Waker*, GameCube, and
related names are trademarks of their respective owners. You are responsible
for complying with the laws that apply to your own software and disc image.
