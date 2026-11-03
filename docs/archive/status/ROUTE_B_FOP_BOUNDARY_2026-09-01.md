# Route B f_op Boundary

**Recorded:** 2026-09-01
**Blocker:** `BW-P4-0073`
**Scope:** macOS source-native Route B

## One-Shot Compile Census

The strict Apple Clang probe covered all 22 translation units directly under
`ref/tww/src/f_op`. Twelve compile unchanged:

- actor and draw iteration/tag units;
- overlap process, manager, and request units;
- scene iteration, pause, request, and tag units; and
- the generic view unit.

Ten stop at the next ownership boundary:

- actor and actor-manager import the whole `dolzel` game PCH and concrete game
  systems;
- camera/camera-manager headers import game camera state and J3D;
- environment/environment-manager import menu, play-scene, and game state;
- message imports play-scene pause state;
- message-manager owns broad J2D, archive, controller, item, and game UI work;
- scene owns the host-I/O root update; and
- scene-manager retains 32-bit integer callback payload casts and JUT assert
  coupling.

The first strict errors are the already classified J3D serialized offset and
pointer-width contracts, GD pointer differences, obsolete `iterator.h` and
`new.h` compatibility includes, `strlen` narrowing, and integer/pointer
payload conversion. No new allocator or process-runtime defect appeared.

## Decision

The 12 passing units are now a tracked default compile gate. The ten failures
are not admitted as one patch batch. The next slice is the thin scene and
camera manager/framework group, with pause/menu/host-I/O represented as
explicit services. Actor manager and message manager remain closed because
they are concrete game/UI owners, not generic process infrastructure.

## Qualified Follow-Up

The thin scene group now compiles through two systematic adaptations: scene
deletion calls a Route B host notification instead of importing host-I/O, and
scene request payloads convert through `uintptr_t`.

The camera manager is represented by a header-light native adapter because it
only stores four process IDs and submits process creation. It does not require
the concrete `dCamera_c` object. Route B diagnostics were also promoted to a
narrow host API after executable linkage proved that the original assertion
umbrella materialized fixed-address GameCube globals in every user.

The resulting tier contains 14 original `f_op` units plus the camera-manager
adapter. Adding only `SComponent/c_request.cpp` closes the project symbol
frontier, and the complete set links with the process runtime and passes an
executable smoke. The unresolved camera owner is now the portable base versus
concrete `dCamera_c` object split.

## Portable Camera Result

The camera header now separates portable `camera_class` process/view state
from the deferred `camera_process_class::dCamera_c` extension under
`TARGET_PC`. The framework body reads menu and scene-pause policy through two
host callbacks and consumes its append payload as the first `u32` it actually
uses, without importing actor-manager definitions.

The donor-backed undefined paused-execute return is initialized to false.
Focused coverage verifies suppressed and normal draw/execute paths. The linked
tier now contains 15 original `f_op` units plus the camera-manager adapter. No
concrete camera or J3D resource layout entered the gate.
