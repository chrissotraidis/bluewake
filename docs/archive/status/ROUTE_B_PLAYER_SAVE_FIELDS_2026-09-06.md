# Original PLAYER save flags and field dispatch — 2026-09-06

Base `0b8536e`; BW-P4-0126 remains active. Actual PLAYER initialization,
continuous gameplay, disk saving and P4 remain unaccepted.

## Original source composition

PLAYER now links original d_save in-memory owners instead of its previous
actor-query fragment. Compiling the complete unit exposed three implicit
pointer-difference narrowings in memory_to_card, card_to_memory and
initdata_to_card. Patch 0163 makes these unqualified serialization methods
absent from the native runtime tier; it supplies no successful replacement.
Other builds retain their existing source selection. No save algorithm body
is changed. Native card layouts, endian conversion, bounds and persistence
remain a separate required product frontier.

Original JPAField.cpp now joins the existing full PLAYER particle composition.
Its manager and concrete field classes supply the previously unresolved
JPAFieldManager::init. No synthetic implementation is introduced.

## Executed tests

The existing combined private model/mirror/particle/camera probe checks:

- All 240 original switch routes (128 memory, 64 dungeon, 48 zone), including
  query, on, off and both reverse outcomes; two original allocated zones
  prove shared memory/dungeon and isolated room-zone semantics.
- Original sentinel handling for -1 and 255, with no room lookup required.
- Every one of 2,048 event bits through original on/query/off methods.
- Original particle manager dispatch to the concrete drag field for five
  deterministic magnitudes, including its upper clamp, followed by original
  list recycling and empty-list initialization preserving particle state.

Save inputs are typed local state, not restored card data. Room-zone mappings
are restored after the test. Particle inputs are typed emitter/field/particle
fixtures with zero random magnitude; they do not qualify field-resource
admission, random distribution, live particles or draw. The prior global
emitter-info pointer is restored, field lists are detached and emitter
destruction runs. Prior combined assertions remain green.

## Verification and remaining frontier

Debug, Release and strict ASan/UBSan pass the new combined assertions, public
72/72 CTests and existing private camera/event/sea/attention/DZB/Toripost/
McaMorf/BG/Stone2/Room44 and camera constructor/matrix/mass regressions.
Both preparers pass (TWW through 0163, Aurora 0001), as do shell syntax and
whitespace checks. Protected runtime and lock hashes remain unchanged.
Audit still fails only for its known 20 tracked local-research files.

The separate PLAYER creation-only census retains six symbols in every mode:
fopKyM_create, fopAcM_getWaterY, JAIZelBasic::linkVoiceStart,
getLinkVoiceVowel, checkPlayingSubBgmFlag and checkPlayingMainBgmFlag.
Creation plus execute retains 121. These remain deliberate link failures;
counts describe static retention, not a six-task estimate or execution trace.

Next: original water/environment creation owners and remaining audio boundary
toward actual Link initialization. Existing diagnostic water/audio fences
remain fail-on-use. No external blocker; no GUI or Simulator launched.

Ignored logs: `local-research/evidence/route-b-player-save-fields-20260906`.
Build `bluewake_route_b_private_player_model_probe` in an Aurora-GX/DVD tree
and pass the validated disc path. Sanitizer execution uses
`ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1`.
