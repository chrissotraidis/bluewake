# Route B Particle Request Census

**Date:** 2026-09-02
**Blocker:** `BW-P4-0096`
**State:** `PASS`

## Exact Play Control

The existing patch 0042 partition retains original play `phase_2` unchanged.
A new public test proves both reached branches: nonzero Stage sync returns
`cPhs_INIT_e` without side effects; zero sync calls stage-info creation first,
reads particle scene 1 from the published STAG owner, stores one returned
command in the scene, and returns `cPhs_NEXT_e`. All 16 public Route B tests
pass in Debug, optimized Release, and strict ASan/UBSan.

## Original Request Owner

Patch 0057 partitions only `dPa_control_c::readScene` from the 1,362-line
original particle unit. Its 13-line body is unchanged. Twelve guard/include
lines give 0.88% adaptation density. The object exports the original method
and has five unresolved symbols: the already-qualified
`mDoDvdThd_toMainRam_c::create`, `sprintf`, and the three standard assert
owners. No J3D or particle-runtime symbol is required by this method.

A private-disc probe separately executes the exact command factory and
promoted host DVD worker for `/res/Particle/Pscene001.jpc`. It publishes a
168,256-byte allocation whose entire contents match an independent AuroraDisc
read in Debug, Release, and strict ASan/UBSan.

The phase test uses a test-only class definition containing the exact six
native pointer fields preceding `mSceneNo` and `mCount`; static assertions fix
those state offsets at 0x30 and 0x31. This gives the object valid C++ lifetime
without raw storage or field mutation. Unchanged `readScene` passes scene
`0xff`, first request, and repeated-controller behavior before exact phase 2
uses the same owner for scene 1.

## Remaining Blocker

The request contract is resolved, but the test-only prefix fixture is not the
production particle controller. Original `dComIfG_play_c::createParticle`
must still allocate a full `dPa_control_c`; its 25 callback members require
vtables that reach clipping, smoke, wind, GX, and emitter methods. Raw storage,
a duplicated production constructor, or direct field mutation would fabricate
ownership and are prohibited.

One full original `d_particle.cpp` compile census stopped in unrelated
JStudio/JGadget errors imported by `dolzel.pch`. `BW-P4-0097` therefore measures
the source with direct particle-owned includes and composes original
`dComIfG_play_c::createParticle` only if the callback/constructor closure is
bounded. Particle scene creation and play `phase_4` stay closed.
