# DSP Integration Feasibility - 2026-08-25

## Current reproducible macOS build - 2026-08-28

The promoted DSP-enabled macOS build is now:

```sh
scripts/build_macos_dsp_host.sh
```

The script configures the pinned RecompCore donor in Release mode with IPO,
builds its `core` archive, configures BlueWake's host with the donor adapter,
and runs every runnable host CTest. `BLUEWAKE_DSP_DONOR_BUILD_DIR`,
`BLUEWAKE_HOST_BUILD_DIR`, and `BLUEWAKE_BUILD_JOBS` may override its build
locations and parallelism. The host configuration requires the donor cache to
record IPO and applies ThinLTO only to the adapter, its tests, and host link;
ordinary non-DSP configurations remain available because the adapter itself
is still an explicit build option.

The promotion follows a matched authentic 700-retrace A/B against the current
composite: `32.30s` control versus `30.96s` target-scoped ThinLTO, with the
complete guest-state digest unchanged. It is a build-boundary optimization,
not a DSP behavior change. The earlier `37.07s`/`36.11s` measurement used a
pre-MEM1-fast-path shared composite and is superseded for current performance.
Evidence SHA-256 control/candidate:
`c5f4c19cdf58ca05d6e1c49ee205a307175ea38447199b8303bbb1186acf89c7`
and `ed6056b3c321d2e23f1f23c19e790fc038f0d5d8147f9dc54747a536f0bb0d0e`.

macOS AppleClang compile-only feasibility check for the RecompCore Dolphin DSP
interpreter donor. No simulator, game execution, or private game artifact was
used.

The source sets `ref/recompcore/Source/Core/Core/DSP/*.cpp` and
`ref/recompcore/Source/Core/Core/DSP/Interpreter/*.cpp` pass C++23 syntax and
object compilation with system `fmt`; 20 object files compiled successfully.
Only existing `sprintf` deprecation warnings in `DSPAssembler.cpp` remain.

The exact pinned recursive submodules were restored in the ignored reference
checkout. A full `DSPTOOL=ON` configure and build then completed with AppleClang
on macOS: `libcore.a` compiled the authentic `DSPCore`, interpreter, and
`DSPLLE` host units, and `/tmp/bluewake-dsp-build/Binaries/dsptool` linked as
an arm64 Mach-O executable. Existing deprecation/shadow and deployment-target
linker warnings remain, but there was no compile or link failure. This proves
donor build feasibility, not runtime DSP behavior or BlueWake integration.

The optional tracked adapter now wraps `DSPCore` with source-compatible
`DSPHost` memory/DMA callbacks and is enabled only with explicit donor source
and build paths. Its donor archive is assembled from the DSP, interpreter, and
DSP emitter object set rather than the unrelated full `libcore.a`. The focused
macOS test passes: it initializes the pinned ROM/coefficients, triggers the
real control-register initialization DMA at `0x81000000`, runs interpreter
cycles, and verifies mailbox/IFX behavior. No game input, simulator, or
synthetic audio sample was used.

## DSP source versus audio context state - 2026-08-29

The bounded disc-backed replay `/tmp/bluewake-audio-dsp-context-trace-1m.log`
reaches the authentic `0x80F3D001` task word and then records the DSP source
asserted with PI cause `0x00010040` and PI mask `0x00000FFC`. The live CPU is
not eligible to deliver that external interrupt: at the source and audio
continuation PCs its `MSR` and `srr1` are `0x00003000`, while the current guest
context `0x803E9260` stores `0x00009032`, the interrupt-enabled startup state
from retail `OSInitContext`.

No `__DSPHandler`, `DsyncFrame2`, donor-to-guest DMA write, or nonzero PCM
payload follows. This promotes the next owner to the authentic
`OSLoadContext`/RFI context-restore contract. The host retains the source
assertion behavior because it exposes the real pending DSP source; it does not
force-enable interrupts or inject mailbox words or samples.

## Startup MSR transition localization - 2026-08-29

The bounded replay `/tmp/bluewake-audio-dsp-msr-startup-600k.log` widened the
interrupt-state trace to the whole authentic audio-thread startup.
`AIInitDMA` and callback registration each show coherent disable/restore
pairs, returning to `MSR=0x00009032`. The first unexplained transition is later
at `0x8027A810`, which source symbols identify as the entry of
`JASystem::Calc::initSinfT`, after `Kernel::init` and before `DspBoot`; the CPU
changes from `0x00009032` to `0x00003000` without an intervening traced guest
interrupt primitive. This promotes the CPU_RUNTIME exception/context path
around the translated continuation. No forced interrupt enable, mailbox
injection, sample injection, or simulator was used.

## Scheduler follow-up after REL entry fix - 2026-08-25

The single DSP-enabled macOS host replay used the materialized module-1 alias
and ran for 1,500,000 blocks with `BLUEWAKE_TRACE_RUNQUEUE=1`. It stopped
normally at `pc=0x80307EF4`. The trace records the corrected raw entry alias,
then authentic `SelectThread` entering its idle path with
`run_bits=0x00000000` and `current_thread=0`; the audio thread is reported
idle with `state=0`, no saved PC, and an empty message queue. No JAudio
`DspBoot` at `0x8028E8A0`, `0x80F3xxxx` task words, donor DAC DMA, or nonzero
PCM payload appears. The active owner is now the scheduler/audio-thread
continuation contract. No wake override, injected task word, simulator, or
synthetic audio sample was used.

The adapter also exposes a plain C ABI for the live C host and has a separate C
caller test. Both the C++ and C contract tests pass against the arm64 donor
object archive, including callback ownership and teardown. With the adapter
enabled, the optional macOS `bluewake_host` target now links the C ABI to live
`CPUState` guest-memory callbacks. The explicitly built C++ and C contract tests
pass in that same configured build. This remains a compile/integration result;
no retail replay has yet shown a donor DSP DMA write or nonzero DAC payload.

Next action: replay the retail `DsyncFrame2` route through the attached adapter
and use donor DMA writes to the guest DAC buffers, followed by nonzero PCM
content at the AI boundary, as the oracle. Do not inject samples or use a
simulator.

## Live attachment result - 2026-08-25

The optional DSP-enabled macOS host was run against the private generated
GZLE01 composite for 1,000 authentic blocks with the pinned donor ROM and
coefficients. The host exited normally at `pc=0x8030964C`. The adapter observed
the retail DSP INIT control transition, attempted the source-compatible 0x1000
byte bootstrap DMA from guest `0x81000000`, and reported that the first source
words were `0x00000000 0x00000000`. It therefore did not execute donor cycles
until a nonzero guest IMEM DMA is supplied. No donor DSP DMA-to-guest write,
interrupt, or nonzero DAC payload was claimed. The earlier unguarded probe that
ran empty IRAM reached an invalid donor opcode and exited with SIGSEGV; that
failure is now prevented by the explicit empty-program guard.

Verification: the same configured build links `bluewake_host`; the focused C++
and C adapter tests pass 2/2; the default runtime regression remains green.
No simulator or synthetic audio sample was used.

## Follow-up register and mailbox result - 2026-08-25

Retail maps `CC005034` to the DSP-side DMA start address (`DSP_DSPA`, donor
IFX `0xCD`). BlueWake forwarded the external address and block length but had
omitted this register; the host map now forwards it, and the focused adapter
test asserts the register contract. The change did not by itself produce a
donor task transfer.

A single-host observation was extended to 1,000,000 blocks. The guest reached
the authentic `0x8071FEED` DSP boot response, proving the existing mailbox
bridge remains active, but the donor produced no host-to-DSP task DMA before
the run later stopped at an unrelated unmapped REL address. This rules out a
simple DSP cycle-budget explanation. The next owner is synchronization between
the guest `__DSP_boot_task` mailbox sequence and donor ROM execution; no task
image or PCM data was injected.

## Focused versus live mailbox result - 2026-08-25

The focused adapter test now presents the exact retail boot sequence after
`0x8071FEED`: `0x80F3A001`, the guest `jdsp` address `0x80399420`, the
`0x80F3C002`/`0x80F3A002`/`0x80F3B002` setup words, length `0x1D20`, and
`0x80F3D001`. It observes the donor reading the guest `jdsp` source, proving
the donor ROM task-DMA path in isolation.

The live single-host observation reaches the host-provided `0x8071FEED`
response but emits no `0x80F3xxxx` task words to the live DSP MMIO bridge
before the unrelated REL mapping stop. The next owner is therefore the
authentic guest continuation/mailbox path after the boot response; no live
task image or PCM data was injected.

## Live route address correction - 2026-08-25

The latest 500,000-block live trace shows `0x8071FEED` being consumed by
`__OSInitAudioSystem` at `0x80302F50`. It does not reach JAudio `DspBoot` at
`0x8028E8A0` or emit the later `0x80F3xxxx` task words before the unrelated
unmapped REL stop at block `465447`. The focused adapter handshake test reaches
that task protocol directly and observes the guest `jdsp` read. The live
blocker is therefore authentic progression/REL continuation into the audio
thread, not donor ROM task execution or mailbox word format.

## Follow-up donor execution result - 2026-08-25

The extracted donor had a separate global opcode-template table whose
`DSP::InitInstructionTable()` call was absent from the donor startup path. The
adapter now initializes that table before constructing the live core. This
eliminates the prior reset-vector `0x0092` invalid-opcode/SIGSEGV result while
preserving the authentic ROM reset route.

The rebuilt optional host passed both focused adapter tests (2/2). One
single-host authentic replay with the pinned donor ROM/coefficients ran for
10,000 blocks and stopped normally. It reached retail DSP control/mailbox
traffic and acknowledged an ARAM-backed DSP interrupt. No donor DMA write to
the guest DAC buffer and no nonzero PCM payload were observed. This advances
donor runtime safety and live attachment, but does not close authentic DSP
task command execution or sample production. No simulator or synthetic audio
sample was used.

## Disc-backed JAudio and live donor read boundary - 2026-08-25

The authentic private-disc opening-scene route used the documented analog and
button pulses. It reached `TAudioThread::start` at block `546848`,
`audioproc` at `548184`, and JAudio `DspBoot` at `549759`. The guest emitted
the retail task words `0x80F3A001`, `0x80399420`, `0x80F3C002`,
`0x80F3A002`, `0x1D20`, `0x80F3B002`, and `0x80F3D001`; this closes the prior
question of authentic task bootstrap reachability.

With the donor adapter attached, the live callback observed reads from the
zeroed DAC buffer beginning at `0x806AEEA0`. No `DMAFromDSP` callback,
donor-to-guest DAC write, or nonzero PCM payload appeared in the 1,000,000-
or 5,000,000-block normal replays. The next owner is the donor task's
source-compatible execution and DAC write contract. No wake override, task
injection, simulator, or synthetic sample was used.

## Materialized module-one entry fix - 2026-08-25

The private composite materializes `f_pc_profile_lst.rel` at guest
`0x81E021C0`, making its raw entry `0x81E02294`; the older compatibility alias
only handled the fixed raw address `0x81E000D4`. The host now records the
actual module-1 raw base and aliases `base + 0xD4` to linked entry
`0x81F800D4`.

Normal and DSP-enabled 500,000-block authentic replays pass that boundary and
stop normally at `0x80307EF4`. A clean DSP-enabled 1,000,000-block replay also
stops normally at `0x80307EF4`. The route still reaches no JAudio `DspBoot` at
`0x8028E8A0`, emits no `0x80F3xxxx` task words, and produces no donor DMA to
the DAC buffer. The prior REL mapping stop is closed; the next live owner is
authentic scheduler/audio-thread progression into `DspBoot`. No simulator or
synthetic audio sample was used.
