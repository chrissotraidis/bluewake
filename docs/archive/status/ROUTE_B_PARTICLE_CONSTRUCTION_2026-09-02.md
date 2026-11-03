# Route B Particle Controller Construction

**Date:** 2026-09-02  
**Blocker:** `BW-P4-0097`  
**State:** `PASS`

## Retail Ownership

Patch 0058 admits the complete original `d_particle.cpp` with direct
source-owned includes and partitions unchanged
`dComIfG_play_c::createParticle` from `d_com_inf_game.cpp`. The broad
`dolzel.pch` experiment remains closed. A `TARGET_PC`-only `inline` qualifier
fixes seven callback template specializations that otherwise violate the C++
one-definition rule as soon as particle headers cross translation units; the
PowerPC source form is unchanged.

The play method is compiled against a bounded declaration containing its
`mParticle` owner field; the allocated `dPa_control_c` is the complete original
class. A control compile against the full play aggregate repeats the already
closed JStudio/JGadget portability errors before reaching this method. This
tier therefore proves the retail allocation transition and complete particle
lifetime, not full native `dComIfG_play_c` aggregate admission.

The first full-unit census reported 124 unresolved symbols. Dead stripping a
real construction executable reduced that to 40: 22 GX calls and 18 callback,
environment, assertion, and static-data owners. Those calls are reachable
only through callback execution and are explicit aborting fences in this
construction test. The original callback methods and all 25 embedded callback
objects remain compiled; no particle calculation or drawing is claimed.

## Acceptance

The unchanged play method allocates an actual `dPa_control_c`. Its unchanged
constructor requests exactly `0x16e800` bytes at alignment zero, stores the
returned heap, initializes scene number `0xff`, count and simple count zero,
clears the common/scene resource and emitter-manager pointers, and constructs
all 25 real `dPa_simpleEcallBack` members with null emitters and zero counts.

Debug, optimized Release, and strict ASan/UBSan each pass all 17 public Route B
tests. Patch replay passes. Patch 0058 changes 40 of 3,622 compiled original
source/header lines, for 1.10% adaptation density.

## Next Boundary

Construction does not provide a usable particle system. During the original
logo lifecycle, `/res/Particle/common.jpc` is loaded into the controller heap
and logo deletion calls `dPa_control_c::createCommon`. That method constructs
the common `JPAResourceManager`, the 3,000-particle/150-emitter/200-field
`JPAEmitterManager`, and the particle model controller. `BW-P4-0098` must
replace the construction test's heap token with an authentic JKR solid heap,
decode the exact common archive through original JParticle owners, and prove
those counts and owners before scene creation. Play `phase_4`, particle
calculation/drawing, and concurrent processes remain closed.
