# Route B KANKYO Profile Creation - 2026-09-02

## Result

The original KANKYO profile now completes one real standard-create request
through the native process runtime. This composes the retail profile and
generic environment manager with the original environment-color, light,
event, sound, wave, and wind initialization graph. It uses the authentic
game-info singleton, room-control state, register HIO, JMath tables, draw-tag
queue, and pinned JAudio singleton.

Patch 0099 promotes the unchanged read-only `isSymbol` and `isEventBit`
methods inside the existing game-info owner. Patch 0100 isolates the unchanged
register-HIO child constructor and real `g_regHIO` static owner. Their focused
tests cover all eight symbol positions, representative event masks including
the last event byte, and every one of the 22-by-30 float and 22-by-10 short
register slots.

The composed test installs the retail profile in the static-REL registry,
creates a real 16-list process layer and draw queue, initializes the same
JMath table required by machine startup, publishes a real non-boss STAG
record, and submits `fopKyM_Create(fpcNm_KANKYO_e)`. The standard-create
handler produces a discoverable live process in `cPhs_NEXT_e`, inserts its
draw tag, clears dirty register values, and publishes the expected
environment, point-light, sound, and ambient-wind defaults.

Delete, execute, draw, and diagnostics remain abort-fenced. The pinned
JAudio reset bodies are still empty `Nonmatching` source and are not claimed
as recovered retail behavior. This milestone also does not qualify KYEFF,
ENVSE, complete `dStage_Create`, or a native gameplay frame.

All 42 public tests pass in Debug, optimized Release, and strict ASan/UBSan.
Patch replay through 0100 passes. Debug SHA-256 values are:

- `d_save.cpp.o`: `28a8e2c067ea15fdc46efee606bd9d958877002970580d5a87f57988ab490a5a`
- register-HIO `d_s_play.cpp.o`: `2ce513f856b2d3f6759d97ae42f66d7ec5b8db6192713184b676430b281cb6a4`
- composed test: `2658f22a76c60438bd46d84d461dcc076398bb169d594115ff026eb2a6599f9d`

## Replay Lesson

The first composition appeared to pass before a clean patch replay, then
correctly failed on missing stage metadata and machine trig tables. A clean
replay established both as authentic predecessor contracts. Future aggregate
tests must be qualified from replayed source before their result is recorded.

## Next Boundary

Return to source order in `dStage_Create`. KANKYO and KYEFF2 creation are now
qualified independently, while KYEFF and ENVSE remain open. The next statement
retrieves authentic `Stage/stage.dzs`, followed by
qualified room-control initialization and the unqualified aggregate stage
loader. Measure that resource/decode closure once and select its smallest
cohesive owner without fabricating successful callbacks or entering room-scene
creation, maps, event management, or lifecycle execution.
