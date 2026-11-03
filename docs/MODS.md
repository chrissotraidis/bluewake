# Mods

BlueWake has a **Mods** section in the in-game ⋯ menu on iPhone and iPad, and a **Mods** tab in the F1 menu
on Mac, Linux and Windows. **Wind Waker HD textures** comes first in it on every build. Each mod takes
effect the next time BlueWake starts, and none of them changes your saves.

| Mod | What it does | What you need |
| --- | --- | --- |
| Widescreen 16:9 | The community 16:9 code (Dolphin's GZLE01 Gecko code): a wider field of view with the HUD placed for 16:9, letterboxed on a 4:3 iPad | Nothing; it is built in |
| Widescreen 16:10 | The same code with its aspect-dependent values recomputed for 16:10 (Mac displays, most iPads' shape is closer to it); one widescreen mod at a time | Nothing; it is built in |
| Widescreen 21:9 | The same code recomputed for 21:9 ultrawide desktop displays (Windows, Linux, Mac) | Nothing; it is built in |
| HD Textures | Replaces the game's textures with a Dolphin-format pack, such as [Hypatia's HD pack](https://forums.dolphin-emu.org/Thread-hypatia-s-tloz-the-wind-waker-hd-pack-v2-0001a) or [the textures from your own Wind Waker HD disc](WWHD_TEXTURES.md) | The pack's `tex1_…` images: on iPhone and iPad in `Documents/BlueWake/Load/Textures/GZLE01` (Install Texture Pack… copies them there); on the desktop, any folder picked with Choose texture pack folder… |
| Better Wind Waker | [Better Wind Waker](https://github.com/WideBoner/betterww)'s quality-of-life changes from Wind Waker HD, each its own setting: Swift or Brisk Sail, instant text, faster rolling, grappling, block pushing, climbing and crawling, Tingle Chests without the Tuner, an unrestricted boat, no song replays, turning while swinging, a faster Ballad of Gales, skipping the opening movie, an inverted camera, a revealed sea chart. With them, BlueWake's own sea compass switch: on, the red end points the way you're sailing, on a north-up dial, as in Wind Waker HD | Nothing; it is built in |

On the iPad, `Documents/BlueWake` is **On My iPad › BlueWake › BlueWake** in the Files app.

## Installing

**Widescreen:** on iPhone and iPad, turn on *Widescreen 16:9* or *Widescreen 16:10* (turning one on turns
the other off); on Mac, Windows and Linux, pick 16:10, 16:9 or 21:9 in **F1 › Mods › Widescreen**. Then
restart BlueWake. The 16:10 code is generated from the 16:9 one by `scripts/mods/widescreen_aspect.py`:
each value it changes (the camera aspect, which also widens the game's view culling, the 2D bounds, HUD
and map positions, a few instruction immediates) is interpolated between the game's 4:3 value and the
16:9 value, and the three HUD constants it loads from the game's pool take the nearest pool value. The 21:9
code is generated the same way, with the 16:9 change carried on 2.25 times as far (its pool constants land
within half a pixel); the HUD elements the 16:9 code leaves in place stay where the 4:3 game puts them, so
at 21:9 they sit further in from the edges. On the desktop hosts, `BLUEWAKE_ASPECT=16:10`, `16:9` or
`21:9` (the menu's aspect setting, or `--aspect`) selects the mod and the frame buffer shape together.
At 21:9 the picture is no taller than 4:3's, so the follow camera starts one wheel notch further back
(`BLUEWAKE_CAMERA_DISTANCE`, 1.12 there and 1 otherwise; the wheel still moves it from there). At the default 3× render resolution 16:9
renders 2560×1440 and 16:10 2304×1440 (the 4:3 picture's height, widened), so the game's letterbox bars line up with the scene;
if an older device struggles, 2× renders 1707×960.

**HD textures:** download a Dolphin-format pack for GZLE01. On the iPad, Hypatia's *Android-Lite* build
(3× resolution, about 530 MB of PNG files) fits comfortably; the full-size PC builds need several GB of
memory. Copy the folder of `tex1_…` images (in Hypatia's pack, the `GZL` folder) into
`Load/Textures/GZLE01`, turn on *HD Textures* and restart. Subfolders are searched, `_mipN`
sidecar mipmaps are used, and PNG and DDS files both work. The menu shows how many textures it found. On the desktop, **Choose texture pack folder…** uses a
pack where it is, with nothing copied.

You can also [import compatible textures from your own Wii U Wind Waker HD
disc](WWHD_TEXTURES.md). The local importer creates a Dolphin-format pack;
add it the same way, in **Mods › Wind Waker HD textures**.

**Better Wind Waker:** turn on *Better Wind Waker*, pick its settings in **⋯ › Mods › Better Wind Waker
Settings** and restart. Each of Better Wind Waker's settings is a switch of its own, on or off by Better
Wind Waker's defaults until you change it: instant text (and holding B to advance), Swift Sail (or Brisk
Sail, which brakes harder), faster rolling, grappling, block pushing and climbing, Tingle Chests without
the Tingle Tuner, turning while swinging, no song replays and a faster Ballad of Gales start on;
an unrestricted boat, skipping the opening movie, an inverted camera, revealing the sea chart
and correcting the sea compass start off. The sea compass switch is BlueWake's own, not Better Wind
Waker's. On, the red end points the way you're sailing, on a north-up dial, as in Wind Waker HD. Off,
it points north. The switch nops the negation at `0x801FD37C`. No patched disc is needed. Swift
Sail's new sail texture and icons are Wind Waker HD's art and are not included (an HD texture pack can supply them); its name stays "Sail". On the Mac host,
`BLUEWAKE_MODS=betterww` turns the settings on and `BLUEWAKE_OPTIONS=name,-name,...` changes them
(`none` first turns them all off); `scripts/mac/run_host.sh` takes `BWW=1` and `OPTIONS=`.

## Forest Water Challenge assistance

On Mac, Windows and Linux, **F1 › Gameplay › Forest Water Challenge** has two independent
options, both off by default and separate from Better Wind Waker:

- **Keep watered trees when time runs out:** Forest Water still expires and
  becomes ordinary water, but watered trees retain their progress. Collect more
  Forest Water to finish the remaining trees; progress uses the normal save data.
- **30-minute Forest Water timer:** newly collected Forest Water lasts 30 minutes
  instead of 20. This does not reset or extend an already running timer.

The switches apply immediately, without restarting. They are saved as
`BLUEWAKE_FOREST_WATER_KEEP_TREES=1` and `BLUEWAKE_FOREST_WATER_30_MINUTES=1`.
With both off, the original challenge rules apply. The host adjusts the game's
fresh-water timer reset, expired-water progress clear and watered-tree wilt check;
the original timer and bottle expiration logic still execute.

## Better Wind Waker's settings as game options

Better Wind Waker patches the disc: assembly patches to the executable and modules, added code, changed
values and changed message data. BlueWake does the same things at runtime, from
`mods/betterww/options.txt`:

- **Option sites.** The instructions a setting changes (a `nop`, a branch made unconditional, a
  different constant) are listed with the setting's switch. DolRecomp's `--option-sites` translates each
  site both ways and picks one by `dolrecomp_option_flags[switch]` at run time, so the game is translated
  once for every combination of settings. Only the 16 chunks that hold a site differ from the base
  translation; they are the `betterww` mod's variants (3 more combine with a widescreen mod).
- **Native code.** What Better Wind Waker's added assembly did is C in `runtime/host/src/game_options.c`,
  called from hook sites: turning with the stick while swinging on a rope, negating the camera's
  C-stick axis, turning the wind to blow from behind King of Red Lions (through the game's own
  `dKyw_tact_wind_set`, entered from the hook) and Swift Sail's braking.
- **Values.** The constants a setting changes (rolling speed, block pushing frames, the boat's speed) are
  written at boot while it is on; a module's go to its data at the linked address, where it stays for the
  session.
- **Message data.** For instant text the host patches the loaded messages as Better Wind Waker patches the
  disc's: every message draws its whole box at once and its timed waits wait no time.

Better Wind Waker's other changes are left out: its randomizer fixes, the custom player model and colours,
random enemy colours and the title and memory-card art.

## How code mods work in a static recompilation

BlueWake runs the game from native code translated ahead of time, so a mod that rewrites game code in
memory (a Gecko code, or a patcher's assembly changes) has no effect at runtime: the instructions it
writes are never executed. Code mods are therefore built into the app:

1. The mod is applied to the game's executable on the Mac (`scripts/mods/gecko_apply.py` for a Gecko
   code), or for game options the translator is given the option sites (above).
2. The patched (or option-sited) `main.dol` and RELs are translated by the same DolRecomp as the base
   game and merged by `scripts/generate_composite.py` into a composite source tree of their own.
3. `scripts/mods/build_mod_variants.py` compares that tree with the base tree. Every translated chunk
   that differs is added to the base composite under a new name (`chunks_mod_<name>/`), code ranges
   only the mod has become extra chunks, and the bytes the mod
   changes in the executable's data and in the relocated REL data become writes. A Gecko code's data
   writes are re-applied at every retrace, as Dolphin does. When two mods change the same chunk, a
   translation of the game patched by both supplies the variant used when both are enabled.
4. At boot the host enables the mods named in `BLUEWAKE_MODS` (the menu sets it):
   `bluewake_composite_apply_mods` points the chunk table at the variants before the first dispatch,
   and the writes go into guest RAM. Chunks never call each other directly, so a variant replaces its
   chunk cleanly, and the base game is untouched when a mod is off.

The chunks include `generated.h`, which is unchanged, and only `module_export.c` sees the dispatcher
with the writable chunk table, so adding mods compiles the new variant chunks only (53 today), not the
748 base chunks.

HD textures need no build step. Aurora's Dolphin-compatible texture replacement (the
`tex1_WxH[_m]_<XXH64>[_<palette XXH64>]_<format>` names, with palettes hashed over the entries the
texture uses) is connected to GXRuntime's texture decode: the first time a texture's bytes are seen,
the pack is consulted before decoding, and the replacement is cached under the same key as a decoded
texture would be. Replacements are decoded on a background thread the first time they are used (the original texture
is drawn meanwhile), so a new area shows its HD textures a moment later instead of stalling a frame.

## Measured on the iPad Pro (M2)

These results use the developer's optimized build. They do not establish performance parity for a
fresh player build; see [the current performance comparison](BUILDER.md#optimization-profiles).

- Widescreen and the HD pack together, on the pier after loading slot 1: 30 FPS at 100% speed, no late
  frames over more than a minute, main thread about 82% busy (the same as without mods). 5,739 of
  the pack's textures registered; the first file-select frame with new textures took 83 ms.
- All three on the iPad: loading slot 1 and walking the village at 29.9-30 FPS; the pause-menu save
  writes the card, and the saved card reloads with all three mods and with none (30 FPS both). The
  menu tree, checked with `BLUEWAKE_MENU_DUMP=1`, lists the Mods section and the installed pack.
- Better Wind Waker (the patched disc, before the settings became game options): its added code section
  ran (guest program-counter samples), and the Swift Sail was used at sea on the iPad on 2026-09-28.

## Building

The Builder (`scripts/builder/build.sh`, or `scripts/ios/build_device.sh`) adds the mods itself, as its step 6;
`--no-mods` leaves them out. It needs only python3. The step runs

```sh
scripts/mods/build_mods.sh BUILD_DIR
```

which applies the widescreen codes, translates the widescreen executables, the game with Better Wind
Waker's option sites (its executable and modules) and each widescreen executable with them, with the
build's own DolRecomp, generates a composite tree for each, and adds the variants, the option table and
the options' values to the build's composite source before the compile. Nothing needs copying to the
device.
