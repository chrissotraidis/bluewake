# BlueWake Execution Route Census

**Date:** 2026-09-01  
**Scope:** PRD section 9.6 donor/whole-route review after translated C and LLVM

## Decision

Reject the two pinned alternate static-recompiler donors as BlueWake execution
bases. Select Route B, source-native TWW over Aurora, for one bounded macOS
boot-spine feasibility gate. This is not yet a declaration that the whole game
can move to Route B today. The accepted Route A app remains the behavioral and
device oracle while the source-native gate is measured.

## Observed Census

### Current Route A

The signed app compiles, boots retail code, reaches Outset, renders, executes
authentic DSP work, and preserves the copied card. It remains the only current
full-behavior route. Its source-correct hybrid O2 execution still runs about
235 seconds of guest time in 343.88 seconds, and all correctness-aware
generated-edge mechanisms failed to turn their raw turn reduction into an
acceptable exact product-speed route. LLVM v27 also failed its one authorized
current-host qualification.

### GameCubeRecompiled (`d380537`)

- The repository advertises Apple Silicon output, but its checked-in Apple
  Cargo configuration forces `-fuse-ld=lld`; a fresh core test build fails
  before compiling project code because Apple Clang rejects that linker name.
- No REL or relocatable-module implementation is referenced by the core,
  runtime, CLI, game, or generated crate.
- The checked-in generated crate is an empty image/dispatcher placeholder.
- Memory load/store code generation is explicitly placeholder code, unknown
  instructions can emit untranslated comments, and pipeline failures can emit
  callable stubs.

**Disposition:** reject. Fixing the linker would not supply Wind Waker's 415
REL modules, current cycle/device semantics, or a generated game.

### sp00nz `gcrecomp` / `ww`

- The toolkit is 53 tracked files with 39 source/header files and is designed
  around Windows, D3D11, XInput, and XAudio2.
- Its runtime labels threads, sleeping, wakeup, mutexes, and message queues as
  simplified stubs; non-Windows audio is a no-op stub.
- The Wind Waker repository explicitly describes its result as a renderer, not
  the game. It registers two selected RELs, leaves external-module relocations
  zero, and contains forced frame/process behavior.

**Disposition:** reject. It repeats generated-C execution with substantially
less authentic runtime, graphics, audio, and REL evidence than BlueWake.

### Source-Native TWW + Aurora/Dusklight

- The pinned current TWW configure manifest contains 592 matching, 82
  nonmatching, and 3 equivalent object declarations.
- Static inspection finds 4,335 `/* Nonmatching */` markers. The later
  executable manifest distinguishes 4,175 syntactically empty marked bodies
  from non-byte-matching functions that already have source.
- The process framework, resource/scene/save paths, Link implementation, and
  retail REL infrastructure are substantially source-readable.
- Dusklight proves that the related decompilation architecture can build a
  native macOS app over Aurora with statically linked REL source. It is a
  recipe, not drop-in code: Twilight Princess uses JAudio2 and different game
  structures, while Wind Waker uses JAudio1 and still has source gaps.

**Disposition:** select only a boot-spine feasibility gate. Route B is the only
candidate that removes translated guest dispatch by construction and has a
credible Apple product architecture, but its missing behaviors preclude an
unqualified whole-game pivot.

## Bounded Gate

Produce a deterministic source/link manifest for the macOS path from native
entry through Aurora initialization, logo/title, file select, and the first
Outset scene request. Classify every required object and symbol as:

- source-ready and portable;
- source-present but portability work required;
- empty/missing behavior;
- replaced behind an existing Aurora/BlueWake service; or
- intentionally deferred outside the boot spine.

The gate passes only if the spine has no unexplained missing behavior and a
bounded implementation plan exists for JAudio1 and static REL registration.
It fails if progress requires thousands of empty-body implementations,
fine-grained calls between host-native objects and guest-memory CPU state, or
capture-derived initialization.

Do not modify the accepted Route A runtime during this census. Do not treat
native compilation alone as gameplay evidence. No iOS/iPadOS work starts until
the macOS route is stable.

## Manifest Tier Result

The deterministic GZLE01 manifest now covers 1,096 configured objects and
reproduces all 4,335 nonmatching markers plus 4,175 syntactically empty marked
bodies. The declared boot roots contain 181 markers, but matching status is not
missing behavior: machine graphics and three J3D functions already have full
source, and all eight name-scene markers are PAL-only code excluded from
GZLE01. The REL destructor declaration resolves to an existing shared runtime
source alias.

The target-active empty-body residue is 141 functions: 140 in
JAudio1/JAZelAudio and one `J3DModel::calcWeightEnvelopeMtx`. This advances the
gate while making audio the explicit Route B blocker. Next decide whether
those 140 functions form a bounded reconstruction or can be replaced behind a
stable native subsystem API. Do not begin broad source porting until that
boundary and the transitive link closure are explicit. Evidence:
`local-research/evidence/route-b-source-manifest-v1-20260901/`.

The audio tier is now adjudicated separately in
`ROUTE_B_AUDIO_BOUNDARY_2026-09-01.md`. Current upstream lowers the audio gap
from 140 to 133 functions but does not provide the high-level JAudio1 control
layer. Route B retains source-native JAudio1/JAZelAudio ownership and reuses
native DSP/output only below JAS. Independent native ABI work may proceed while
that source closure remains external.
