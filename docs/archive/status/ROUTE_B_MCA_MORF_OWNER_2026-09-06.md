# Route B Toripost McaMorf/BCK Owner - 2026-09-06

## Boundary

The Toripost link census isolated a five-symbol reusable boundary: original
`mDoExt_McaMorf` construction, `play`, `calc`, and `entryDL`, plus original
`dLib_bcks_setAnm`. Linking complete `m_Do_ext.cpp` or `d_lib.cpp` would have
admitted unrelated systems, while an actor-specific replacement would not
advance the shared source runtime.

## Result

Patch 0137 adds opt-in source partitions that retain the McaMorf constructor,
`calc(u16)`, `setAnm`, `setMorf`, `play`, no-argument `calc`, and no-argument
`entryDL`, together with exact `dLib_bcks_setAnm`. It reuses the already
qualified original J3D model-create/model-draw, matrix, animation, joint, and
resource owners. Link-required JAI animation-sound methods are explicit abort
fences, and the probe constructs McaMorf with audio disabled.

The first retail run did not reveal a morph defect. It exposed two upstream
resource defects:

1. The partitioned stage dRes owner converted BDL and BTK resources but left
   `BCKS` entries as raw archive pointers. Patch 0139 admits only the existing
   original BCK/BCKS conversion branch and preserves native-width BAS-data
   pointer arithmetic.
2. BCK transform-key headers, table entries, and scale/rotation/translation
   samples were serialized big-endian. Patch 0138 gives the ANK1 block its
   exact 0x24-byte field layout, endian-aware metadata and offsets, and
   non-mutating big-endian sampling with source-equivalent Hermite
   interpolation.

The exact 9,852-byte `Toripost.arc` now resolves BDL index 9 as a 3-joint,
1-material model and BCKS indices 6 and 4 as 25- and 14-frame animations.
Both animations produce finite joint transforms that change from frame 0 to
frame 1. The original first animation correctly cuts in because construction
leaves `mPrevMorf` negative; the second transition starts at zero and advances
to 0.25 after one `play` call with a four-frame morph. Original `calc` traverses
the root and original `entryDL` submits exactly one packet owned by the model.

Stable output in all three configurations:

`mca-morf archive=9852 model=joints3-materials1 bck=wait25-get14 morph=8+4 frame=1 calc=1 packets=1 audio=closed pass`

## Evidence

- Debug executable SHA-256:
  `d44d8c89af50e25a1e1e9b6eaa24a073b708fe63280fc5254fdb5f5c74a3832f`.
- Optimized Release executable SHA-256:
  `3f00d563b637158fc082574e4ca236bff44bc7e209d25761332819cfdc0701be`.
- ASan/UBSan executable SHA-256:
  `5e1dd968ddc77226fbd2147a6e75273c0c8c063e8e94558a8d75d20d41d5c6e1`.
- Exact McaMorf probe passes in Debug, optimized Release, and strict
  `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1`
  `UBSAN_OPTIONS=halt_on_error=1`.
- Exact Room44 parent, standalone Stone2, and standalone BG probes pass in all
  three private configurations.
- Full default Route B builds and all 65 public tests pass in Debug, optimized
  Release, and strict ASan/UBSan.
- Patches 0001 through 0139 replay cleanly from pinned TWW commit
  `03d27aa14389e648f51df2bc055ee3d53a3b67f2`.
- No BlueWake process or Simulator was launched.

## Limitations

This is a reusable morph/resource-owner proof, not a Toripost profile proof.
The actor's resource phases, solid heap, create initialization, standard
request, passive collision, lighting/shadow calls, and deletion are not linked
here. Audio is deliberately fail-closed and absent. Mail delivery,
talk/present/event, item creation, particles, player crash, collision response,
and non-idle modes remain outside the boundary. BCK transform keys are now
qualified; this does not claim payload sampling for every other J3D animation
format.

## Next Boundary

Promote original `JAIAnimeSound` construction and BAS metadata initialization,
then exercise McaMorf with the actor's unchanged `param_8=TRUE` against exact
Toripost BCKS data. Position-driven playback remains a named fail-closed
boundary because idle Toripost supplies no sound position. After that owner
passes in all three configurations, partition original
`d_a_obj_toripost.cpp` around resource/heap construction, fresh-save WAIT,
stable execute/draw, and delayed teardown before parent composition.
