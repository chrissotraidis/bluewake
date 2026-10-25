# Retail viewport offset and faint duplicate outlines

The screenshot investigation found a definite viewport decoding bug in
GXCore. The original game's `GXTransform.c::__GXSetViewport` writes XF origin
values as `left + width/2 + 342` and `top + height/2 + 342`. Our GXCore draw
submission subtracted 340, shifting all retail geometry two native pixels
right and down (six output pixels at 3x resolution).

Aurora's native GX API uses a paired 340 encoder/decoder convention. That
convention was copied into the retail FIFO path, where the encoder is the
original game and uses 342. RecompCore `51270de` corrects only GXCore's retail
viewport decoding; the paired Aurora API path remains consistent. This is the
main-branch integration of the correction first tested as `88f4b4c` on the
experimental branch. It preserves the latest main-branch interpolation code
and does not include the native-60-Hz experiments.

Wind Waker's `m_Do_graphic.cpp::drawDepth` copies the scene and depth to
half-size textures, then alpha-blends the scene copy over the existing image.
The misplaced postprocess quad adds another offset to the copied scene. Where
both layers contribute, this can produce a faint second silhouette, matching
the type of artifact in the supplied screenshot. This is independent of
Smooth Motion. The exact screenshot's scene/build/settings have not been
reproduced, so the fix is not a claim to eliminate every possible outline.

Initial validation on the isolated experimental-branch Mac build:

- Rebuilt the host and passed all five draw-merge/viewport tests. The two new
  viewport tests cover retail full-screen coordinates, fractional origins and
  unchanged near/far depth mapping.
- Captured the same Outset frame at 1920x1440, native 30 Hz gameplay,
  interpolation off, packed GPU vertices off, fixed input, before and after.
- The baseline had 11 entirely black columns/rows at the left/top. The corrected
  capture has zero. Both complete runs have 42 identical player-state records.
- Inspected both rendered captures. Local evidence stays under ignored
  `build/profile-60hz/outline-30hz*` and `outline-fixed-30hz*`; frame 900 is the
  paired capture, and both runs finish at retrace 2100.

Main-branch integration validation:

- Applied the same correction to RecompCore `3b65983`, retaining its newer
  interpolation and presentation work. The standalone `gxcore_viewport_tests`
  target passes both viewport regressions, and the Mac host rebuild succeeds.
- Loaded Outset with interpolation off at 1920x1440 and captured retrace 951
  through the host's existing framebuffer readback. The left/top black-border
  counts are both zero, and the run stops normally at retrace 1050. The private
  capture and log remain in ignored `build/profile-60hz/outline-main-capture*`.

This renderer correction does not complete the separate native-60-Hz work.
