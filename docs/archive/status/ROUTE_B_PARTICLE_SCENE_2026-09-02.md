# Route B Particle Scene Owner

**Status:** qualified on 2026-09-02.

Unchanged `dPa_control_c::createScene` now executes after the qualified common
owner using authentic `Pscene001.jpc`. A separate 8 MiB game `JKRExpHeap`
backs the source-equivalent max-sized game `JKRSolidHeap`; the scene owner then
adjusts that heap exactly as retail does.

The archive registers 3 emitter resources and 5 textures. Original source
publishes its manager in emitter-manager slot 1 and preserves the exact input
pointer. Independent iteration of `dPa_name::s_o_id` finds zero matches, so
the correct Outset result is seven retained common emitters and zero added
scene-simple emitters. The retained pointers do not change, all counts agree,
and no particle exists or callback fence fires.

Debug, optimized Release, and strict ASan/UBSan pass. Public code did not
change after the already-green 19-test matrix. Logs and hashes are preserved
privately under
`local-research/evidence/route-b-particle-scene-v1-20260902/`.

The next action is a one-time compile/link census of the remaining unchanged
play `phase_4` body. It must normalize owners before implementation; this
scene result does not claim background, collision, demo, stage, viewport,
HIO, attention, vibration, or actor initialization.
