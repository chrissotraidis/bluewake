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
- 2026-11-25: FXAA's bench on the owner's Deck, as far as it got: Game Mode, the owner's settings with both #655's and #658's switches on, twelve states, `render_bench --paced`, build `5941dffe9`, 3x as the base against 2x with the smoothing Off, Low, Medium and High. Medians of three launches a scene for 3x, Off and Low, three or four for Medium, four for High.

  **The Deck was docked:** Steam's display log shows it change from its own 1280×800 screen to an external 1920×1080 one at 22:03:56, eight seconds before the first launch, and stay there. So these numbers are for a 1080p screen, where 2x (1707×960) is below the screen's size and 3x (2560×1440) above it, not for the Deck's own screen as planned. They also explain 3x here being about 10 % slower than in #655's and #658's runs that afternoon on the Deck's screen with the same settings (Big Octo 23.6 game frames a second against 26.2): both switches' fast paths show in these logs, nothing else used the processor, and the two builds differ only by the smoothing code.

  **Game frames a second** (the change against 3x in parentheses):

  | Scene | 3x | 2x, Off | 2x, FXAA Low | 2x, FXAA Medium | 2x, FXAA High |
  |---|---|---|---|---|---|
  | Outset | 22.9 | 28.1 (+22.7 %) | 27.9 (+21.8 %) | 28.1 (+22.3 %) | 28.1 (+22.4 %) |
  | Lava room | 29.6 | 29.8 (+0.8 %) | 29.9 (+0.9 %) | 29.9 (+0.9 %) | 29.9 (+1.0 %) |
  | Hyrule Castle | 29.1 | 29.9 (+2.8 %) | 29.9 (+2.8 %) | 30.0 (+2.8 %) | 29.9 (+2.8 %) |
  | Butterfly room | 29.2 | 30.8 (+5.3 %) | 30.9 (+5.7 %) | 30.9 (+5.7 %) | 30.9 (+5.7 %) |
  | Shipwreck | 26.3 | 29.8 (+13.0 %) | 29.8 (+13.1 %) | 29.9 (+13.5 %) | 29.8 (+13.1 %) |
  | Outset: play | 27.4 | 29.7 (+8.6 %) | 29.8 (+8.9 %) | 29.8 (+8.9 %) | 29.7 (+8.5 %) |
  | Outset: heavy | 26.0 | 29.9 (+15.2 %) | 29.8 (+14.9 %) | 30.0 (+15.4 %) | 29.8 (+14.8 %) |
  | Outset: busiest | 23.3 | 28.8 (+23.6 %) | 28.6 (+22.7 %) | 28.6 (+22.7 %) | 28.5 (+22.2 %) |
  | Rain, Windfall | 26.9 | 29.8 (+10.7 %) | 29.8 (+10.7 %) | 29.8 (+10.9 %) | 29.9 (+10.9 %) |
  | Big Octo | 23.6 | 30.0 (+27.0 %) | 30.0 (+27.0 %) | 29.9 (+26.7 %) | 30.0 (+27.0 %) |
  | Lava geyser | 26.2 | 29.8 (+13.9 %) | 29.8 (+13.8 %) | 29.8 (+13.9 %) | 29.9 (+14.1 %) |
  | Dragon Roost ash | 24.4 | 29.8 (+21.9 %) | 29.8 (+22.2 %) | 29.8 (+22.2 %) | 29.7 (+21.8 %) |

  **GPU time a shown frame, ms** (the change against 3x in parentheses):

  | Scene | 3x | 2x, Off | 2x, FXAA Low | 2x, FXAA Medium | 2x, FXAA High |
  |---|---|---|---|---|---|
  | Outset | **23.5** | **16.0** (−31.9 %) | **16.1** (−31.4 %) | **16.1** (−31.6 %) | **16.0** (−31.7 %) |
  | Lava room | **17.3** | **11.4** (−34.2 %) | **11.8** (−31.7 %) | **12.9** (−25.7 %) | **12.3** (−28.8 %) |
  | Hyrule Castle | **19.0** | **14.6** (−22.8 %) | **14.2** (−25.4 %) | **15.5** (−18.3 %) | **14.9** (−21.5 %) |
  | Butterfly room | **22.5** | **14.2** (−37.2 %) | **16.0** (−28.9 %) | **16.3** (−27.7 %) | **16.7** (−25.8 %) |
  | Shipwreck | **20.6** | **13.4** (−35.1 %) | **15.0** (−27.4 %) | **14.1** (−31.9 %) | **15.5** (−24.9 %) |
  | Outset: play | **19.3** | **12.4** (−35.7 %) | **16.3** (−15.3 %) | **13.3** (−31.1 %) | **14.0** (−27.6 %) |
  | Outset: heavy | **23.5** | **18.3** (−22.1 %) | **16.8** (−28.7 %) | **16.2** (−31.1 %) | **16.5** (−30.0 %) |
  | Outset: busiest | **22.7** | **15.9** (−30.0 %) | **15.9** (−29.7 %) | **15.7** (−30.9 %) | **16.1** (−29.2 %) |
  | Rain, Windfall | **20.1** | **13.7** (−32.0 %) | **15.8** (−21.4 %) | **16.3** (−19.3 %) | **17.7** (−12.1 %) |
  | Big Octo | **26.8** | **17.9** (−33.2 %) | **17.6** (−34.3 %) | **17.9** (−33.3 %) | **17.5** (−34.8 %) |
  | Lava geyser | **22.0** | **15.5** (−29.5 %) | **16.4** (−25.5 %) | **17.4** (−20.7 %) | **16.5** (−25.0 %) |
  | Dragon Roost ash | **19.1** | **14.7** (−23.3 %) | **16.4** (−14.4 %) | **15.5** (−19.0 %) | **15.6** (−18.4 %) |

  **The pass's own GPU time, ms a shown frame at 2x** (its GPU profiler zone, the real frame's and the in-between frame's together):

  | Scene | FXAA Low | FXAA Medium | FXAA High |
  |---|---|---|---|
  | Outset | 0.56 | 0.64 | 0.78 |
  | Lava room | 0.66 | 0.75 | 0.85 |
  | Hyrule Castle | 0.76 | 0.88 | 1.21 |
  | Butterfly room | 0.62 | 0.88 | 1.10 |
  | Shipwreck | 0.70 | 0.73 | 0.95 |
  | Outset: play | 0.83 | 1.05 | 1.07 |
  | Outset: heavy | 0.82 | 1.11 | 1.19 |
  | Outset: busiest | 0.61 | 0.65 | 0.76 |
  | Rain, Windfall | 0.71 | 0.87 | 0.95 |
  | Big Octo | 0.72 | 0.73 | 0.85 |
  | Lava geyser | 0.72 | 0.78 | 0.92 |
  | Dragon Roost ash | 0.82 | 0.93 | 1.06 |

  Against 3x, 2x takes 22 to 37 % off the GPU time and brings ten of the twelve scenes to full speed; Outset and Outset: busiest stay at 28 to 29. The smoothing changes game frames a second by 0.3 or less at any level. Its own GPU time is 0.56 to 0.83 ms at Low, 0.64 to 1.11 at Medium and 0.76 to 1.21 at High: over the 1 ms the brief asks for at High in five scenes and at Medium in two. The whole frame's GPU time with the smoothing on moves both ways against Off (−2.4 to +3.9 ms) with no order between the levels: with headroom at 2x the GPU's clock follows the load, so that column does not show the pass's cost; the zone does.

  The run stopped itself short. At 00:02 the Deck's network dropped for a moment, and every launch after it waited behind Steam's "Unable to Sync" dialog (Play anyway / Cancel), 64 s each. Cancelling the dialog from here did not help (the next launch raised it again); accepting it for the owner was not done. Stopped at 00:10 after 193 good launches and 7 failed: the fourth pass of 3x, Off and Low and the five picture passes were not run. The Deck was left on its normal build and settings.
- 2026-11-25: pictures on a desktop in place of the Deck's: every state at 3x, 2x, and 2x with each of the six modes, from the SMAA build (`e40240187`), headless, the owner's settings but without the HD texture pack (its folder is on the Deck). 96 runs, all ended cleanly with four captures each and no validation errors; one (Big Octo, FXAA Medium) captured a retrace late and was taken again. Each frame is shrunk to 1280×720, as the Deck's own screen shows a 16:9 picture, and compared with 3x's. A sheet a scene: 3x, 2x Off, FXAA Low, Medium, High, SMAA Low, Medium, High, and under each its difference from 3x, eight times stronger.

  ![Outset: 3x, 2x Off, FXAA Low, Medium, High, SMAA Low, Medium, High, and each one's difference from 3x](images/sheet-01-outset.jpg)

  ![Lava room: 3x, 2x Off, FXAA Low, Medium, High, SMAA Low, Medium, High, and each one's difference from 3x](images/sheet-02-lava-room.jpg)

  ![Hyrule Castle: 3x, 2x Off, FXAA Low, Medium, High, SMAA Low, Medium, High, and each one's difference from 3x](images/sheet-03-hyrule-castle.jpg)

  ![Butterfly room: 3x, 2x Off, FXAA Low, Medium, High, SMAA Low, Medium, High, and each one's difference from 3x](images/sheet-04-butterfly-room.jpg)

  ![Shipwreck: 3x, 2x Off, FXAA Low, Medium, High, SMAA Low, Medium, High, and each one's difference from 3x](images/sheet-05-shipwreck.jpg)

  ![Outset: play: 3x, 2x Off, FXAA Low, Medium, High, SMAA Low, Medium, High, and each one's difference from 3x](images/sheet-06-play.jpg)

  ![Outset: heavy: 3x, 2x Off, FXAA Low, Medium, High, SMAA Low, Medium, High, and each one's difference from 3x](images/sheet-07-heavy.jpg)

  ![Outset: busiest: 3x, 2x Off, FXAA Low, Medium, High, SMAA Low, Medium, High, and each one's difference from 3x](images/sheet-08-busiest.jpg)

  ![Rain, Windfall: 3x, 2x Off, FXAA Low, Medium, High, SMAA Low, Medium, High, and each one's difference from 3x](images/sheet-09-rain-windfall.jpg)

  ![Big Octo: 3x, 2x Off, FXAA Low, Medium, High, SMAA Low, Medium, High, and each one's difference from 3x](images/sheet-10-big-octo.jpg)

  ![Lava geyser: 3x, 2x Off, FXAA Low, Medium, High, SMAA Low, Medium, High, and each one's difference from 3x](images/sheet-11-lava-geyser.jpg)

  ![Dragon Roost ash: 3x, 2x Off, FXAA Low, Medium, High, SMAA Low, Medium, High, and each one's difference from 3x](images/sheet-12-drc-ash.jpg)

  How close each is to 3x at 1280×720, PSNR in dB (higher is closer; the change against 2x Off in parentheses):

  | Scene | 2x, Off | FXAA Low | FXAA Medium | FXAA High | SMAA Low | SMAA Medium | SMAA High |
  |---|---|---|---|---|---|---|---|
  | Outset | 37.4 | 37.7 (+0.3) | 37.7 (+0.3) | 37.7 (+0.3) | 37.7 (+0.3) | 37.7 (+0.3) | 37.8 (+0.4) |
  | Lava room | 37.8 | 38.0 (+0.2) | 38.0 (+0.2) | 38.0 (+0.2) | 38.0 (+0.2) | 38.0 (+0.2) | 38.0 (+0.3) |
  | Hyrule Castle | 42.8 | 42.2 (−0.6) | 42.2 (−0.6) | 42.1 (−0.7) | 42.7 (−0.1) | 42.4 (−0.4) | 42.5 (−0.2) |
  | Butterfly room | 44.5 | 46.7 (+2.2) | 46.8 (+2.3) | 46.8 (+2.2) | 47.1 (+2.5) | 47.1 (+2.6) | 47.0 (+2.5) |
  | Shipwreck | 43.6 | 43.7 (+0.1) | 43.7 (+0.1) | 43.7 (+0.1) | 43.7 (+0.1) | 43.8 (+0.2) | 43.8 (+0.2) |
  | Outset: play | 41.0 | 41.0 (−0.1) | 41.0 (−0.1) | 40.9 (−0.1) | 41.2 (+0.2) | 41.2 (+0.1) | 41.2 (+0.2) |
  | Outset: heavy | 42.4 | 42.3 (−0.1) | 42.2 (−0.2) | 42.2 (−0.2) | 42.6 (+0.2) | 42.5 (+0.1) | 42.7 (+0.3) |
  | Outset: busiest | 38.2 | 38.2 (+0.0) | 38.2 (+0.0) | 38.2 (−0.0) | 38.4 (+0.1) | 38.3 (+0.1) | 38.4 (+0.2) |
  | Rain, Windfall | 45.1 | 45.1 (−0.1) | 45.1 (−0.0) | 45.1 (−0.1) | 45.1 (−0.1) | 45.2 (+0.0) | 45.2 (+0.0) |
  | Big Octo | 47.1 | 47.1 (+0.0) | 47.1 (+0.0) | 47.0 (−0.1) | 47.2 (+0.2) | 47.3 (+0.2) | 47.3 (+0.2) |
  | Lava geyser | 41.5 | 41.6 (+0.1) | 41.6 (+0.1) | 41.6 (+0.1) | 41.7 (+0.2) | 41.7 (+0.2) | 41.7 (+0.2) |
  | Dragon Roost ash | 43.8 | 43.8 (−0.0) | 43.8 (−0.0) | 43.7 (−0.1) | 43.9 (+0.1) | 43.8 (−0.0) | 44.1 (+0.3) |

  At the Deck's screen size, plain 2x is already within 37 to 47 dB of 3x, and the smoothing moves that little: FXAA by +0.15 dB on average (it is a little farther from 3x in Hyrule Castle, −0.6 to −0.7), SMAA by +0.3 (nearer in ten or eleven scenes of twelve). The butterfly room is the one scene where both gain clearly (+2.2 to +2.6): large flat shapes with long edges. The shrink to 1280×720 already averages 1.78 drawn pixels into each shown one, which hides most of what the smoothing does at the drawn size (the crops above). The HUD shows in every difference because it is drawn at the render scale too.
- 2026-11-25: the same frames shrunk or stretched to 1920×1080 instead, for a docked Deck: plain 2x is farther from 3x there (40.5 dB on average over the twelve scenes against 42.1 at 1280×720), and the smoothing brings a little more: FXAA High +0.3 dB on average, SMAA High +0.5.

- 2026-11-25: Steam's "Unable to Sync" on every launch came from its controller-layout sync (Steam keeps layouts in its own cloud space, AppID 241100): it failed during last night's network drop and Steam kept that failure until restarted. The owner restarted Steam and the launches went through.
- 2026-11-25: the bench on the owner's Deck, on its own screen (undocked), build `e40240187`, both methods: 3x as the base, then 2x Off, FXAA Low, Medium, High, SMAA Low, Medium, High; two launches a setting and scene (the owner asked for half the earlier runs: the FXAA run's fps had held with three or four), then one picture launch each. 200 of 200 measured launches went through. Medians:

  **Game frames a second** (the change against 3x in parentheses):

  | Scene | 3x | 2x Off | FXAA Low | FXAA Medium | FXAA High | SMAA Low | SMAA Medium | SMAA High |
  |---|---|---|---|---|---|---|---|---|
  | Outset | 25.2 | 29.4 (+16.6 %) | 29.2 (+15.9 %) | 29.2 (+15.8 %) | 29.3 (+16.0 %) | 29.2 (+15.6 %) | 29.1 (+15.4 %) | 29.2 (+15.7 %) |
  | Lava room | 30.0 | 30.0 (−0.1 %) | 30.0 (−0.1 %) | 30.0 (−0.1 %) | 29.9 (−0.1 %) | 30.0 (−0.1 %) | 30.0 (−0.1 %) | 29.9 (−0.1 %) |
  | Hyrule Castle | 30.0 | 29.9 (−0.2 %) | 29.9 (−0.2 %) | 29.9 (−0.2 %) | 29.9 (−0.2 %) | 29.9 (−0.2 %) | 30.0 (−0.2 %) | 29.9 (−0.2 %) |
  | Butterfly room | 31.0 | 31.0 (+0.0 %) | 31.0 (−0.0 %) | 31.0 (−0.0 %) | 31.0 (−0.0 %) | 31.0 (−0.0 %) | 31.0 (−0.0 %) | 31.0 (−0.0 %) |
  | Shipwreck | 29.5 | 30.0 (+1.5 %) | 30.0 (+1.4 %) | 30.0 (+1.4 %) | 30.0 (+1.4 %) | 30.0 (+1.4 %) | 30.0 (+1.4 %) | 30.0 (+1.4 %) |
  | Outset: play | 30.0 | 30.0 (−0.0 %) | 30.0 (−0.0 %) | 30.0 (−0.0 %) | 30.0 (+0.0 %) | 30.0 (−0.0 %) | 30.0 (−0.0 %) | 30.0 (+0.0 %) |
  | Outset: heavy | 29.0 | 30.0 (+3.5 %) | 30.0 (+3.5 %) | 30.0 (+3.5 %) | 30.0 (+3.5 %) | 30.0 (+3.5 %) | 30.0 (+3.5 %) | 30.0 (+3.5 %) |
  | Outset: busiest | 26.1 | 29.6 (+13.6 %) | 29.4 (+12.7 %) | 29.5 (+13.1 %) | 29.5 (+13.0 %) | 29.4 (+12.5 %) | 29.5 (+13.1 %) | 29.5 (+13.0 %) |
  | Rain, Windfall | 29.9 | 29.9 (+0.2 %) | 30.0 (+0.2 %) | 30.0 (+0.2 %) | 29.9 (+0.2 %) | 30.0 (+0.2 %) | 30.0 (+0.2 %) | 30.0 (+0.3 %) |
  | Big Octo | 26.2 | 30.0 (+14.6 %) | 30.0 (+14.6 %) | 30.0 (+14.6 %) | 30.0 (+14.6 %) | 30.0 (+14.6 %) | 30.0 (+14.6 %) | 30.0 (+14.6 %) |
  | Lava geyser | 29.3 | 30.0 (+2.3 %) | 30.0 (+2.2 %) | 30.0 (+2.3 %) | 30.0 (+2.3 %) | 30.0 (+2.3 %) | 29.9 (+2.2 %) | 30.0 (+2.3 %) |
  | Dragon Roost ash | 27.8 | 29.9 (+7.7 %) | 29.9 (+7.7 %) | 29.9 (+7.7 %) | 30.0 (+7.8 %) | 30.0 (+7.7 %) | 29.9 (+7.7 %) | 29.9 (+7.7 %) |

  **GPU time a shown frame, ms** (the change against 3x in parentheses):

  | Scene | 3x | 2x Off | FXAA Low | FXAA Medium | FXAA High | SMAA Low | SMAA Medium | SMAA High |
  |---|---|---|---|---|---|---|---|---|
  | Outset | **20.9** | **13.5** (−35.3 %) | **14.6** (−30.3 %) | **13.8** (−33.9 %) | **14.6** (−30.1 %) | **14.2** (−32.1 %) | **14.3** (−31.4 %) | **14.7** (−29.9 %) |
  | Lava room | **18.5** | **8.9** (−51.9 %) | **9.4** (−49.0 %) | **8.8** (−52.2 %) | **9.2** (−50.0 %) | **9.7** (−47.6 %) | **9.6** (−48.1 %) | **9.8** (−47.0 %) |
  | Hyrule Castle | **16.9** | **11.1** (−34.3 %) | **12.0** (−29.4 %) | **11.9** (−29.8 %) | **12.0** (−28.9 %) | **11.9** (−29.5 %) | **12.1** (−28.5 %) | **11.9** (−29.5 %) |
  | Butterfly room | **20.2** | **12.8** (−36.6 %) | **14.5** (−28.0 %) | **13.0** (−35.4 %) | **13.8** (−31.8 %) | **12.4** (−38.7 %) | **13.0** (−35.3 %) | **13.5** (−32.9 %) |
  | Shipwreck | **18.3** | **12.0** (−34.3 %) | **11.8** (−35.3 %) | **12.5** (−31.8 %) | **13.4** (−27.1 %) | **11.1** (−39.5 %) | **12.0** (−34.7 %) | **12.7** (−30.4 %) |
  | Outset: play | **17.5** | **11.4** (−34.9 %) | **11.4** (−34.8 %) | **11.3** (−35.2 %) | **10.4** (−40.7 %) | **13.4** (−23.8 %) | **11.0** (−37.2 %) | **12.8** (−26.8 %) |
  | Outset: heavy | **20.3** | **13.0** (−35.8 %) | **14.5** (−28.7 %) | **15.1** (−25.7 %) | **13.5** (−33.2 %) | **14.1** (−30.6 %) | **13.0** (−35.7 %) | **14.1** (−30.5 %) |
  | Outset: busiest | **19.2** | **12.1** (−36.9 %) | **12.9** (−32.7 %) | **12.9** (−32.9 %) | **13.5** (−29.8 %) | **13.6** (−28.9 %) | **13.5** (−29.9 %) | **13.5** (−29.8 %) |
  | Rain, Windfall | **16.2** | **9.7** (−40.3 %) | **14.3** (−11.7 %) | **14.0** (−13.7 %) | **11.3** (−30.1 %) | **13.4** (−17.1 %) | **12.2** (−25.0 %) | **11.2** (−30.6 %) |
  | Big Octo | **23.7** | **16.0** (−32.4 %) | **16.1** (−32.3 %) | **16.3** (−31.4 %) | **16.7** (−29.7 %) | **16.8** (−29.0 %) | **16.6** (−30.2 %) | **16.7** (−29.8 %) |
  | Lava geyser | **19.4** | **13.2** (−31.8 %) | **13.4** (−31.0 %) | **15.1** (−22.1 %) | **14.9** (−23.2 %) | **14.7** (−24.1 %) | **14.6** (−24.9 %) | **13.8** (−29.1 %) |
  | Dragon Roost ash | **16.3** | **14.6** (−10.2 %) | **11.0** (−32.2 %) | **11.1** (−31.7 %) | **14.0** (−13.7 %) | **13.4** (−17.4 %) | **11.4** (−30.0 %) | **15.0** (−7.5 %) |

  **The smoothing pass itself, GPU time a pass, ms at 2x** (from the GPU profile's in-between frames, which every profile line carries; with Smooth Motion on it runs twice a game frame, once for the in-between frame and once for the real one):

  | Scene | FXAA Low | FXAA Medium | FXAA High | SMAA Low | SMAA Medium | SMAA High |
  |---|---|---|---|---|---|---|
  | Outset | 0.56 | 0.62 | 0.73 | 0.89 | 0.98 | 1.14 |
  | Lava room | 0.43 | 0.48 | 0.54 | 0.76 | 0.81 | 0.87 |
  | Hyrule Castle | 0.42 | 0.45 | 0.53 | 0.70 | 0.74 | 0.78 |
  | Butterfly room | 0.48 | 0.56 | 0.64 | 0.86 | 0.92 | 1.05 |
  | Shipwreck | 0.47 | 0.53 | 0.62 | 0.74 | 0.81 | 0.89 |
  | Outset: play | 0.50 | 0.57 | 0.64 | 0.81 | 0.89 | 1.00 |
  | Outset: heavy | 0.51 | 0.59 | 0.68 | 0.82 | 0.91 | 1.02 |
  | Outset: busiest | 0.53 | 0.61 | 0.71 | 0.88 | 1.00 | 1.06 |
  | Rain, Windfall | 0.44 | 0.48 | 0.55 | 0.71 | 0.76 | 0.85 |
  | Big Octo | 0.47 | 0.53 | 0.63 | 0.70 | 0.81 | 0.87 |
  | Lava geyser | 0.42 | 0.48 | 0.52 | 0.74 | 0.79 | 0.85 |
  | Dragon Roost ash | 0.53 | 0.60 | 0.70 | 0.85 | 0.96 | 1.02 |
  | **Median** | **0.48** | **0.55** | **0.63** | **0.79** | **0.85** | **0.95** |

  2x takes about a third off 3x's GPU time (half in the lava room) and brings the four scenes that 3x leaves below full speed (Outset 25.2, Outset: busiest 26.1, Big Octo 26.2, Dragon Roost ash 27.8) to it. Neither method costs a game frame a second at any level. The whole-frame GPU time swings by a few ms between launches of one setting, more than the pass costs, so the pass's own time is the figure to compare the levels by: FXAA 0.48 / 0.55 / 0.63 ms (median of the twelve scenes), SMAA 0.79 / 0.85 / 0.95, twice that a game frame with Smooth Motion on.
- 2026-11-25: the first table of the pass's cost added the zones of the per-launch summary, which keeps only the largest; SMAA's real-frame zone and its sub-passes came and went there (counted twice in some cells, missing in others). The table above uses the in-between frame's zone, present in every profile line, over how often it ran.
- 2026-11-25: the Deck's picture launches are not frame-exact (Game Mode, paced): a setting's capture sometimes lands a game frame from 2x Off's, and five of 96 failed when Steam's connection dropped at 10:16. They are not used. The owner's HD texture pack was copied to the desktop and the pictures taken there instead.
- 2026-11-25: pictures of the twelve scenes at 2x with the owner's HD texture pack (all 3,546 replacements loaded), on a desktop, headless, each filter level against 2x Off at 1280×720 (the owner asked to leave 3x out). A capture can land a retrace apart between runs, so each level's frame is the one of its four that matches 2x Off's. Each sheet, left to right: 2x Off, FXAA Low, Medium, High, SMAA Low, Medium, High; under each level its difference from 2x Off, eight times stronger, with its PSNR.

  ![Outset: 2x Off, FXAA Low, Medium, High, SMAA Low, Medium, High, and each one's difference from 2x Off](images/vs2x-01-outset.jpg)

  ![Lava room: 2x Off, FXAA Low, Medium, High, SMAA Low, Medium, High, and each one's difference from 2x Off](images/vs2x-02-lava-room.jpg)

  ![Hyrule Castle: 2x Off, FXAA Low, Medium, High, SMAA Low, Medium, High, and each one's difference from 2x Off](images/vs2x-03-hyrule-castle.jpg)

  ![Butterfly room: 2x Off, FXAA Low, Medium, High, SMAA Low, Medium, High, and each one's difference from 2x Off](images/vs2x-04-butterfly-room.jpg)

  ![Shipwreck: 2x Off, FXAA Low, Medium, High, SMAA Low, Medium, High, and each one's difference from 2x Off](images/vs2x-05-shipwreck.jpg)

  ![Outset: play: 2x Off, FXAA Low, Medium, High, SMAA Low, Medium, High, and each one's difference from 2x Off](images/vs2x-06-play.jpg)

  ![Outset: heavy: 2x Off, FXAA Low, Medium, High, SMAA Low, Medium, High, and each one's difference from 2x Off](images/vs2x-07-heavy.jpg)

  ![Outset: busiest: 2x Off, FXAA Low, Medium, High, SMAA Low, Medium, High, and each one's difference from 2x Off](images/vs2x-08-busiest.jpg)

  ![Rain, Windfall: 2x Off, FXAA Low, Medium, High, SMAA Low, Medium, High, and each one's difference from 2x Off](images/vs2x-09-rain-windfall.jpg)

  ![Big Octo: 2x Off, FXAA Low, Medium, High, SMAA Low, Medium, High, and each one's difference from 2x Off](images/vs2x-10-big-octo.jpg)

  ![Lava geyser: 2x Off, FXAA Low, Medium, High, SMAA Low, Medium, High, and each one's difference from 2x Off](images/vs2x-11-lava-geyser.jpg)

  ![Dragon Roost ash: 2x Off, FXAA Low, Medium, High, SMAA Low, Medium, High, and each one's difference from 2x Off](images/vs2x-12-drc-ash.jpg)

  How far each level moves the picture from 2x Off at 1280×720, PSNR in dB (lower is a bigger change):

  | Scene | FXAA Low | FXAA Medium | FXAA High | SMAA Low | SMAA Medium | SMAA High |
  |---|---|---|---|---|---|---|
  | Outset | 42.6 | 42.5 | 42.4 | 44.0 | 42.9 | 43.5 |
  | Lava room | 48.6 | 48.2 | 48.0 | 49.2 | 48.2 | 48.1 |
  | Hyrule Castle | 55.0 | 54.7 | 54.2 | 60.5 | 56.3 | 57.5 |
  | Butterfly room | 46.9 | 46.5 | 46.4 | 50.0 | 47.7 | 48.2 |
  | Shipwreck | 54.8 | 54.6 | 54.0 | 57.2 | 55.4 | 55.3 |
  | Outset: play | 47.1 | 46.9 | 46.7 | 48.9 | 47.7 | 48.0 |
  | Outset: heavy | 48.6 | 48.3 | 48.1 | 49.9 | 48.7 | 49.2 |
  | Outset: busiest | 43.5 | 43.5 | 43.3 | 44.6 | 43.7 | 44.1 |
  | Rain, Windfall | 53.3 | 53.6 | 53.1 | 62.0 | 56.9 | 56.9 |
  | Big Octo | 53.8 | 53.8 | 53.2 | 56.0 | 54.5 | 54.0 |
  | Lava geyser | 57.3 | 57.2 | 56.9 | 62.0 | 59.5 | 60.0 |
  | Dragon Roost ash | 50.4 | 50.1 | 49.9 | 52.2 | 50.3 | 51.0 |
  | **Median** | **49.5** | **49.2** | **49.0** | **51.1** | **49.5** | **50.1** |

  The changes sit on edges only (palm trunks, flowers, rigging, Link's outline); everything else is untouched. Every level stays 42 to 62 dB from plain 2x: small at the Deck's screen size, which matches what the owner saw in play. FXAA's three levels change about the same amount (High a little most); SMAA Low changes the least, and SMAA Medium and High sit near FXAA.
- 2026-11-25: MSAA as a probe, on the owner's go-ahead ("probe first"). The renderer already supported multisampling but nothing set it (RecompCore #69, `DOL_AURORA_MSAA=4`; WebGPU allows only 1 or 4 samples). The first build stopped the renderer at once: the game's first EFB depth copy ("Depth tex copies from multisampled EFB targets are not supported"; the depth peek had the same stop). A small full-screen pass now writes sample 0 of each pixel to a single-sample depth for those two. With that, twelve scenes at 2x with and without MSAA, the depth-reading HD effects off in both (they skip a multisampled frame), ran cleanly on a desktop:

  | Scene | MSAA 4x against 2x Off (HD effects off in both), dB |
  |---|---|
  | Outset | 36.7 |
  | Lava room | 40.5 |
  | Hyrule Castle | 24.8 |
  | Butterfly room | 45.3 |
  | Shipwreck | 46.7 |
  | Outset: play | 43.1 |
  | Outset: heavy | 43.3 |
  | Outset: busiest | 40.7 |
  | Rain, Windfall | 49.5 |
  | Big Octo | 29.6 |
  | Lava geyser | 47.8 |
  | Dragon Roost ash | 47.1 |

  MSAA moves the picture far more than either filter (37 to 50 dB in ten scenes): it smooths the geometry's edges as it is drawn. Hyrule Castle turns grey under MSAA: the pass that keeps its colour (BlueWake's monotone handling) skips a multisampled frame too, and the game's own black-and-white look shows. Big Octo's difference is mostly its rain, which falls differently between runs. Its GPU cost on the Deck is not measured yet.
- 2026-11-25: MSAA's cost on the owner's Deck, its own screen (Steam restarted by the owner first), build `2373cd97f`: 2x Off against 2x with 4x MSAA, the depth-reading HD effects off in both, two launches a scene, no pictures. 48 of 48 launches went through. Medians:

  | Scene | fps, 2x Off | fps, MSAA 4x | GPU ms, 2x Off | GPU ms, MSAA 4x |
  |---|---|---|---|---|
  | Outset | 30.0 | 28.4 (−5.1 %) | **9.7** | **12.8** (+32 %) |
  | Lava room | 30.0 | 30.0 (+0.0 %) | **6.4** | **10.7** (+66 %) |
  | Hyrule Castle | 29.9 | 30.0 (+0.0 %) | **4.2** | **7.4** (+78 %) |
  | Butterfly room | 30.9 | 30.9 (+0.2 %) | **7.3** | **10.9** (+49 %) |
  | Shipwreck | 29.9 | 29.9 (−0.1 %) | **5.7** | **7.7** (+35 %) |
  | Outset: play | 29.9 | 30.0 (+0.0 %) | **7.3** | **7.9** (+8 %) |
  | Outset: heavy | 30.0 | 30.0 (+0.0 %) | **10.3** | **15.1** (+46 %) |
  | Outset: busiest | 30.0 | 29.0 (−3.4 %) | **9.4** | **11.6** (+23 %) |
  | Rain, Windfall | 30.0 | 29.9 (−0.1 %) | **8.2** | **10.2** (+24 %) |
  | Big Octo | 30.0 | 30.0 (+0.0 %) | **10.8** | **13.3** (+23 %) |
  | Lava geyser | 30.0 | 30.0 (+0.0 %) | **7.6** | **10.4** (+36 %) |
  | Dragon Roost ash | 30.0 | 30.0 (+0.0 %) | **8.8** | **8.6** (−2 %) |
  | **Median** | | | | **+2.6 ms** |

  MSAA adds 2.6 ms of GPU time a frame (median of twelve scenes; up to 4.8 ms in Outset: heavy), three to five times SMAA High's pass, and already costs game frames in Outset (28.4) and Outset: busiest (29.0) with the HD look off. With it on there is less room still, and keeping the HD look under MSAA needs the effects reworked first. Recommendation to the owner: stop at the probe.
