# Progress — Edge smoothing (FXAA) for lower render resolutions

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md)

- 2026-11-24: the owner found the game runs much better at 2x on the Deck and saw barely any difference from 3x. The sizes worked out from the renderer's code (2x 1707×960, 3x 2560×1440, shown at 1280×720). FXAA chosen over SMAA for the first step; SMAA and multisampling noted as follow-ups.
- 2026-11-24: the code: RecompCore #69 (`aurora_set_fxaa`, `DOL_AURORA_FXAA`, levels 0 to 3), pinned here; the Display chooser "Edge smoothing (FXAA)" and its test entries. `settings_table_test` passes locally. Not yet built, run or measured: the shader has not been through the GPU's compiler.
- 2026-11-24: built (`5941dffe9`: main at 0.13.0 + #642 + #643 + #655 + #658 + this branch, RecompCore #68's head with #69's commit on top, two local diagnostics; an in-place rebuild of the trained test build) and run on a desktop from the Outset state, headless, High against Off at 2x and 3x plain. The shader compiles and the pass runs: no validation errors, the frames are 1707×960 at 2x and 2560×1440 at 3x as worked out, and the GPU profile shows "Smooth edges (FXAA)" at 0.05 ms a frame on the desktop's GPU (not the Deck's). The HUD is unchanged in the captures. Not yet run on the Deck.
- 2026-11-24: the same desktop run at Low and Medium too, for pictures in the owner's order: 2x, 3x, 2x with Low, Medium and High. The whole frame, each shrunk to 640×360:

  ![Outset: 2x, 3x, 2x with Low, Medium, High](images/desktop-outset-full5.jpg)

  A 240×160 crop of the grass and flowers, three times enlarged (3x shrunk to 2x's size first):

  ![Outset, a crop: 2x, 3x, 2x with Low, Medium, High](images/desktop-outset-crop5.png)

  The stair steps on the grass blades and petals at 2x are gone at all three levels, about as at 3x shrunk; the bush's texture is a little softer than at plain 2x. Pixels changed against plain 2x: Low 3.6 %, Medium 5.9 %, High 6.4 %. The pass's GPU time on the desktop's GPU: 0.045, 0.049 and 0.052 ms a frame.
- 2026-11-24: the bench on the owner's Deck started (22:04): 3x as the base, then 2x Off, Low, Medium and High, twelve states, four measured launches and one picture launch each, on build `5941dffe9`.
- 2026-11-24: SMAA written as a second method (RecompCore #69's second commit, pinned here): the mode is now one value, `DOL_AURORA_EDGE_SMOOTHING` 0 to 6, and the chooser reads Off, FXAA Low to High, SMAA Low to High. `settings_table_test` passes locally. Not built: the owner is using the desktop, and the Deck is benching. SMAA's shader has not been through the GPU's compiler.
- 2026-11-25: SMAA built (`e40240187`: the same test stack with RecompCore #69's two commits) and run on a desktop from the Outset state at 2x, the three SMAA levels and FXAA High. SMAA's shader compiles and its three passes run: no validation errors, "Smooth edges (SMAA)" in the GPU profile at 0.047 to 0.055 ms a frame on the desktop's GPU (edges about 0.011, weights 0.012 to 0.019), against FXAA High's 0.053. A 240×160 crop, three times enlarged: 2x, 3x shrunk to 2x's size, FXAA High, SMAA Low, Medium, High:

  ![Outset, a crop: 2x, 3x, 2x with FXAA High, SMAA Low, Medium, High](images/desktop-outset-crop6-smaa.png)

  SMAA takes the stair steps off the grass blades and petals as FXAA does and leaves the bush's texture nearer plain 2x. Pixels changed against plain 2x: SMAA Low 2.9 %, Medium 3.7 %, High 3.6 %, FXAA High 5.5 %.
- 2026-11-25: one of the eight desktop runs of that build (the first, SMAA Low) ended with a segmentation fault after the game's shutdown summary, with all four captures written; the next launch started in safe mode, so the three runs after it were taken again. Two more SMAA Low runs ended cleanly. Not explained: it may be #656's intermittent crash or SMAA's teardown.
