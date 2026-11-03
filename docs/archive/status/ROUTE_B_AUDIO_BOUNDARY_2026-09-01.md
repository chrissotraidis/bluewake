# Route B JAudio1 Boundary Decision

> **SUPERSEDED FOR CURRENT EXECUTION ORDER (2026-09-01):** this census remains
> valid evidence, but custom audio-parser implementation is no longer the active
> tier. `INDEPENDENT_REVIEW_2026-09-01.md` demonstrated that the first native
> frame is blocked earlier by original JKernel/resource/JUT ownership. Resume
> original JAudio admission only after that resource and host-thread spine is
> present; do not promote BlueWake-owned replacements for complete retail
> parsers.

**Date:** 2026-09-01  
**Decision:** reconstruct the JAudio1/JAZelAudio control layer; reuse a native
DSP/output seam only below JAS

## Evidence

The pinned TWW source at `03d27aa` has 140 syntactically empty audio bodies in
the declared GZLE01 boot roots. The current upstream head `94a5ead` was fetched
into an isolated worktree without changing the project pin. It has 133, a
reduction of seven from completed `JASChannelMgr` and `JASBankMgr` work. The
whole configured tree improved from 4,175 to 3,956 empty bodies over the same
interval, but the high-level audio gap remains concentrated in:

- `JAIBasic`, `JAISeMgr`, `JAISequenceMgr`, `JAISound`, and `JAIStreamMgr`;
- `JASBasicWaveBank`; and
- `JAIZelBasic`, `JAIZelAnime`, and `JAIZelAtmos`.

No pinned local donor provides Wind Waker's JAudio1 implementations. Dusklight
and the Twilight Princess source provide JAudio2 classes with different APIs
and object layouts. Substituting those classes would not preserve TWW callers
or native object contracts.

Dusklight does provide a useful lower boundary. Its native audio system retains
the game's JAS/JAI control code, then replaces DSP rendering and SDL output
below `JASDriver`/`JASDsp`. BlueWake's current Route A also has proven donor DSP
execution, PCM draining, and SDL/Aurora output behavior at that lower layer.

## Decision Rules

- Do not transplant JAudio2 high-level classes into TWW.
- Do not bridge native JAudio1 objects to guest-memory Route A code.
- Reconstruct or adopt source for the JAudio1/JAZelAudio control methods whose
  layouts and side effects are visible to native TWW callers.
- Reuse a native DSP/mixer/output implementation only after the JAS channel
  contract is explicit and tested.
- A muted/no-op audio route may be used only as a labeled compile/boot
  feasibility spike. It cannot qualify gameplay, timing, or product audio.

The source gap is bounded but substantial. It makes Route B a strategic
performance route, not an immediate patch for the current app. While upstream
audio closure proceeds, independent work may advance a native ABI/portability
probe for the marker-free process, player, game-state, and REL foundations.

## Startup Reorientation

The logo gate needs a smaller subset than complete gameplay audio, but that
subset is real execution rather than a readiness callback. `m_Do_main.cpp`
runs `mDoAud_Execute()` before `fapGm_Execute()` each frame. `mDoAud_Create()`
loads `JaiInit.aaf` and `JaiSeqs.arc`, initializes JAS/JAI, and publishes the
init flag. `JAIBasic::initInterfaceMain()` schedules initial BankWave loads;
logo phase zero waits until group 2 reaches status 2 through the JAS DVD
completion callback.

The reproducible startup probe in
`config/route_b_audio_startup_probe.json` classifies nine exact owners:

- `JAIBankWave.cpp`, `JASWaveBankMgr.cpp`, and `JASDvdThread.cpp` compile
  unchanged under the strict native ABI;
- `JAIBasic.cpp`, `JAIInitData.cpp`, `JASWaveArcLoader.cpp`, and
  `JASAudioThread.cpp` have bounded size/offset/pointer/message diagnostics;
- `m_Do_audio.cpp` and `JAIZelBasic.cpp` import 282 and 256 dependencies through
  broad PCHs and require startup-only partitioning.

BlueWake now parses the big-endian AAF command stream into native descriptors
with bounds and termination checks. Private qualification observes 7 commands,
65 bank entries, 65 wave entries, and initial-load group 2 in the authentic
540,416-byte file. The authentic 1,211,200-byte sequence archive is a valid
95-file RARC. These facts prove resource structure only. They do not prove JAS
thread startup, wave transfer, DSP work, or audible output.

**Next action:** qualify the four bounded JAI/JAS ports together, partition only
the required machine/Zelda startup functions, and link the retail frame order.
Acceptance requires the real group-2 load path to produce status 2. A forced
status, file-exists result, muted success stub, or test readiness callback is
not admissible.

### Serialized parser diagnostic

The strict probe counts diagnostics, not semantic byte-order debt. Following
the first-wave call chain farther shows that `BankWave::init()` hands AAF
payloads directly to `JASWSParser` and `JASBNKParser`. Their raw structures use
native `u16`, `u32`, `s16`, and `f32` fields for big-endian disc data, and
`JSUConvertOffsetToPtr` forms addresses by narrowing the base through `s32`.
The existing code is coherent on PowerPC and algebraically wrong on ARM64 even
if its visible casts are silenced.

A one-shot bounded view validated the authentic group-2 WS section as one
group with 67 wave records, then was removed before promotion. This confirms
the endian/offset classification without creating a parallel runtime parser.
The eventual original-source port must preserve serialized bytes as serialized
data and use shared width-safe layout types. Pre-swapping an opaque file,
overwriting 32-bit offset slots with host pointers, or treating strict compile
success as runtime qualification remains forbidden.
