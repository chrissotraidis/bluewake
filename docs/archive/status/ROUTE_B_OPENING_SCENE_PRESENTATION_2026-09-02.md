# Route B Opening Scene Presentation

**Recorded:** 2026-09-02  
**Blocker:** `BW-P4-0087`  
**Result:** resolved

## Result

One 640x480 Aurora Metal window presents unchanged original
`dScnOpen_proc_c::proc_draw()` at 60 Hz while calling original `proc_execute()`
on alternate presented frames. The complete private-disc run reaches state 44
at scene tick 6,532 and presented frame 13,063, holds the final state for 180
frames, and exits successfully at frame 13,242. Stream prepare/play remain
recorded exactly once at scene ticks 40 and 92; this does not qualify audio.

The headless control remains unchanged: 6,532 draws, every GX packet nonempty,
with packet sizes from 9,792 through 19,936 bytes. Debug, Release, and strict
ASan/UBSan complete the exact state/tick oracle.

## Visual Falsification

The first paced run displayed correct opening artwork and text placement, but
the glyphs were malformed over bright cyan rectangles. This ruled out scene
progression and broad layout failure. The reached `JUtility::TColor` owner
packed and unpacked 32-bit RGBA values through native memory order, preserving
a big-endian PowerPC assumption on little-endian Apple Silicon.

Patch 0040 explicitly maps RGBA channels to the canonical 32-bit color word on
target PC and leaves the GameCube path unchanged. A focused regression proves
that `0x12345678` maps to channels `12/34/56/78` and round-trips exactly. The
second one-window A/B renders the original sentence cleanly without the cyan
rectangles and completes the full timing oracle.

Private screenshots are ignored and retain no disc bytes in the repository:

- Before fix SHA-256: `2ddc412343a1ef99b717d2ecec356ae6962688d6a535c055c3584e9cd4063b3b`
- After fix SHA-256: `05d3706f3ea9f8ef18662b8675d0fafc1e9e6db7c6782efc06f750bc53c37aa5`

Aurora emits its known device-lost warning after deliberate shutdown only.

## Next Boundary

Step 0 corrected an initial naming assumption after this result. Logo requests
`fpcNm_OPENING_SCENE_e`, whose profile is the play-scene owner in
`d_s_play.cpp`; this presented `dScnOpen_c` owns the separate story-scroll
profiles `OPEN_SCENE` and `OPEN2_SCENE`. `BW-P4-0088` therefore begins with a
compile-only census of the ten-phase post-logo play-scene initializer. It may
not substitute a probe-owned story-scroll handoff, reopen the closed logo unit,
add success stubs, or claim audio playback.
