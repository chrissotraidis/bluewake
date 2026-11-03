# Animation request protocol reconstruction — 2026-09-06

Base `22dda48`; BW-P4-0126 remains the sole active blocker. This is a verified
subsystem reconstruction checkpoint, not a PLAYER integration or gameplay
milestone. P4 remains open. There is no external blocker.

## Changes and ownership boundary

Patch 0179 reconstructs JAIZelAnime::setSpeedModifySound from GZLE01
0x802ACD34/0x26C: category exceptions, crawling-specific volume, separate float
operations, signed-factor behavior, modular integer addition and signed-16
clamping. Undefined native float-to-integer conversions are avoided using the
retail fctiwz low-word behavior with exceptions disabled.

Patch 0180 reconstructs JAIZelAnime::startAnimSound from 0x802AC888/0x4AC:

- Early scene/flag suppression and singleton-only special requests preserve
  their order relative to later suppression.
- Five stream queries retain short-circuit order, original sound remapping,
  and material restrictions before the packed reverb byte is stripped.
- Typed actor/context/camera fields replace any reliance on retail byte
  offsets. Position is copied before the matrix call; the caller's vector is
  not overwritten. Context is stripped even when distance culling returns.
- Distance uses Aurora's pinned frsqrte helper followed by the original three
  double-precision refinement steps, with contraction disabled for this unit.
  Sound-specific culling exceptions and unordered comparison behavior remain.
- Material/additional requests precede old-handle stop, replacement request,
  and conditional port-9 reverb. Failed replacement does not receive a port
  write. Signed seStart reverb and unsigned port reverb are kept distinct.

Neither callback is admitted to PLAYER yet. The new parameter/emission test
compositions use explicit observation sinks, while PLAYER retains its abort
fences. Native test handle assignment models a service-return contract; it is
not proof of real voice allocation, ownership or teardown. Matrix-call output
is similarly controlled to isolate the reconstructed caller, not to certify
the matrix backend. No ordinary game service is replaced by a silent success.

## Independent evidence

Both Python oracles require the user's private DOL with SHA-1
`8d28bab68bb5078c38e43f29206f0bd01f7e7a67` and Capstone 5.0.6. They decode the
actual instruction ranges and read constants from the DOL. Unsupported
instructions and uninitialized memory reads fail closed. Branches, call
arguments/order, integer narrowing, slot result and actor context are checked.
The emission oracle uses pinned RecompCore reciprocal-square-root semantics
and its hash-checked public table; the native implementation uses Aurora's
separate helper. Neither oracle models FPSCR exceptions or NaN payload identity.
These are bounded diagnostic interpreters, never linked into the game.

- Parameter: 11,072 generated cases plus four hand-computed controls pass.
  Corpus includes independent signed factors, arbitrary pitch/speed float
  encodings, category exceptions, volume bytes and unsigned age extremes.
- Emission: 12,871 generated cases plus nine hand-computed controls pass.
  Includes remap/suppression edges, stream short circuits, high-byte reverb,
  old/reused/new/null slot results, distance exceptions/nonfinite values and
  adjacent float distance thresholds across exponents.
- Public CTest: **78/78**, Debug, Release and strict ASan/UBSan.
- Both extended oracle suites pass in all three modes: **11,076 parameter**
  and **12,880 emission** total comparisons per mode.
- Rebuilt/replayed private matrix passes all modes: PLAYER init, model and
  event identity; camera run/event/constructor/matrix/mass; sea; attention;
  DZB; Toripost heap; MCA morph; BG profile; Stone2; room lifecycle.
- PLAYER init retains real BAS coverage from the preceding checkpoint:
  91 resources, 186 records and 1,662 init/seek comparisons. This is not yet
  playback through the new callbacks.
- Production link censuses remain **4 / 41 / 52** in every mode, intentional
  linker exit 2. Phase three has not executed.
- Player asset tests 10/10; TWW preparer through 0180 and Aurora preparer pass.
  git diff --check passes. Both protected recompcore SHA-256 hashes and the
  dependency lock hash remain unchanged. Repository audit reports only the
  same 20 intentionally tracked local-research files; no new audit failure.

Private logs/corpora: ignored evidence alias
`route-b-animation-requests-20260906`. SHA-256 corpus identifiers:

- Emission: `debddf8fcc358e16c0063b05d7b571b595c46b5a51ed91eb16897f9ef6e4da3f`
- Parameter: `5c0f2f845c62d32a89bbf3e6603487dcfc04b2b3856e800dd09bb1abdd4536a5`

Aurora `include/dolphin/ppc_math.h` is used directly from the pinned MIT
dependency (Luke Street, 2022), not copied into TWW patches. Its SHA-256 is
`1c8d731af920d7f7716627b846b24985355c46ca04d57b2d43737bf8a4ffb718`.
The reference RecompCore table SHA-256 is
`cad871a97bb73b3fba635de85064fa7ec5eded965aa7cb1b32a43c374ba0cea8`.

## Failed attempts retained

The first parameter oracle assumed a single-lane paired save; the encoding
requires two lanes. The emission decoder must override paired-single opcodes
before Capstone's unrelated PPC64 aliases. The initial emission fixture read
the stack-register startup pair as r13; its instruction-encoding assertion
failed before producing evidence, and the actual r2/r13 startup pairs fixed it.
Focused native builds found a missing matrix declaration, an explicit test
count narrowing requirement, and the constructor's random-owner dependency.
The original random.cpp now supplies that dependency in the test composition.
An attempted atomic delete/add of one patch snapshot was rejected by the edit
tool; separate operations succeeded and preparer replay verifies the result.

## Reproduction and next integrated action

Run each oracle with `generated/full/main.dol`, redirecting stdout to an ignored
TSV; pass the TSV to the corresponding `bluewake_route_b_animation_*_test`
binary. Strict environment is `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1`
and `UBSAN_OPTIONS=halt_on_error=1`. Public CTest runs the independent controls
without private input. Dependency preparation and full regression commands
remain the standard Route B three-mode matrix.

Next compose the original JAIAnimeSound frame scheduler/playActorAnimSound and
qualify its eight-slot lifecycle with the reconstructed callbacks and real BAS
resources. Retain explicit backend boundaries: JAIBasic::startSoundDirectID /
startSoundBasic and JAIZelBasic::seStart are incomplete, as are lower JAISound
interpolated setters. Do not confuse protocol observers with those owners.
Then close the remaining audio/runtime dependencies and execute original
makeBgWait with real Room44 ground, followed by continuous Link/camera/input/
draw. Do not resume isolated symbol-count checkpoints as the main plan.
