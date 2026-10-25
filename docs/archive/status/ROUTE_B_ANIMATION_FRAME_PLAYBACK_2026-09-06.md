# Original animation frame playback — 2026-09-06

Base `25966ce`. BW-P4-0126 remains active; P4 is not passed. No external
blocker. This increment composes the original scheduler with the reconstructed
callbacks and real BAS resources, but retains diagnostic lower audio services.
It is not PLAYER integration, audible sound or gameplay acceptance.

## Implementation

Patch 0181 exposes original JAIAnimeSound::setAnimSoundVec,
setAnimSoundActor and playActorAnimSound in a playback composition. Their
bodies are unchanged. The derived setAnimSound wrapper now packs reverb via
an unsigned byte before shifting, preserving the original bits without native
undefined behavior for negative values. Original initialization/stop, both
reconstructed callbacks and original random construction compose together.
The native unit retains disabled float contraction from callback verification.

PLAYER continues using its previous animation-state composition and abort
fences. The new public/private playback targets do not substitute diagnostic
services into PLAYER. Eight test voices observe handle ownership, parameter
requests and release ordering. Immediate stop clears its owning handle; a
fade-one request retains it until an explicit diagnostic tick retires it.
That is an injected service-return model, not actual voice/DSP behavior.
The matrix observer provides identity-camera output and is not matrix-backend
acceptance. Special seStart requests are recorded separately.

## Focused controls and failures

Independent synthetic controls prove:

- Nine simultaneous entries fill eight slots; the ninth is not allocated.
- Active level requests stop/reuse their own slots, and ending frames release
  all slots. An injected failed start leaves a free slot for the next entry.
- A zero-rate wrapper call does not advance time or issue requests.
- Forward wrap increments loop state and replays eligible entries.
- Direction flags suppress allocation; reverse playback safely reaches the
  unsigned counter sentinel and wraps again.
- A loop-limited one-shot starts on its selected loop and is not restarted
  like a level request. Animation replacement can retain a fading old slot,
  followed by diagnostic retirement and null-animation reset.
- Every observed handle is owned by exactly one original scheduler slot,
  and every playing entry pointer belongs to the live BAS buffer.

The first expectation incorrectly gave all simultaneous requests the original
reverb byte. The verified callback mutates their shared actor context: the
first request consumes that high byte, subsequent requests see it cleared.
The control now verifies that behavior instead of changing the implementation.
A second control initially selected end-frame stop flag 0x10 for replacement
fading; source inspection confirmed replacement uses flag 0x04. Correcting the
fixture to loop flag 0x08 plus replacement flag 0x04 makes the intended control
pass. Neither failure justified a behavioral change to the original scheduler.

## Private resource replay

The new headless probe validates GZLE01, reads Lkanm.arc via AuroraDisc,
decompresses/mounts it with original JKRDecomp/JKRMemArchive, and enumerates
both BCK and BCKS through original archive find/readIdxResource methods.
Each embedded BAS is copied into an aligned 0x200 owned buffer using the
already qualified fixed-layout contract; no game data is tracked.

Each resource runs representative start/end boundary frames in both directions
for three cycles, then explicit stop and null-animation reset. These are
diagnostic frame inputs, not recorded normal gameplay or a complete animation
campaign. All modes give the same measured totals:

| Measurement | Result |
| --- | ---: |
| Real BAS resources | 91 |
| BAS records | 186 |
| Original scheduler frame calls | 4,686 |
| Diagnostic start requests | 1,205 |
| Diagnostic stop requests | 1,205 |
| Parameter requests | 16,023 |
| Extra seStart requests | 0 |

Private totals are regression observations, not an independent retail oracle
for the entire scheduler. The preceding callback instruction oracles and the
independent public controls provide separate evidence. This corpus does not
exercise every special seStart branch, which remains covered only by the
preceding isolated request suite. All diagnostic voices are released before
their resource buffers leave scope; this is not full JAI heap recovery proof.

## Regression and provenance

- Public CTest **79/79**, Debug, Release and strict ASan/UBSan.
- Prior extended callback oracles replay: **11,076 parameter** and **12,880
  emission** cases per mode.
- New private playback and accumulated private probes rebuild/replay in all
  modes: PLAYER init/model/identity, camera run/event/constructor/matrix/mass,
  sea, attention, DZB, Toripost heap, MCA morph, BG profile, Stone2 and room
  lifecycle. Link still only has phase-two acceptance.
- Production link censuses remain **4 / 41 / 52** in every mode, with the
  expected linker exit 2. No phase-three execution occurred.
- Player asset tests 10/10; TWW preparer through 0181 and Aurora preparer pass.
  Protected recompcore and dependency-lock SHA-256 hashes remain unchanged.
  Repository audit has only its known 20 tracked local-research files.
  Its output is identical to the preceding checkpoint's audit; diff checks
  are clean.
- No interpreter or request observer is added to the game runtime. No pin,
  protected source, private asset, save, mobile target or GUI process changed.

Reproduce the focused public target with
`bluewake_route_b_animation_playback_test`; run
`bluewake_route_b_private_animation_playback_probe <private-gzle01-iso>` in
the corresponding Aurora-GX build. Strict environment:
`ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1`.
Ignored evidence alias: `route-b-animation-frame-playback-20260906`.

## Next integrated ownership boundary

Source inspection confirms the lower gap is a subsystem, not a single missing
callback: LinkSound::getSound/releaseSound, JAISound::initParameter,
SeMgr::getSeParametermeterPointer/releaseSeParameterPointer/storeSeBuffer/
releaseSeBuffer, and JAIBasic start/stop dispatch have missing bodies.
SeMgr's frame scheduling and several lower setters are also incomplete.
SoundTable has original source but reads serialized u16/u32 data in host order.
Empty constructor bodies must not automatically be classified as missing:
SeParameter contains member arrays whose constructors can own real behavior.

Next reconstruct and qualify the sound/SE pool registration lifetime together
with canonical native handle/actor ownership, using verified DOL evidence and
bounded pool exhaustion/release/reuse tests. Qualify constructor/member state
and sound-table byte order as required by that owner. Then replace diagnostic
voice responses with real start/stop/parameter services and continue remaining
runtime closure to original makeBgWait on real Room44 ground, followed by
continuous Link/camera/input/draw. Do not use isolated symbol counts or test
voice assignment as the gameplay milestone.
