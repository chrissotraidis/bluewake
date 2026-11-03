# BlueWake architecture

This documentation explains how BlueWake is built and how its native host,
native game module, guest state, compatibility services, and graphics
stack work together at runtime.

Start with the [architecture overview](Overview.md), which introduces the
project boundaries and links to the detailed build and runtime documents.

For the native execution path that alternates between host coordination and
translated game code, continue to the [game-thread architecture](GameThread.md).

For how generated translated code, handwritten native replacements, and the
external decompilation relate, continue to [translated, native, and decomp
routines](NativeRoutines.md).

For repository checks, source-only platform suites, private module oracles,
packaging tests, and hardware evidence, continue to the
[test architecture](Testing.md).

For the measurement loop that profiles and compares these components, continue
to the [performance documentation](../performance/README.md).
