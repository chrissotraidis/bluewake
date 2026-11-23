# Wind Waker HD's sky and light, measured against BlueWake (Oct 30, 2026)

Research notes from comparing Cemu (Wind Waker HD, USA) with BlueWake's HD look on Windows (i5-12600KF,
RTX 5070 Ti). The Cemu tools and captures named here are local to that PC and aren't in the repository. HD's data was changed in Cemu's memory only; nothing was saved. Everything here is
measured, unless marked as a guess.

**Oct 31 companion reports** (read these with this file):
- `RESEARCH-20261031-hd-normals-coverage.md` (+ `.csv`): HD keeps the GameCube's positions on every stage (97.5%, 99.1% without Hyroom); per-model coverage, outliers, what a runtime lookup needs.
- `RESEARCH-20261031-hd-shadow-prepass.md`: Nintendo's agl `shadow_pre_pass.sh` from the disc; no facing term in HD's shadow, the grazing term, the blur chain, the material's sharp/blurred mix.
- `RESEARCH-20261031-hd-decomp-lighting.md`: the decomp's lighting beyond these notes: palette float pairs (float 11 a second light in the shadow colour), HD's palette schedule an hour earlier from 17:00, the day/night fade at 06:30/19:30, the per-pixel light pass, sea fog, the "staring at the sun" darkening (Windfall's doorways), what the day/night "presets" really are (depth of field).
- `RESEARCH-20261031-hd-disc-sources.md`: HD's own material shader source (`Cave06_Room0.szs` › `model.sharc`, an older build), the terrain uniforms by name, AAMP v1 params (bloom, DOF, FXAA), the agl library's shader list.
- `RESEARCH-20261031-hd-ao-bloom-edge-spec.md`: HD's SSAO, bloom and rim light, exactly, beside BlueWake's, with port plans and Cemu checks.
- `RESEARCH-20261031-hd-materials.md`: which of HD's 10,576 materials get rim light, bloom and SSAO, and with which values.
- `PLAN-20261031-hd-ao-bloom-rim.md`: the order of the next steps (bloom, AO, rim light, then the material table).

## Setup

- **Cemu 2.6, portable:** `C:\src\tools\cemu\Cemu_2.6`. Settings, packs, the HD save and the log are in `portable\`.
  - **Packs:** stock HD shadows. The Ultra 300% Shadow Resolution pack that came from Arch was removed, because we compare against the original game (backup `settings.xml.with-ultra-shadows`). Still on: Resolution 5160x2160 (21:9), Mega Cheats, Stage Select (BlueWake, L + R + D-Down).
  - **Volume:** TVVolume 0.
- **Tools:** `C:\src\tools\cemu\tour-win\`.
  - `vpad_win.py` (the server) and `pad.py` (the client): a virtual Xbox pad on ViGEmBus. It presses only while Cemu or BlueWake is the foreground window. Before that guard, presses leaked to the desktop and opened the Start menu and the on-screen keyboard.
  - `step.py` and `bwstep.py`: bring the game to the front, send pad commands, take a shot.
  - `f12shot.py`: Cemu's own F12 screenshot. PrintWindow comes out grey because of Vulkan.
  - `cemu_mem.py`: reads and sets HD's clock, `*(0x101F84DC)+0x44`, 15 degrees an hour.
  - `cemu_virt.py`: finds and edits HD's sea Virt (sky) and Pale (`PALE=1`, light) tables in RAM. `restore-all` writes a whole table back from the disc.
  - `wrap_falloff.py`: measures the sky wrap.
- **Matched saves at 10:00 on Outset's bridge** (the default load spawn at this point, with the lookout behind):
  - **Cemu:** quest log 1, "d", saved just after Aryll woke Link.
  - **BlueWake:** quest log 3 on `C:\src\test-builds\hd-fog\cirrus-ql3.card`, a copy of the tour card. A new game through the stage select's Cutscenes > New opening and Aryll's talk, then re-saved after a warp to `sea:44:0` at 10:00.
- **Holding Link still:** crouch (ZR), crawl a moment, keep crouching. About 99% still; one lock of hair moves in the wind.
- **BlueWake runs:** `C:\src\test-builds\hd-fog\run-load.ps1` loads quest log 3 with no warp. Variants:
  - `full`: HD sky models and linear blend on.
  - `noshadow`
  - `kumo`: test pack with the cloud colour set white.
  - `log`: fog log.

  `run-outset.ps1` warps instead, and `run-windfall.ps1` is for the Windfall dock.

## Starting time

- **GameCube:** a new game starts its clock at 165 degrees, 11:00 (`d_save.cpp:54`).
- **HD:** a fresh new game in Cemu read 10:00 (150 degrees) from 10 s after Start through the legend and into the opening. So HD starts an hour earlier.
- **The developers' stage select:** its "Norm" time writes 120 degrees, 08:00 (`d_s_menu.cpp:254`). A log started through the stage select keeps that, because time stands still on Outset.
- **Left for later:** whether BlueWake's HD look should start new games at 10:00.

## The sky palette (Virt): HD's five colours and five floats

- **HD's entry:** each Virt entry is 0x38 bytes, the GameCube's 0x24 plus five floats. The five colours are:
  - kumo, the cloud layers
  - kumo centre, the puffs
  - sky, the dome
  - uso umi, the horizon band ("false sea")
  - kasumi mae, the low front haze
- **The floats scale those colours,** one per colour, in that order. Tested from 0 to 4 at Outset 10:00, entry 21:
  - **0:** the cloud layers, dark at 0 and white at 4.
  - **1:** the puffs. The disc value is 1.1 here.
  - **2:** the dome. At 0 it goes nearly black. It does not light the ground; what moved on the ground was cloud shadows.
  - **3:** the horizon band.
  - **4:** the front haze strip right on the horizon, between the cliffs. No visible change at 0 or 4 from either view.
- **Values:** nearly all are 1.0 in the sea's 37 entries. Only entry 21 (1.1 for the puffs) and entry 23 (1.636 for the horizon band) differ.
- **Cloud colour:** HD multiplies the cirrus by the kumo colour; set to red, the cirrus turned red. The alpha (180 in entry 21) did nothing visible.
- **BlueWake's cirrus at 10:00:** greenish grey, because HD's kumo colour for this hour is mint (187, 255, 170 linear, which is 221, 255, 212 after our gamma).
  - Setting it white over-corrects toward pink. Mid-cloud band in R, G, B: Cemu 191, 245, 245; mint 167, 215, 201; white 184, 214, 226.
  - HD lands between them, and brighter. The clear sky behind is also darker and less cyan in BlueWake: 71, 178, 222 against 113, 205, 237. The two sky switches below help.
- **Two HD sky switches are off by default:** "HD's taller haze and cloud layers" (`BLUEWAKE_HD_SKY_MODELS`) and the linear blend (`BLUEWAKE_HD_SKY_LINEAR`).
  - Without them BlueWake's horizon stays deep blue.
  - Every comparison shot before the `full` runs lacked them, including the Windfall tower numbers below.

## The sky wrap: the haze on the far side of the island

The effect Dale saw in Cemu: the edges of scenery seen against the sky take the colour of the sky behind them.

- **Where:** on the cliff edges by a gap, the far rope bridge and the far rock stacks.
- **It's a mix, not an add.** On the rock, green fell from 115 to 100 as red rose.
- **It follows the hour:**
  - 06:30–07:00: dim lavender, nearly invisible.
  - 07:30: pale.
  - 10:00: cyan.
  - 18:30: orange.
- **It's the same with stock and Ultra shadows,** so it isn't a cover for shadow acne.
- **It reaches far scenery and works in every direction,** so it isn't BlueWake's edge light.
  - BlueWake's edge light (RecompCore `post_fx.cpp`, `kEdgeShader`) is a fixed warm colour (1.0, 0.95, 0.82) x 0.4, toward the sun only, 3 steps of 1.5 px, faded out by 1,500 units, and off when the sun is below about 3 degrees.
- **Link is not lit by any of the five sky colours.** With all five red, his hair and shirt didn't change.
- **Falloff:** the share of the sky colour mixed in, by distance from the sky (per 1080 lines, red-dome pairs at stock shadows):

  | Distance | 1–2 | 3 | 5 | 7 | 10 | 14 | 20 | 28 | 40 | 56 |
  |---|---|---|---|---|---|---|---|---|---|---|
  | Share | .34 | .26 | .23 | .20 | .17 | .12 | .09 | .06 | .035 | .016 |

  - **Fit:** two Gaussian blurs of the sky mask, sigma 4 px and 25 px per 1080 lines, mixed at 0.32 and 0.46 (rms error 0.008).
  - **Not motion:** a repeat with Link still gave 0.41 at 0–4 px, 0.23 at 10–20 and 0.04 at 40–80 (at 2160 lines). Motion-only pairs gave 0.
- **Oct 31: it's HD's bloom, fed by the sky** (Cemu, Outset spawn 0, 10:00, shots in `tour-win\shots\wrap\`):
  - **Bloom blanked** (the detect shader `3a8d0f380931d09b` returning 0): the glow over the far island's edges and Link's hair is gone. The sky falls from about (170, 245, 236) to (137, 228, 215), and the edges turn hard.
  - **Far blur blanked** (`730873462c703e9a` discarding): the glow stays; only the island's outline sharpens.
  - **The mask probe** (the detect writing its luminance-offset input) lights the sky and nothing else in view. The sky materials bloom at offset 0 (`RESEARCH-20261031-hd-materials.md`); lit materials and the sea write "no bloom".
  - **The offset scale is at least 2** (a `step(1.99, …)` probe flooded white), so non-bloom materials can't glow at all.
  - So the light-wrap pass below is **dropped**: porting HD's bloom (a wide, tinted, added chain fed by the sky) gives the wrap. Plan: `PLAN-20261031-hd-ao-bloom-rim.md`.
- **Implementation plan (superseded Oct 31, see above):** a new pass in RecompCore's post effects. Branches are made: RecompCore `codex/hd-sky-wrap` from 259627f (in `C:\src\bluewake\ref\recompcore`), and BlueWake `codex/hd-sky-wrap` from 2b63027 (the main checkout).
  - **Sources:** pixels with no depth (depth 0) whose view ray is above the horizon. Excluding below the horizon keeps the far sea out.
  - **Masks:** premultiplied colour and mask at half size, blurred twice (sigma 4 and 25 per 1080 lines).
  - **Composite:** a mix into pixels with depth, after the exposure and before bloom. Outdoors only, off by default, with its own switch under the HD look.
  - **Open:** the islands' far models also have no depth, so they'd count as sky.

## The light palette (Pale) and Link

- **Entries:** 0x6C bytes, the GameCube's 0x2C plus 16 floats. The first float isn't a float (garbage values). Outset at 10:00 blends entries 1 and 2.
- **What each colour does**, set red one at a time with Link still:
  - **actor_c0:** the actor ambient (`d_kankyo.cpp:3088`, `addcol_amb`). It's not AO.
  - **actor_k0:** the actor diffuse, the sun's colour (`addcol_dif`, `:3122`).
  - **bg0_c0:** the cliffs' shade.
  - **bg0_k0:** the sunlit pier and cliff faces.
  - **bg1_c0:** the sea foam around the dock posts (Dale).
  - **bg1_k0:** the sea.
  - **fog:** the mid-distance cliffs and houses. HD's fog is in this palette, as on the GameCube.
  - **bg2:** unclear from the pier; black against white changed only moving things (palms, wind streaks, foam). It may be foliage or a night-time effect (Dale): come back to it at night, or from a spot with still plants.
  - **bg3:** the houses' lit windows, a night light. At 21:00 (bg3 set in every entry) the windows of both houses go dark at black, white at white and red at red; normally they're orange. At 10:00 its value is 0, 0, 0, and only bg3_c0 showed (the lookout's window, set red).
  - **bg2 at 21:00,** set in every entry to black, white and red: no visible change on Outset from the pier. It isn't used here, or only by models out of this view.
  - **Testing with the screen locked:** Cemu can't be brought to the front, and F12 can't reach it, so the shots fail. With the monitor turned off and the session unlocked, they work.
- **The 16 values after the GameCube's 0x2C bytes** (tested at 10:00, entries 1 and 2, each at 0 and 4):
  - **The first isn't a float. It's a twelfth colour, the shadow colour** (RGB plus a padding byte):
    - morning (entry 1): 73, 95, 134
    - noon (entry 2): 27, 98, 181
    - night (entry 8): 75, 75, 75

    Set red, Link's shadow on the pier, the cliffs' shaded sides and Link's shaded side all turned red. Black darkens them, white makes them pale.
    BlueWake uses one fixed shadow colour, (0.22, 0.41, 0.60) (`post_fx.cpp:55`, from Link's shadow on Windfall's dock at noon), at every hour. HD's changes with the hour. The host could pass it to the shadow pass each frame, blended between entries like the other colours.
  - **Floats 1–11 are brightness multipliers for the eleven colours, in order:**

    | Float | Colour | What it changed at 4 |
    |---|---|---|
    | 1 | actor_c0 | Link, strongly |
    | 2 | actor_k0 | Link, slightly (2.0 in the morning, 2.75 at noon) |
    | 3 | bg0_c0 | the cliffs' shade |
    | 4 | bg0_k0 | the pier and the sunlit cliffs (2.0 in the morning, 3.0 at noon) |
    | 5 | bg1_c0 | nothing visible |
    | 6 | bg1_k0 | the sea |
    | 7–10 | bg2, bg3 | nothing at 10:00 |
    | 11 | fog | everything lighter |
  - **Floats 12–15:** no clear change. Possibly unused.
- **Red is a bad probe for Link,** because his colour is the texture times the light. Black works on any texture:
  - **k0 black (ambient only):** the lit parts go black: the back of his shirt, the elbow and shoulder, the bright side of his knee. The shaded side keeps its colour: the back of his head, his bottom, the soles, the boot.
  - **c0 black (sunlight only):** the shaded side only darkens somewhat, lime to darker green on his hair. So HD's shaded side gets the ambient plus a good share of soft sunlight. It's a gradient, not the GameCube's hard two-tone.
- **The sheen on Link's hair:** HD's lavender actor ambient at 10:00 (entry 1, 186, 180, 205 after gamma) over the yellow hair, plus that soft sunlight. It reads as a cool lime. It's not a sky reflection.
- **BlueWake at 10:00, the same view:** the back of Link's head is dark olive with a dithered edge from the sun shadow map. With sun shadows off it's flat GameCube yellow. HD: bright, with a soft lime gradient and a paler outline.
- **Two things to tune:**
  1. A softer light-to-shade transition on characters, with sunlight wrapping past the edge.
  2. The shadow map not double-darkening a character's own shaded side.
- **Actor colours, HD (after gamma) against GameCube:**

  | Pale | Hour (guess) | GameCube shade (c0) | HD shade (c0) | GameCube light (k0) | HD light (k0) | HD floats |
  |---|---|---|---|---|---|---|
  | 1 | morning | 180, 174, 199 | 186, 180, 205 | 247, 238, 229 | 247, 238, 223 | 2.0, 2.0 |
  | 2 | noon | 156, 140, 134 | 185, 171, 146 | 255, 255, 255 | 255, 248, 210 | 2.75, 3.0 |
  | 3 | evening | 172, 133, 97 | 145, 132, 130 | 215, 188, 105 | 241, 200, 136 | 2.0, 2.0 |
  | 4 | dusk | 116, 108, 124 | 130, 106, 102 | 131, 129, 145 | 130, 149, 140 | 1.0 |
  | 5 | night | 81, 100, 148 | 96, 150, 151 | 119, 148, 176 | 119, 167, 191 | 1.0 |

## The cliff left of the bridge: a grazing wall

- **Where:** Outset's eastern cliff, left of the bridge, viewed facing south at 10:00. The sun angles match between the games (confirmed earlier), and nothing casts on it. Link's shadow points to about 4–5 o'clock, so the sun nearly grazes this wall.
- **HD:** the whole wall is in shade. With each colour set red, the main face gained +57 from the shadow colour, +26 from bg0's shade and +1 from bg0's light; the lower block gained +75, +24 and 0. So no sunlight reaches it.
- **BlueWake:** a pale, sunlit patch on the lower block, with a hard diagonal edge.
  - The facing view (debug 7) shows the face turned toward the sun.
  - The cascade view (debug 6) shows the cliff in the second cascade, and **the lit patch exactly where the first cascade reaches. The "shadow edge" is the cascade boundary.** The two cascades disagree on a grazing wall: the second shades it, the first lights it.
- **HD's hour sweep** (07:00 to 10:00): the main face is a little lighter at 08:00 and 09:00 than at 10:00. Link's head hides the lower block, so this is inconclusive.
- **Measured with the checkers (both games, 10:00):** Outset's rock textures were swapped for dev checkers in both games. The light squares give the light each face receives:

  | | Main face | Lower block |
  |---|---|---|
  | HD | 134, 163, 185 (bluish, about 60%) | 172, 186, 190 (bluish, about 72%) |
  | BlueWake | 87, 114, 119 (bluish, about 43%) | 201, 200, 173 (warm sunlight, about 78%) |

  - **HD's shade on this wall is much lighter than BlueWake's.** The dark slate seen before was mostly the rock texture itself.
  - **HD gives the lower block no warm sunlight,** where BlueWake's first cascade lights it fully.
  - Two fixes follow: a lighter, bluer shade (with HD's hourly shadow colour), and a first cascade that doesn't light a grazing wall.
- **The same view through the morning** (light squares, R, G, B; HD's lower block may include Link's head at some hours; BlueWake warped to `sea:44:128` at each hour):

  | Hour | HD main face | HD lower block | BlueWake main face | BlueWake lower block |
  |---|---|---|---|---|
  | 07:30 | 152, 151, 162 | 175, 170, 172 | 110, 120, 129 | 178, 170, 170 |
  | 08:00 | 181, 182, 192 | 210, 207, 206 | 126, 139, 143 | 196, 190, 187 |
  | 09:00 | 161, 174, 191 | 178, 184, 188 | 91, 115, 126 | 197, 193, 181 |
  | 10:00 | 141, 168, 188 | 186, 198, 196 | 87, 114, 119 | 201, 199, 173 |

  - **HD's light follows the sun gradually:** brightest at 08:00 (the sun most square on to the east face), then dimmer and bluer as it swings south and grazes it. There's no hard shadow at any hour.
  - **BlueWake switches by cascade:** the main face is in the second cascade's shadow at every hour (about 35–55%), and the lower block in the first cascade's sunlight at every hour (about 70–78%, warmest at 10:00, when HD's turns blue).
  - **HD's main face is 1.4–1.8 times brighter than BlueWake's** at every hour.
  - **Correction, with HD's shadows dyed red** (the shadow colour set red in every light palette entry): **HD has the whole wall in shadow; the brightness changes above are its shade light changing with the hour**, not sunlight. The red gain from the wall's top edge down:

    | Hour | Top row | Lower down |
    |---|---|---|
    | 08:00 | about 41 | 41–52 |
    | 09:00 | 46 | about 100 |
    | 10:00 | 54 | about 125 |
    | 11:00 | 31 | about 120 |

    - At 08:00 the wall is fully in shadow. From 09:00 only the top few metres catch some sun over the cliff's edge (half the shadow colour), as Dale read it. The lower block is in full shadow at every hour.
    - **For BlueWake:** the first cascade's sunlit lower block is wrong at every hour. The second cascade's shadow on the main face is right in place, but too dark (HD's shade is about 1.5 times brighter) and needs the hour's shadow colour.
  - **What the fix needs (before the dye test):** a smooth shade by how directly a surface faces the sun, from the rebuilt normal; a lighter shade floor tinted by HD's hourly shadow colour (a grazing wall near 55–65%, not 35–45%); and the shadow map kept for real casters, with its bias not flipping a grazing wall at a cascade boundary.
  - **The setup:**
    - **BlueWake:** pack copy `C:\src\test-builds\hd-fog\pack-rockgrid`, `run-load.ps1` variant `rockgrid`.
    - **Cemu:** graphic pack `portable\graphicPacks\BlueWake_RockGrid`, built by `C:\src\tools\cemu\tour-win\hd_texswap.py`. It encodes BC1/BC4 at every mip level, swizzles with the texture's GX2 tiling, writes in place, decodes back to check, recompresses Yaz0 and rebuilds the pack, so the game files stay untouched.
    - **The checkers:** `Txa_iwakabe_d` (most rock faces) is the 1 m checker in black and white, tiled 4x4. `Txa_iwa2_c` (the left cliff's main face and lower block) is the 2 m green checker, tiled 2 across.
- **Proposed fix:** in the sun shadow pass, darken surfaces by how directly they face the sun. Use the normal the pass already rebuilds from depth, with a smooth fade to the shade as a surface turns edge-on, the way HD's shade fades on Link. A grazing wall then shades in every cascade, and the false edge at the cascade boundary goes away.

## HD's own code: the WWHD decompilation

[ZeldaWWHDDecomp/wwhd](https://github.com/ZeldaWWHDDecomp/wwhd) (Dale's find): 28,244 of HD's 39,713 functions decompiled and verified against the game. Its `docs/hd-differences.md` lists HD changes unit by unit. What it answers here:

- **Light palette (d_kankyo):** HD's Pale entries are 0x6C bytes with float light parameters, matching what we measured. Every frame, light colours and the fog colour and distances go into HD shader parameter objects.
  - Actor and BG0 K0 fade toward C0 through day/night transitions.
  - The fog end distance is pushed out by an HD brightness value.
  - The fog takes an alpha from the palette.
  - A per-room light-direction table (in the Stage archive) can replace the base light.
  - The sun gets an HD height factor.
  - HD has day and night light presets, blended over 6–7 and 18–19 on "daylight" stages.
- **Sky clouds (d_kankyo_rain):** always one pass with three texture layers.
- **Shadows (`wwhd_src/hd/hd_layer_shadow.cpp`):** three cascades, 1024x1024 each, set up per frame by `calculateShadowRanges`. Their ranges are dynamic, as Dale suspected:
  - **Near:** 30.
  - **Middle:** about 2.5 x the camera's distance to its target, adjusted by the camera's height (offset 70, scale 2), clamped to 300–1000.
  - **Far:** middle plus the middle–near span scaled by 2500/1000, clamped to 500–2500.
  - A line check from the camera to the scenery shortens the ranges when the view hits terrain.
  - Special camera modes, events and three cutscenes (Demo02, Demo06, Demo25) use fixed ranges.
  - Defaults: depth bias -0.2, blend 0.45, bias parameters 1.25, 1.75, 0.001, 0.0001, cascade scales 0.5, 2, 4, 2.
- **Tuning table** (`sLayerShadowTuning`, 0x101F8994, read from Cemu's memory):
  - middle limit 1000, middle minimum 300, middle maximum 900
  - normal height scale 2, normal distance scale 2.5, normal clamp minimum -100
  - far limit 2500, far extension 500, near extension 200
  - default near 30, height offset 70
  - high mode: middle 200–1700 (base 1000), clamp ±1800, height scale 80, near 30
  - flags 1, 1, 1, 0, 1; default range 30, 1500, 4000
- **Compared with BlueWake:** BlueWake's cascades are fixed: 1250 from the centre, each next one 4x wider. HD's first cascade reaches only about 300–1000 units, and fits the camera. That likely explains why BlueWake's first cascade lights Outset's wall while HD has it in the second, softer level.
- **The full file, and HD's shaders, read (Oct 30 evening).** The decomp is cloned read-only at `C:\src\wwhd-decomp` (b2811a3). Cemu's own shader dump was used, switched on through its menu by `tour-win\cemu_menu.py`. The shader cache was set aside as `*.keep-20261030`, so every shader compiled fresh; the dumps are in `portable\dump\shaders`, local only. HD's live shadow block was read with `tour-win\cemu_shadowenv.py`, and tuning values were changed with `tour-win\cemu_poke.py`.
  - **HD shadows a full-screen pre-pass, not the materials** (dumped `62ae9bc6efcb56c6` ps):
    - It gathers 4 depths and rebuilds the view position and a normal from depth.
    - It picks the cascade by view depth against the boundaries (one cascade per pixel; no blend between cascades).
    - It moves the position along the normal by `20 x pow(1 - |N.L|, k)` units before the lookup. That's a normal offset that grows as the surface turns edge-on to the sun, up to 20 units (BlueWake's bias is fixed slope/normal values).
    - It takes 2 hardware-PCF taps, 80/20, jittered per pixel by a sin-hash (radius 0.01 x a uniform), and adds a far fade.
    - **Output:** G = shadow (1 = lit) and B = `scale x pow(1 - |N.L|, k)`, a grazing term written separately. Material shaders then read this buffer with a depth-aware upsample (gather of 4 depths). So HD has a smooth "turned away from the sun" term apart from the shadow map, which is what the proposed fix for the grazing wall wanted.
  - **Beyond the second cascade only actors cast shadows (HD's own code, not Cemu).** `drawShadowCascade` draws the background lists (terrain, buildings) into cascades 0 and 1 only. The third cascade gets just the model list, unless a special camera sets the extra-cascades flag (decomp line 775).
    - **At the bridge spot at 10:00** the live block held cascade boundaries 420 | 1395 | 14000, cascade-blend fades 420, 1/200, 1005, 1/390, and screen 1920x1080.
    - **So terrain stops casting shadows about 14 m from the camera.** With the far limit raised to 25,000 (`cemu_poke.py set 101F89A0 25000`), the second cascade reached 10,169, and new terrain shadows appeared on the sea right of the bridge and on the far beach. Beyond that edge, HD's terrain shade is lighting, not the shadow map. That includes Outset's grazing wall, as far as the cascade edge goes. BlueWake's terrain shadows reach kilometres.
    - Shots: `tour-win\shots\sh-base*.png`, `sh-far25k*.png`. The camera drifted between them, so only the sea and beach changes count.
    - **Correction:** an earlier draft read the 420/1005 fades as the shadow cutoff. They're the cascade blends. The real far fade is the pre-pass's 12,000–14,000 (120–140 m).
  - **`shadow_pre_pass.baglshpp`** (AAMP in `Common/Misc/Misc.szs` > Misc.bfres; names recovered by CRC32 against cking.rpx's strings):
    - enable 1, linear sampler 1
    - resolutionMode 1; screen-space blur type 1, width 3, repeats 1
    - noiseWidth 0 (but uniform 19.x drives the shader's jitter)
    - edgeCutoff off (0.5 / 0.1); static depth shadow off; decal AO off
    - **far fade on: 12000–14000**
    - **depth-to-normal on with blur; normal2ShadowRatio 9, normal2ShadowMul 10** (likely the grazing term's exponent and scale)
    - mip-level blur on (reduce, width 3, repeats 1); pre-combined SSAO on; SSAO direct draw on
  - **SSAO, `wwHD_basic.baglssao`:** on, radius 0.03, ao_far 10000, dist_attn 320 (on), reprojection off, mix_rate 0.7, density 1.1, 4 sample pairs, depth offset 0.001.
  - **Shadow strength follows the sun's height:**
    - intensity = 1 - SunHeightFactor x skyBrightness
    - SunHeightFactor = max(0.25, -0.9 cos(time - 15 deg))
    - skyBrightness is 1 in clear weather, the storm value with lightning otherwise
    - **Values:** 10:00 → 0.364 (matches the live value), 13:00 (highest sun) → 0.1, dawn/dusk floor → 0.75.
    - **Tested: no visible effect at 10:00 on Outset.** Forced to 0 and to 1 through the two constants (`cemu_poke.py set 1004F134 0 1` / `0 0`), Link's cast shadow measured (57, 86, 83) in all three shots. The frame differences were only motion.
    - **Cemu or HD?** Cemu runs HD's own compiled shaders, so a value no shader reads changes nothing either way. The dumped pre-pass doesn't read it. The material shaders that read the environment block aren't decoded yet, and a Cemu fault in that buffer's upload can't be ruled out from here.
    - **Other shader values:** fade = 0.5 x (1 - DayNightFade). occlusion = 1 - (1 - RoomLightBlend) x skyBrightness (0 in clear weather).
  - **HD's lighting constants**, read from cking.rpx with `tour-win\rpx_read.py`: sun at time - 15 deg (highest at 13:00), radius 80000, sun height scale 0.9, minimum 0.25, day 97.5-292.5 deg (06:30-19:30), day/night fade step 0.05.
  - **Third cascade:** only the model list is drawn into it (`drawShadowCascade`: index ≥ 2 skips the background lists unless a special camera sets the extra-cascades flag). Depth bias at draw: constant 3, slope 80.
  - **How the materials use it (decoded and probed, Oct 30 evening):**
    - **Method:** a Cemu graphic pack, `portable\graphicPacks\BlueWake_ShaderProbe`, replaces dumped material shaders by hash with edited copies that draw intermediate values. It's local only and enabled through `settings.xml` (backup `settings.xml.before-probe`).
    - **The buffer:** materials read the pre-pass result as a screen-space texture (unit 3) twice, once at mip 0 and once at a blurred mip ("mip-level blur").
    - **Actors** (Link: `4122805e…` and 27 like it, probe v1, kept in `Cemu_2.6\probe-v1`):
      - Channel x is the shadow: white where lit, black where his hair and head shadow his back, with a soft blurred edge. Shot: `tour-win\shots\pr-outset*.png`.
      - It **multiplies the toon-ramp lookup coordinate**: the vertex N·L (`R8.x`) x shadow indexes a ramp texture.
      - The colour is `mix(shade colour from the vertex shader, lit colour uniform, ramp x cloud shadow)`.
    - **Terrain** (`2491cf3a…_7fd249` family, 11 shaders: cliffs, bridge, house, beach):
      - **shadow:** a mix of sharp and blurred channel x. The blur weight is channel y + distance + a near-camera term (within 60–70 units).
      - **shadow term:** clamp(`uf2.x * pow(shadow * uf2.y, e)` + floor `uf3.x`), where e depends on distance (`uf2.z/w`).
      - **light:** `vertex term R2.w x shadow term x cloud shadow` (the cloud shadow is a projected texture plus `uf3.y`).
      - **colour:** `mix(vertex ambient x AO, lit colour uf5, light)`, x vertex colour, + `uf4.x x AO x environment texture`, x albedo, then fog to `uf6` by a squared distance factor.
      - **AO:** channel w, blurred x sharp.
    - **Probe v2** (R = shadow term, G = `R2.w`, B = AO; shot `tour-win\shots\pr2-outset.png`, channels in `pr2-channels.png`):
      - **The shadow term is black on the big right cliff's north faces, turned away from the 10:00 sun**, and white on sunlit faces, the bridge and the beach. HD's shadow channel already carries "facing away from the sun", from the pre-pass's depth-rebuilt normals (`is_useDepth2Normal`, `normal2ShadowRatio 9`, `normal2ShadowMul 10`), on top of cast shadows. That's HD's grazing-wall behaviour: a smooth face-the-sun shade merged into the shadow channel before the lighting, so cascade edges and slope bias don't decide it.
      - `R2.w` is low on all cliffs and high on the bridge and beach. **Answered (vertex shader `092a92f4…`): it is plain per-vertex N·L, `clamp(dot(N, normalize(light - vertex)))`, toward the scenery's base light, which on Outset is the sun's position (below).** It looked like a weight because at 10:00 the cliffs are nearly edge-on to the sun (the east wall 0.01–0.1 linear) and flat ground isn't (the bridge 0.67 ≈ sin 40°). The probe's values are sRGB-encoded: 0.35 shown is 0.1 linear.
      - AO is nearly white, darker only in corners.
      - All values are read after HD's post-processing (bloom, grading), so they're approximate.
  - **For BlueWake:**
    - Build the sun-facing term from the normal the shadow pass already rebuilds. Scale it with `normal2Shadow` 9/10, and merge it into the shadow (min or multiply) before the lighting.
    - Apply shadow as a cut to the light amount between ambient (x AO) and the lit colour, with a floor, rather than as a darkening of the final colour.
    - Turn terrain shadows off past the second cascade, as HD does, or keep them as a BlueWake choice.
  - **The terrain uniforms, read (probe v3; shaders kept in `Cemu_2.6\probe-v3`):**
    - **Method:** terrain pixels drew horizontal grey bands. Five bands held known greys, and they came out as plain sRGB (0.25 → 137, 0.5 → 187, 0.75 → 224), so with bloom blanked (`960d3ef6…` returns 0) nothing else grades the output. Each value was drawn twice, coarse `v/16 + ½` and fine `fract(2v)`. Every band was identical across its rows. Shot: `tour-win\shots\pr3-outset.png`.
    - **Values at 10:00 on Outset:**
      - `uf2` = 0.5, 2, 4, 2. The shadow term is `0.5 x pow(2 x shadow, e)`, with e from 4 (near) to 2 (far, by `uf0`'s distance fade). That's a steep curve: full light at shadow ≥ 0.5, almost none below 0.25.
      - `uf3.x` = 0: no shadow floor.
      - `uf3.y` = 0.365: the cloud-shadow minimum, cloud = clamp(0.365 + cloud texture). It equals the shadow block's `intensity` (0.364), so **`intensity` = 1 − 0.9 x sun height is how light cloud shadows stay**: darkest at 13:00 (0.1), weak at a low sun (0.75). That explains the strength test above (no cloud shadow in view). It's HD's behaviour, not a Cemu bug.
      - `uf4.x` = 1.25: the environment term's weight.
      - `uf5` = 3.0, 2.82, 1.97: the lit colour, = Pale entry 2's bg0_k0 (255, 240, 167)/255 x its float 4 (3.0), exactly.
      - `uf6` = 0.066, 0.396, 0.734: the fog colour, = entry 2's fog (17, 101, 187)/255, exactly.
      - `uf0.x/z`, `uf1.x/y` are above 8 (out of range): distance-fade starts and buffer sizes. `uf0.y/w` = 0.0049, 0.0024; `uf1.z/w` = 0.0005, 0.0011 (texel sizes).
    - **So:** at 10:00 HD uses palette entry 2 outright, not a blend with entry 1. It treats the palette bytes as linear values times the brightness floats, so the lit colour is above 1.
  - **The shade side (probe v4: the per-vertex ambient `R2.xyz` and vertex colour `R0.xyz`, ÷4, in alternate bands; `pr4-outset.png`, `pr4-amb-vc.png`):**
    - **Flat ground** (bridge, beach): the ambient is **the hourly shadow colour itself**, (28, 97, 181) against entry 2's (27, 98, 181).
    - **Vertical cliffs:** (61–66, 102, 138–147), about 38% of the way from the shadow colour toward bg0_c0 (126, 106, 75). **Confirmed in the vertex shader: a hemisphere ambient, `mix(uf1, uf0, 0.5 + 0.5 x N.y)`, uf0 the shadow colour (from above), uf1 the colour from below (bg0_c0 x its factor).** A vertical wall takes half of each.
    - **Vertex colour:** the GameCube's baked greys, 0.5–0.77 on the cliffs and 1.0 on the bridge and beach. The house reads differently (another vertex layout).
  - **HD's terrain light, all together:** `albedo x vertex colour x mix(ambient x AO, bg0_k0 x float4, R2.w x shaped shadow x cloud) + 1.25 x AO x environment`, then fog to the palette's fog colour.
    - The ambient is the shadow colour on flat ground, blended toward bg0_c0 on walls.
    - The shaped shadow carries both cast shadow and facing away from the sun.
  - **Where the scenery's light comes from (decomp `SetBaseLight`, both games):** a room's own light (the GameCube's LGTV entry; HD replaced it with a per-room elevation/heading table at `g_env_light` + 0x1108 / + 0x1188, 64 rooms) wins; otherwise the sun's position (`mSunPos2`). Outset has neither (no LGTV in GameCube `sea/Room44` or the sea's stage; HD's table empty, read live), so both games light it from the sun, placed by the same formula (`time - 15 deg`, 80,000 units from the camera's eye). HD read live: base light = mSunPos = mSunPos2 = (-138355, 56806, 348000), about 40 degrees up from Link. **The difference is how the light falls off: HD's scenery takes N·L per vertex; the GameCube's doesn't** (with sun shadows off, BlueWake's north and east faces come out about equally bright), so in BlueWake only the shadow map shaded faces turned from the sun. `tour-win\cemu_roomlight.py` reads the room angles.
  - **Not yet known:**
    - exactly how the pre-pass folds the facing term into channel x. The dumped pre-pass's output shows it as B; a later combine/mip pass (pre-combined SSAO) likely merges it.
    - the jitter radius (19.x)
    - ~~whether `intensity` (0.36) is read anywhere~~: answered, it is the cloud shadow's floor (`uf3.y`).

## Making BlueWake's shade land where HD's does (Oct 30, late)

Built on #494 (BlueWake 2b63027, RecompCore pin 259627f). RecompCore branch `codex/hd-sun-facing` and BlueWake's changes are local, not committed or pushed. Builds were all host-only (the game module never recompiled).

- **The test method:**
  - **Matched view:** after a load both games show a load-in camera; **pressing and releasing L** (Dale) gives the follow camera, and then the two views nearly match. 21:9 starts BlueWake's follow camera at distance 1.12 (`BLUEWAKE_CAMERA_DISTANCE`), but that only acts on the follow camera.
  - **One save state for every run:** `C:\src\test-builds\hd-fog\states\bridge-follow.bwstate` (quest log 3 of `cirrus-ql3.card`, L at retrace 1050, saved at 1200; frames at 1255). Pad scripts don't run after a state load, and a state restores the retrace counter.
  - **Exact shade masks:** a debug-9 frame divided by the same frame with sun shadows off. Single sample points mislead (the two games' shadows fall a little apart).
  - **Each HD look part off in turn** (`run-load.ps1` variants `xao`, `xcsh`, ...), from the state.
- **Grazing and turned-away faces (the east wall): fixed by the facing term.** Since the GameCube's scenery light doesn't fall off with angle (above), the sun pass now also shades what faces away from the sun, from the depth-rebuilt normal: `lit = min(lit, clamp(N.L x mul))`, up to HD's far fade (12,000–14,000), not on the sea or characters. **x1.5 matched HD's east wall** (dark, a little light near the top); x1 also darkened the flat bridge; x3 left the wall's edge lit. Off by default (`DOL_AURORA_SUN_SHADOW_FACING=1`, `_MUL`), multiplier still 10 by default. HD's terrain curve (`0.5 x pow(2 x shadow, e)`) made little difference and is left off in tests (`DOL_AURORA_SUN_SHADOW_SHAPE=0`).
- **The sea mask's green channel marks characters:** a draw is a character if it is toon-shaded (a texgen from a lit colour, GX_TG_SRTG, Wind Waker's ramp) or skinned. Skinned alone missed Link's hair, which is rigid on his head; sun casters alone missed most of Link.
- **The mask is drawn whenever the facing term needs it** (`sea_mask_wanted()`). Before, it was drawn only while the scene was brightened (gain over 1), and in the state runs it wasn't (see the save state bug), so the facing term silently stayed off.
- **Link shading himself: two causes, both fixed.**
  - **The gap fill** (steps toward the sun through the depth buffer) found Link's own front and shaded his hair, back and legs. Dale: "Gap fill was a bandaid." **Off by default now** (`DOL_AURORA_SUN_SHADOW_MAP_GAP_FILL=1` for comparison); his shadow on the bridge keeps its feet without it.
  - **The near slope term (10)** let his side turned from the sun (his whole back at 10:00) shade itself from his front. **Near slope is 20 now, as far off** (Dale: the low value kept shadows at their feet before the sea's own rule did; the facing term covers acne). At 20 only Link and a few palm leaves changed; the shadow under him didn't (21.1% against 21.3% of that patch). His hair then measured exactly as with sun shadows off, as HD's.
- **HD's palette in the shade (the recipe):** a shaded pixel is multiplied by HD's ambient over HD's sunlight, in the scene's gamma: `pow(mix(1, mix(c0 x f3, shadow colour, 0.5 + 0.5 N.y) / (k0 x f4), 1 - lit), 1/2.2)`.
  - **At noon that is (0.22, 0.40, 0.63), the fixed colour the pass had (0.22, 0.41, 0.60, tuned by eye on Windfall)**, so the recipe reproduces it from HD's data; at other hours the shade's tint and depth now follow the palette.
  - **Data:** `stages.bin` version 3 keeps each Pale entry's shadow colour (linear, as HD keeps it) before the six factors; the WWHD pack's copy was rebuilt (old one `stages.bin.v2-20261030`), checked against Cemu (morning 73, 95, 134; noon 27, 98, 181; night 75, 75, 75). The host blends shadow colour, bg0_c0 x f3 and bg0_k0 x f4 as it blends the brightness and calls `aurora_set_sun_palette`; version 2 or no palette keeps the fixed colour.
  - **Measured over Link's shadow on the bridge:** BlueWake multiplies by (0.22, 0.40, 0.62); HD's shadow over its lit bridge is (0.23, 0.43, 0.63).
- **A save state made with HD's palettes in never got them back (fixed).** The host recognised a stage, and took its palette table, only when the table held the GameCube's colours; a state's held HD's, so state-loaded runs had no HD brightness (exposure 0) and no palette. Every state run before this fix was without HD's brightness.
- **HD sunlight's power: 1/2.2, not 0.37.** HD's sunlight in the game's gamma is factor^(1/2.2): 1.65 at noon. 1.65 is where BlueWake started (Dale); it "looked too strong on the islands' sand and rock" (the old comment) and was taken back to 0.37 while the cloud shadows darkened the whole picture and turned-away rock wasn't shaded. It's the default again; the menu's "As in HD" is 1/2.2, "Softer" the old 0.37, "Much softer" 0.3.
- **The cloud shadows were the thing shooting us in the foot.** With each HD look part off in turn from the state, only two moved the picture: HD's sky palettes (-11.6 mean) and **cloud shadows (61% of pixels, +10.9 mean when off)**. AO, bloom, edge light and clouds changed under 3%; far blur, torch light and distant islands nothing. With cloud shadows off and the power 1/2.2:

  | | BlueWake before | BlueWake after | HD |
  |---|---|---|---|
  | Bridge, lit | 189, 160, 101 | 222, 188, 119 | 212, 182, 121 |
  | Bridge, shade | 42, 66, 65 | 50, 78, 77 | 53, 84, 84 |
  | Sea | 1, 136, 222 | 1, 156, 254 | 0, 157, 255 |

  HD shows no cloud shadow at that spot and moment (its cloud term is `clamp(intensity + cloud texture)`, nothing where the sky is clear). **Cloud shadows are off by default now**, as `docs/WWHD_LOOK.md` already said they were; the code had them on with the look. Dale's own `settings.ini` still has them on, and 0.37.
- **Ideas that were wrong (kept so they aren't tried again):**
  - **The facing multiplier 10 from HD's pre-pass (`normal2ShadowMul`)**: counted the east wall as fully lit from N.L 0.1 up. HD's light amount is plain per-vertex N.L; 1.5 matched.
  - **A fixed overhead light for Outset's scenery**: Outset has no room light; it's the sun in both games.
  - **HD's room light angles or the GameCube's LGTV as the east wall's cause**: neither exists for Outset.
  - **Culling one side of the characters in the sun map** (`DOL_AURORA_SUN_SHADOW_CHARACTER_CULL`, local test commit): no change to Link at all; to be dropped.
  - **Skipping the gap fill only on characters**: replaced by dropping the gap fill.
  - **"The brightening lifts the shade too, so move it into the sun pass"**: a misreading; the shade's ratio form already divides the sunlight back out, so it's the same thing.
  - **`BLUEWAKE_CAMERA_DISTANCE` to match HD's framing**: it acts only on the follow camera; the fix was pressing L.
  - **The first facing-term sheets (x1 / x1.5 / x3, and "it marks the north faces")**: the term never ran in them (no mask without the brightening); the differences read into them weren't there. Debug view 9 now tints the frame pink when the facing term is off or has no mask.
  - **Reading probe values as linear**: the probe draws into an sRGB target; values shown are sRGB-encoded.
  - **A depth-normal facing term in HD's shadow (x10, "normal2ShadowMul")**, tried Oct 31 as `DOL_AURORA_SUN_SHADOW_FACING_DEPTH_MUL`: Nintendo's source has that line commented out; Normal2ShadowRatio 9 and Mul 10 make a grazing weight instead. Blurring it (`_DEPTH_BLUR`) changed almost nothing, since it was all-or-nothing per face.
- **The afternoon, against HD (Oct 31, early):** Cemu at 12:00, 14:00, 16:00 and 18:00 (clock set with `cemu_mem.py`, L for the follow camera; `tour-win\shots\hd-bridge-HHMM.png`) beside BlueWake at 2520x1080 with the facing term x1.5 and off (`run-facing-check.ps1`, sheets `hd-fog\hd-vs-bw-HHMM.png`).
  - **The facing term is right at every hour.** With it off, BlueWake lights large grey patches of rock HD has in shade (the noon cliffs most of all); with it on, the shade falls where HD's does, and the west-facing low wall right of the bridge lights at 16:00 and 18:00 as in HD.
  - **What's left are hard triangles** (Dale: one at 14:00, a few at 16:00, one on the left cliff higher up at 18:00): whole triangles lit or shaded with straight edges. The term's normal is rebuilt from depth (`depth_normal`), one flat normal per triangle; HD's `R2.w` is N.L per vertex, interpolated, so its light fades across a facet. HD's own depth normal is blurred and only folded in at x10.
  - The sea's brown at 18:00 is HD's too.
  - **Outset's scenery has no vertex normals in BlueWake (the GameCube's data).** A local debug log (`DOL_AURORA_TERRAIN_NORMAL_LOG=1`, RecompCore 82d902a, run variant `normals`) printed each scenery draw's format at 16:00 on the bridge. The island's draws (8 textures at 0x013f0f60–0x0141af60 and others like them) have no normal, take vertex colour, and have lighting off: the GameCube's baked shading. Only some objects and actors carry normals. So HD's smooth `R2.w` comes from HD's own models, and the facing term can't read a vertex normal here. Options: smooth normals made on the CPU per static model (weld by position, average the faces), drawn by the mask pass; or a blurred depth normal.
  - **Smooth normals made on the CPU (Dale: "Let's try it like hd"; RecompCore b01e62f, local):** while the facing term is on, a solid scenery draw without normals gets them in its vertices' normal slot (the copy sent to the GPU): faces summed by area at each position. The sea mask's pass draws them into a second target (RGBA8, view space), and the facing term reads them, with the depth's normal as the fallback. With the facing term off nothing runs and the pass has no second target. Without a crease limit: the tall cliff at 14:00 and 16:00 lost its lit facets (Dale: "1400 is way better on that taller rock wall"), but the low wall facing the camera got a lit wedge at 14:00 (its top's normal shared with the flat top above). **Crease 60°** (`DOL_AURORA_SUN_SHADOW_FACING_CREASE`, run variant `creaseN`): the wedge at 14:00 goes, but a sharper diagonal comes back on that wall at 16:00, likely a strip that turns the crease, whose shared vertex keeps a normal between the two sides. **The diagonal is the facing term's**, not the shadow map's (new debug views: 10 the facing term alone, 11 the map alone; sheet `debug-1600.png`): the wall's corners get different normals, and across its two big triangles the light splits on the diagonal. **Crease 45° clears it at both 14:00 and 16:00** (`crease-wall.png`); 30° breaks the wall into lit and dark panels, 60° and no limit keep the diagonal. 75–90° untried. Cost not measured yet (every scenery draw, every frame; static models could be cached). Sheets: `smooth-crops.png`, `crease-crops.png`.
  - **16:00 still differs (Dale: "4 o'clock still looks weird"): HD's normals, not the sun.** In HD the tall cliff's middle is in shade; BlueWake lights a pale band down it. HD's probe v2 at 16:00 (`tour-win\shots\hd-probe2-1600*.png`, `hd-fog\nl-1600.png`): its shadow term is white there (nothing casts on it), and its `R2.w` is low, so HD shades it by N.L. HD's light position at 16:00, read live: mSunPos2 = mSunPos = (-251509, 56847, 347955), the camera's eye plus the GameCube's own offset for a = 225 degrees (eye 278 up), about 41 degrees up from the west: **the same sun BlueWake uses.** Both BlueWake normals (depth and smooth) light that face, so HD's remade models' normals there turn from the sun more than the GameCube mesh's shape does.
  - **HD's own normals, by the GameCube's positions (Dale: "Let's attempt 2"; RecompCore 09b6baf, local test switch `DOL_AURORA_SUN_SHADOW_FACING_HD_NORMALS`, run variant `hdn`):** HD's room models keep the GameCube's vertex positions, each vertex buffer moved by its bone (Outset's main model: two bones, offsets (406, 1492, -321) and (406, 1492, 19679)). After that, 100% of the main model's 5,946 GameCube positions find HD vertices (98% one normal, the rest 2–3 at hard edges), model1 99.9%; model3 has no normals in HD. A local table from Dale's own disc and dump (`hd-fog\hd-normals-sea44.bin`, 6,703 positions; never shared) feeds the smooth-normal path: a vertex at a listed position takes HD's normal (at a hard edge the one nearest its own faces), else the computed one (crease 45). In play 91% of the scenery's vertices found one. Result (`hdn-*.png`, `hdn-crops-1600.png`): no triangles or diagonals; the tall cliff's middle darkens in soft gradients at 16:00. **With HD's normals, plain N.L (x1.0, HD's own `R2.w`) matches HD better than x1.5 at 14:00 and 16:00, and at 10:00 the bridge and the east wall look as with x1.5** (`hdn-mul-*.png`): the x1.5 had made up for the GameCube mesh's normals. A subagent is checking the positions on every stage (`RESEARCH-20261031-hd-normals-coverage.md`).
  - **The far walls (Dale: "a tiny bit of weirdness left in the far walls with hd n×l"): HD uses both normals.** At 16:00 HD's `R2.w` with its models' normals is light on the notch's inside faces and the far stacks (they face the sun), yet HD has them dark: its shadow term is dark there, in broad soft patches, beyond where its terrain casts (second cascade). That is the shadow pre-pass's own facing from the depth-rebuilt normal (`normal2ShadowMul` 10, blurred). So HD lights by its artists' smooth normals and shades what the real shape turns away by the depth's normal. Added as a local test (RecompCore b797e87, `DOL_AURORA_SUN_SHADOW_FACING_DEPTH_MUL`, run variant `dmulN`): with smooth normals, lit = min(lit, clamp(N.L x 1.0), clamp(N.L of the depth's normal x 10)). At 14:00 and 16:00 the far walls darken toward HD; 10:00 is unchanged (`dmul-*.png`). **Blurred** (Dale: "Yep blur it"; RecompCore b81309c, `DOL_AURORA_SUN_SHADOW_FACING_DEPTH_BLUR`, 8 px at 1080 lines by default, up to 3x wider far off; variant `dblurN`): 3 x 3 points of the shape's facing, leaving out points across a 10% depth jump; the turn from lit to shade on the far walls softens (`dblur-*.png`). GPU cost not measured (9 depth normals per scenery pixel).
  - **Correction, from Nintendo's own shader source on HD's disc** (`content/Cafe/Common/agl_resource_cafe_dev.sarc` > `agl_technique_shdw.sharc` > `shadow_pre_pass.sh`; full write-up `RESEARCH-20261031-hd-shadow-prepass.md`): **HD's shadow term has no facing term.** The line that multiplied the shadow by N.L x Normal2ShadowMul is commented out (`// 13/07/22`); in its place a grazing term g = (1 - |N.L|)^9 from the unblurred depth normal moves the lookup 20 x g units and is written as its own channel, B = 10 x g, which the terrain material uses as a weight toward the blurred shadow and a linear curve (with distance and the near-camera term). So the far walls' darkness in HD is its shadow map (blurred into wide patches), not a depth facing term: **the `DOL_AURORA_SUN_SHADOW_FACING_DEPTH_MUL` test is not HD's rule.** Next: why BlueWake's map leaves the far walls lit at 16:00 (HD's live cascade ranges at that view), and HD's grazing offset and blurred mix.
  - **The far walls were our bias (Oct 31, night).** HD's live shadow block at 16:00 from the bridge: cascade ends 545 | 1833 | none; the material's fades `cShadowParam` = (545, 0.005, 1318, 0.00194): the curve's exponent fades 4 to 2 from 545, the blend to the blurred shadow runs 1318 to 1833 (the second cascade's end), so past it HD uses only the blurred shadow on the linear curve. The decomp (`drawShadowCascade`, hd_layer_shadow.cpp:773) draws the **model list into every cascade** and only the main, extra and opaque lists into the first two (correcting "only actors cast past the second cascade"). BlueWake's map alone at 16:00 leaves the far walls lit; with the far slope term at 0 they shade as in HD (`bias-far-1600.png`): our receiver slope bias (20 texels x tilt, in the far cascades' huge texels) took the cliff's shadow off itself. **HD's bias** (RecompCore 0bc6c1f, local switches `DOL_AURORA_SUN_SHADOW_MAP_HD_BIAS=1`, `_CASTER_BIAS=3,80`; run variants `hdbias`, `cbNsM`): the lookup moved 20 x (1 - |N.L|)^9 along the normal, no receiver slope term, the casters' depth biased when drawn (constant 3, slope 80). For scenery it shades the far walls and stacks as HD does with no new acne; on Link it made him shade himself (hair, legs), so characters (the sea mask's green) keep BlueWake's bias (`hdbias-*.png`, `hdbias2-*.png`).
  - **HD's terrain mix ported** (RecompCore 7651e39, local switch `DOL_AURORA_SUN_SHADOW_HD_MIX=1`, `_HD_MIX_FADES=545,1318,1833`, `_HD_MIX_BLURS=1`; run variant `hdmix`): the sun shader also runs at a quarter of the size writing (map shadow, 10 x (1-|N.L|)^9) into an RGBA16F target, blurred X then Y with HD's kernel (taps 0, +-1.3636, weights 5/17, 6/17); the main pass, on scenery (not the sea or characters), takes w = clamp(blur fade + blurred graze + near-camera 60–70), s = mix(sharp, blurred, w), shade = 0.5 x (2s)^e with e = mix(4 -> 2 from 545 over 200, 1, w); the facing term multiplies (HD's R2.w x shade) instead of the lesser. With HD's bias on top: at 16:00 the tall cliff's right part and the far walls darken and soften toward HD; 10:00 stays as it was (`hdmix-*.png`). HD's fades follow its camera (its middle cascade); ours are fixed at the 16:00 bridge reading.
  - **HD's light rule for the shadows ported** (BlueWake host `hd_sun_rule()` in `hd_look.c` behind `BLUEWAKE_HD_SUN_RULE=1`, RecompCore 336176e `aurora_set_sun_hd_light`; run variant `hdsun`): the host works out HD's `setLightPosHD` from the game's clock (g_env_light.mCurTime, 0x803E56EC): by day the sun with its height held at 40,000 before 09:00 and after 17:00, by night the other side held at 40,000; crossing 06:30/19:30 the light holds while a fade rises 0.01 a game frame, changes sides at the top and the fade falls. RecompCore then uses that direction as is (no hold of its own), the moon's strength at night, and the shadow's strength x (1 - fade), as HD's K0 goes to C0. HD's shadow map follows the same light (`getShadowDirection`: camera to the base light). At 08:00 and 17:30 (`hdsun-*.png`, HD shots `hd-bridge-0800/1730`) the shadows move only a little against BlueWake's own rule; the fade wasn't filmed yet. **HD's palette schedule was already in #494** (`sky_schedule()`, hd_look.c: noon to 17:00, 17–18 and 18–20 blends). New: at 17:30 BlueWake's sky and sea are teal-green where HD's are lavender (colour, for later).
  - **HD's normals for every stage** (Oct 31, night): `make_hd_normals.py` (scratchpad, standard library; to become an importer step) wrote 121 tables, 18 MiB, to the local pack's `optional/hd-normals/<stage>.bin`, keyed by the raw VTX1 position with the normal turned into the joint's frame (details and coverage in `RESEARCH-20261031-hd-normals-coverage.md`: 99.4% of the positions in the models used). The host hands the renderer the stage's table as the stage changes (`hd_normals_stage()` in hd_look.c, `aurora_set_hd_normals`, RecompCore 24e4440). In play: Outset 91% of scenery vertices take HD's normal (as the hand-made table), Windfall 83% (`pack-*.png`). Known: 974 positions shared between rooms with different HD normals (most in Obshop's identical rooms); the runtime's nearest-normal choice covers them unless Obshop looks wrong.
  - **HD holds the sun's height in the morning and evening** (`setLightPosHD`, constants read from cking.rpx): during the day (97.5–292.5 degrees) before 09:00 (135) or after 17:00 (255) the light's height is fixed at cos(240 degrees) x 80000 (40,000 up), so about 27–29 degrees up at 17:00–18:00; at night always (the night test, under 315 or over 75 degrees, is always true) at cos(300 degrees). BlueWake reads the GameCube's `mSunPos`, which isn't held; #494's "held at 24 degrees" for the shadows is close but not HD's rule.
  - **#494 as it stands, from the same spot** (`run-load.ps1` variant `pr494`: today's build with #494's `stages.bin` in a hard-linked pack copy `pack-494`, gap fill on, near slope 10, HD sunlight 0.37, cloud shadows on, facing term off). Sheets `hd-494-today-HHMM.png`: HD, #494, today with and without the crease.
- **Still open:**
  - The low wall facing the camera: the crease limit's angle, or splitting a strip's vertex at a crease.
  - The smooth normals' cost, and caching them per static model.
  - The facing term's default (on with the HD look's sun shadows? x1.5 as its default?).
  - Rebuilding cloud shadows on HD's rule (its `cloud_shadow` shader in `Prim/wii_pipeline.sharcfb`, the cloud texture over `intensity`).
  - Other stages (the bridge is checked at 12:00–18:00, above).
  - Remaining colour differences: HD's Link has a lime gradient in his hair and a bluer shirt (actor ambient and ramp); HD's palms are greener.
  - Two RecompCore commits to drop before any PR (the cull test and the facing debug log), and the work to split into clean commits off #494.
- **New tools** (`C:\src\tools\cemu\tour-win\`): `cemu_menu.py` (Cemu menu items by message), `cemu_poke.py` (guest floats), `cemu_shadowenv.py` (HD's shadow block), `cemu_roomlight.py` (room light angles), `rpx_read.py` (constants from cking.rpx), `dshot.py` (Dolphin F9 shot). Run script options in `C:\src\test-builds\hd-fog\run-load.ps1`: `-SaveState`, `-LoadState`, `-Zoom`, `-Pad`, and variants `facing`, `mulNN`, `noshape`, `nogap`/`gapon`, `cullf`/`cullb`, `nnearN`/`snearN`, `sunNNN`, `xPART`, `flog`, `hdtrace`; `run-hours.ps1` for the bridge through the day.

## Windfall's dock tower (parked)

- **Too light under HD's sky:** at noon the upper wall measured 135 with HD's sky, 62 with the GameCube's, and about 80 in Cemu. That was measured without the two sky switches, so it needs redoing with them on. The textures aren't the cause (HD's Windfall art averages the same as the GameCube's).
- **A lit gap under the balcony:** the slope bias. Far slope must stay 20 because of Ice Ring Isle; `STEPS=1` doesn't help. ~~Idea: let the gap fill's 120-unit limit grow with the reach~~ (the gap fill is off now); look at it again with the facing term on.

## Open questions and next tests

- **The light palette's 16 floats:** which float scales which colour (at noon two are 2.75 and 3.0).
- **bg2:** nothing on Outset at 10:00 or 21:00. Look for it on another stage.
- **Shadow tuning in a test room** (Dale): the test maps have dev textures marked in metres. In one that counts as outdoors (stage type 0 or 7) and that HD still has, shadow offsets from the caster's foot, the width of the soft edge, cascade boundaries and the grazing-angle falloff can be measured in real units, with HD's sun moved through the clock and the light palette.
  - **Stage types, from the GameCube disc's STAG:**
    - **`E3ROOP`** (the sea minigame test): outdoors, and in HD. It's the only candidate for a side-by-side comparison, but our stage select marks it "hangs" in BlueWake.
    - **`VrTest`, `I_TestM`, `I_TestR`, `K_Test2`:** outdoors but GameCube only. Usable in BlueWake alone.
    - **`ITest61–63`, `I_SubAN`, `Kaisen`, `Msmoke`, `TEST`, `H_test`:** not outdoors.
  - **HD has no stages or rooms the GameCube lacks;** it only removed some.
  - **Later (Dale): possibly port a test room to Cemu.** A lighter option is a metre-grid texture swapped into an ordinary HD outdoor stage (a copy of HD's content folder; BlueWake gets the same grid through its texture pack). Objects of known size, like the pier's posts and Link, can also serve as rulers.
  - **The dev metre textures** are on the GameCube disc in `I_TestM` (the fullest set), `I_TestR` and every `K_Test*` room. They're two-by-two checkers labelled 1, 2 and 4 m (`Txi_c1m`, `Txi_c2m`, `Txi_c4m`, CMPR 128x128) and 10, 50 and 100 m (`Txk_c10m`, `Txk_c50m`, `Txk_c100m`, RGB565), plus unlabelled checkers (`Txi_c1`, `Txi_c2`, `Txi_c4`) and numbered checks (`Txi_Check_5` to `_30`). Decoded locally to `C:\src\test-builds\hd-fog\dev-textures` (disc art, never committed).
    - **The metres come from the test rooms' UV mapping, not the texture.** Swapped onto another surface, a checker tiles at that surface's own scale, so it needs calibrating once (1 game unit is about 1 cm).
    - **First choice: the sea** (Dale). It's flat and huge, and its textures (`Txa_umi_nami_01`, `_kiwa_01`, `_kage_01`, I4) are mapped in world space, so one calibration holds everywhere. Once calibrated, it's a ruler readable at a glance:
      - the sun's elevation, from a post's known height and its shadow's length (elevation = arctan(height ÷ length)), and its direction from the shadow's angle to the grid
      - where shadows start and how soft they are, in metres
      - where BlueWake's cascade boundaries fall
      - where the haze begins over the water

      Catches: the sea scrolls and swells (take shots close together, or freeze it); its greyscale layers tint the checker sea-blue; BlueWake's shadow pass treats the sea without slope or normal bias, so terrain biases still need a solid surface; and HD's water may blend those textures differently.
    - **Swapping one into HD** means writing it in HD's texture format and GX2 tiling, at the replaced texture's size, into a copy of HD's content folder. BlueWake gets the same checker through its texture pack.
  - **HD ships 131 stages, the GameCube 156.** HD kept `ITest61–63`, `I_SubAN`, `E3ROOP`, `Kaisen`, `Msmoke` and `Mtest` (in szs_permanent2.pack), and pruned `H_test`, `I_TestM`, `I_TestR`, `K_Test*`, `TEST` and `VrTest`.
- **The soft-sunlight share on Link's shaded side:** divide the c0-black shot by the normal one on the shaded side.
- **HD's SSAO's part in Link's softness:** Cemu's `WindWakerHD_NoSSAO` pack, after a restart.
- ~~**The BlueWake shadow map's debug overlay on Link** on the bridge (is he shadowing himself?)~~: answered, yes, by the gap fill and the near slope term (both fixed, above).
- **Dawn and dusk comparisons** of Link and the cirrus colour, to fit how HD applies the kumo colour and the 1.1.
- **The cirrus ring's position:** a longer timed run in both games (start angle or speed).
- **HD's puffs are larger** and keep filling the sky. BlueWake's drift away.
- **Whether the two sky switches should come on with HD's sky.**
- **New games starting at 10:00** under the HD look.

## Captures

- **Shots carry where they're from (Dale, Oct 31: "We should tag room and spawn in shots we save in metadata").** `run-load.ps1` writes `run.json` in each run folder (stage, room, point, hour, card or state, window, RecompCore commit, every switch). `C:\src\tools\hd-look-scripts\compare_sheet.py` builds sheets from run folders and PNGs and keeps each tile's facts in the sheet's PNG text (`shot` JSON and `Description`); `shotmeta.py tag PNG key=value ...` tags a single shot (the Cemu ones), `shotmeta.py show PNG` prints them. Shots made before Oct 31 don't have them.

- `C:\src\tools\cemu\tour-win\shots\`:
  - `cir-*`: Cemu at 10:00 after the load.
  - `fx*`, `floats-min-max.png`, `floats-heat.png`, `floats-bridge.png`, `sk-*`: the sky floats.
  - `rd-*`, `rd-redmaps.png`, `gap-edge-closeup.png`: the red dome and the wrap.
  - `tod*`, `tod2-sheet.png`: times of day.
  - `lc-*`, `pa-*`, `pb-*`, `pk-*`: Link with the sky and light colours.
  - `stock-*`, `ultra-*`, `planks-stock-ultra.png`: stock and Ultra shadows.
- `C:\src\test-builds\hd-fog\`:
  - `outset-*`: BlueWake runs.
  - `cirrus-*.png`, `link-hair-*.png`: comparisons.
  - `windfall-*`, `tower-*.png`, `awning-*.png`: the tower.
  - `pack-kumo`: a hard-linked pack copy.
  - `arch-shots`: copies from the Arch disk.
