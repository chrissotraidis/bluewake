# Bird-scene draw path, from zeldaret/tww decomp (clone: /home/james/Projects/tww-decomp)

## What the tally's hot calls actually are (source confirmed)

The slowdown-window spike in GXCallDisplayList (~11x) and
J3DFifoLoadPosMtxImm/NrmMtxImm (~14x/~5x) is the J3D full-redraw path:

  J3DShapePacket::draw()            (J3DPacket.cpp:352)
    -> J3DShape::draw()             (J3DShape.cpp:233)
         loadPreDrawSetting()       resets sOldVcdVatCmd = NULL (line 234)
           -> GXCallDisplayList(mVcdVatCmd)  EVERY shape, cache defeated
         drawFast()                 (J3DShape.cpp:206)
           -> for each mtx group: J3DShapeMtx::load()   (matrix calc + FIFO push)
           -> J3DShapeDraw::draw() -> GXCallDisplayList(mDisplayList)

## Root-cause mechanism

J3DShape::draw() sets sOldVcdVatCmd = NULL before loadPreDrawSetting(), so the
VCD/VAT cache (J3DShape.cpp:184) NEVER hits during a full draw: every shape
re-calls GXCallDisplayList even when its VCD/VAT config is unchanged. And
drawFast()'s per-matrix-group loop does a J3DShapeMtx::load() per group, which
runs the full matrix calc (calcTransform) then J3DFifoLoadPosMtxImm/NrmMtxImm
-- 12 and 9 raw f32 FIFO writes with an XF header each (J3DSys.cpp:61-103).

So the bird cutscene cost = the game walking every shape through the full
redraw path every frame: re-emit VCD/VAT display list + recompute + push every
position/normal matrix per matrix group. That is exactly the game-thread bind
the perf logs show (cause=game-thread, gx worker idle).

## Why this is useful to BlueWake (not a source edit)

BlueWake does NOT compile this source; it translates the binary. But these
are exactly the functions native_entries.py hooks, and the decomp gives the
correct reference semantics to write/re-certify the native replacements:

  - J3DFifoLoadPosMtxImm = XF cmd header + 12 f32 FIFO writes (J3DSys.cpp:61)
  - J3DFifoLoadNrmMtxImm = XF header + 9 f32 writes (J3DSys.cpp:78)
  - J3DFifoLoadNrmMtxImm3x3 = XF header + 9 f32 writes (J3DSys.cpp:92)
  - J3DMtxCalc{Maya,Basic,Softimage}::calcTransform (J3DJoint.cpp:141+)
  - J3DShapeDraw::draw = GXCallDisplayList (J3DShapeMtx.cpp:426)

## Actionable insight

1. native_fifo.c / native_mtxcalc.c already target J3DFifoLoad* and
   J3DMtxCalc*::calcTransform. The decomp is the reference to regenerate
   the certified hashes in native_entries.py correctly (issue #179).
2. NEW candidate native hook: J3DShape::loadPreDrawSetting's sOldVcdVatCmd
   cache is defeated by draw() resetting it to NULL. A native/recompiler
   specialization that keeps the VCD/VAT comparison (instead of the guest
   re-walking GXCallDisplayList every shape) could skip most of the
   ~584K display-list calls in the slowdown window. This is the single
   highest-call-count function in the tally and it is a pure state-compare
   + emit -- a strong native-replacement candidate.
3. These JSystem files are SDK framework code (fully matched upstream), not
   the AI-assisted ~21%; safe to use as reference.

## Files (in clone)
  src/JSystem/J3DGraphBase/J3DShape.cpp      (draw path, VCD/VAT cache)
  src/JSystem/J3DGraphBase/J3DPacket.cpp     (J3DShapePacket::draw/drawFast)
  src/JSystem/J3DGraphBase/J3DShapeMtx.cpp   (J3DShapeDraw::draw, load)
  src/JSystem/J3DGraphBase/J3DSys.cpp        (J3DFifoLoad*MtxImm bodies)
  src/JSystem/J3DGraphAnimator/J3DJoint.cpp  (J3DMtxCalc*::calcTransform)
