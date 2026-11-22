# Module art read from the wrong address (#545)

Findings from November 3, 2026, on `main` at ee991d31 (RecompCore 78c266d2), Linux (Arch, Ryzen 9 9900X, Vulkan). This is research only: no code changes. The fixes are #583 and #584; [Outcome](#outcome) at the end says which route was taken and why.

## What players see

- Earth Temple room 5: the hanging tapestries are purple block noise instead of the red and gold skull cloth.
- The Bokoblin cannon boats, the reefs' towers (Cyclops Reef, Two-Eye Reef) and the Forsaken Fortress fly a purple, noisy flag.
- The floating barrels' banners are wrong.
- The pirate ship's sail is flat dark red instead of tan cloth with the skull and swords. Its flag is a black blob instead of the tattered skull flag. Both look plausible, so nobody noticed.
- The Darknut and Phantom Ganon capes decode to noise. A cape wasn't seen up close on screen.

It shows with the HD look on or off, and with or without a texture pack.

## Cause

Some modules hand GX a pointer to art in their own data section, at its linked address:
- textures and palettes, through `GXInitTexObj`, `GXInitTexObjCI` and `GXInitTlutObj`;
- vertex arrays, through `GXSetArray`.

The retail functions keep only the low bits of the address. A texture's address goes into SETIMAGE3 as `ptr >> 5` in 21 bits, and `GXSetArray` masks with `& 0x3FFFFFFF`. So the tapestry's cloth at 0xC1856760 reaches GX as 0x01856760.

BlueWake keeps module data at its linked address (0xC0400000-0xC1F8xxxx), outside MEM1. `host_graphics_guest_resolve_uncached` (`runtime/host/src/main.c`) maps a GX address back to a module only in the 0xC0xxxxxx form (display lists) and the 0x40xxxxxx form (OSCachedToPhysical arrays, #126). An address with its top bits cut is read as plain RAM.

Two cases, depending on where the module is linked:
- **Linked at or above 0xC1800000** (119 of 415 modules): the cut address lands past the retail 24 MiB. There BlueWake keeps only its own data: the callback sentinel, the raw-REL scratch window and module 1. So the art decodes from stale REL bytes.
- **Linked below 0xC1800000:** the cut address is real game memory (heap), so the art decodes from whatever the heap holds there.

Display lists aren't affected: `GXCallDisplayList` keeps the full address, and patch 0156 handles textures a module's own display list names. The CPU reads module data at full addresses, so game logic and state are unaffected; only what GX reads is wrong.

## Affected modules

| Module | Linked | What | GX calls |
|---|---|---|---|
| `d_a_obj_tapestry` | high (0xC185) | Earth Temple tapestries | 1 texture, 2 arrays |
| `d_a_pirate_flag` | high (0xC1A2) | pirate ship flag | 1 array (texture coordinates) |
| `d_a_sail` | high (0xC1A9) | pirate ship sail | 1 array |
| `d_a_majuu_flag` | low (0xC0B9) | reef, Forsaken Fortress and cannon boat flags (`d_a_oship` creates it with parameter 4: texture type 0) | 1 CMPR texture (`l_flag02TEX`), 1 array |
| `d_a_obj_buoyflag` | low (0xC126) | floating barrels' banners | 2 textures, 4 arrays |
| `d_a_mant` | low (0xC0BA) | Darknut and Phantom Ganon capes | textures and palettes (its arrays go through `GFSetArray` and already resolve) |
| `d_a_goal_flag` | low (0xC08C) | goal flag | 1 array (its texture comes from an archive and is fine) |

The King of Red Lions' sail is not affected: it is `daHo_packet_c` in `d_a_grid`, in the DOL.

## Modules linked high: a host-only fix, checked

In `host_graphics_guest_resolve_uncached`, a GX address in [24 MiB, 0x01F80000) is looked up as a module alias at `0xC0000000 | address`. If no alias holds it, the resolver reads RAM as it does now. Module 1, which sits in RAM from 0x81F80000, is left out: its alias holds its relocated pointers, which differ from that RAM. About 10 lines:

```c
u32 linked = (address & 0xC0000000u) == 0x40000000u ? address | 0x80000000u : address;
if (address >= GC_MAIN_RAM_SIZE && address < (BLUEWAKE_DYNAMIC_SCRATCH_LIMIT & 0x3FFFFFFFu))
    linked = address | 0xC0000000u;
```

Checks:
- **Census:** a temporary log of every GX resolve between 24 and 32 MiB over 62 stages (sea, towns, every dungeon with 3-5 places each, caves, ships, Forsaken Fortress, Ganon's Tower, Hyrule). It found 5 distinct addresses, all held by a module alias: the tapestry's texture and 2 arrays, and the pirate flag's and sail's arrays. Nothing else in that range reaches GX. GX never reads module 1's range.
- **Nothing else changes:** save states made on `main`, loaded with the fix and dumped frame by frame (frames 100-130), are byte-identical to `main`. That covers 10 places (Outset, Windfall, Dragon Roost Cavern, Forest Haven, Tower of the Gods, Forsaken Fortress, Hyrule, a cave, the open sea, a shop), plus the King of Red Lions with the sail up. Warp-from-boot captures aren't frame-exact (async readback), so this used states.
- **The fixed things:** the tapestry, pirate sail and pirate flag draw their real art (deck of A_umikz, and from Outset).
- **Save states:** states made on `main` and with the fix load with the tapestry right.
- **Speed:** rendered on the 9900X limited to 4 cores, fix against `main`, 4 rotating reps at the Earth Temple, Forbidden Woods, Outset and Windfall. The difference was -0.1% to -0.4%, inside the spread. GX does about 1,600 uncached resolves a frame (median; Forbidden Woods about 6,500), and the fix adds one compare to each. This isn't a weak-PC measurement.

## Modules linked low: the options

No lookup on the cut address alone can be safe, because it is a valid RAM address. The full pointer has to be kept at the call, or the data has to move. What was looked at:

- **Tag bits in the GX registers: no.** SETIMAGE3 bits 21-23 are decoded by RecompCore as address bits 26-28. LOADTLUT0's spare bits hold junk from uninitialized palette objects (patch 0081), which would be read as tags and break menu palettes.
- **Composite natives (`native_gx` and the like): no.** They are certified exact replays of the translation.
- **Host edge intercepts on the four GX functions: they'd work.** But every call then goes through the host, which moves the certified host-turn counts that `bench_rendered.sh` and `bench_instructions.sh` check.
- **Relinking module data above 0xC1800000: big.** Text would have to move too: at 64 KiB a module, text alone is 26 MiB, more than the 20 MiB free below 0xC1800000. So every REL code address, chunk name and PGO profile changes. Save states store module data at its linked address and their digest hashes unrelocated bytes, so old states would load and then break; it needs a state version bump. Relinking only the four modules' data into the free space above module 1 is exact and small. It still means a DolRecomp change (a RecompCore bump) and a state version bump.
- **DolRecomp option-site hooks: the smallest code change.** The hooks would sit before the roughly 10 GX calls in the four modules. Each rewrites the pointer argument to a GX-only window above module 1. That window is a second entry in the host's GX alias registry over the same bytes (`0xC1F90000 + k*0x10000 + (linked & 0xFFFF)`): no copy, so it can't go stale, the game can't see it, and save states don't change. The catch is that option sites exist today only in the opt-in Better Wind Waker build, and this fix has to be always on. So:
  - the base REL translation would need option sites (`scripts/linux/build.py`, `scripts/windows/build.py`, `scripts/builder/profiles/bluewake.sh`, and the base, betterww and combo builds in `scripts/mods/build_mods.sh`);
  - the same hooks would also go into Better Wind Waker's list, because the variant builder treats chunks that differ from the base as the variant's;
  - the base composite would need the option runtime;
  - it needs a new verified composite digest, which means a composite rebuild on every platform and in CI, retrained PGO on Linux, and a partly stale Mac profile.

  It needs no RecompCore or DolRecomp source change.

- **The same hooks as an opt-in option on the mod build.** Mods are added as variant chunks beside the base composite, and the base game is untouched when a mod is off (`docs/MODS.md`). So a fix option built that way leaves the base translation and `COMPOSITE_DIGEST` unchanged (checked later on Windows and Linux; see [Outcome](#outcome)). It would be off by default, as AGENTS.md asks for rendering changes until each platform is tested. Costs:
  - builds without the mod phase (`--no-mods`) don't get the low-module fix, and keep today's flags;
  - where its chunks overlap Better Wind Waker's, a combined variant is needed, as for Better Wind Waker with widescreen;
  - a bug fix sits among the options.

## Outcome

Chris picked the route on November 6, 2026 (on #549): the high-module fix first as its own pull request, and the low modules as the opt-in mod-build option, as long as it leaves the base composite and its digest unchanged. He didn't want a DolRecomp change and a save-state version bump for this.

- **#583**, the modules linked high: the host-only resolver fix above. It fixes the Earth Temple's tapestries and the pirate ship's sail and flag, and is always on.
- **#584**, the modules linked low: the `moduleart` mod (`mods/moduleart/options.txt`). It hooks the four modules' 26 GX calls and points them at the GX-only window above module 1, so save states don't change. The switch is Config › Fixes, "Art fixes for flags, banners and capes (testing)", off by default. Builds without mods don't get it. It is stacked on #583, and the two together close #545.

Checked for #584 on November 9, 2026:
- **Windows:** the cannon boats' flag and Phantom Ganon's cape draw their art. With the switch on, frames are pixel-identical wherever those modules aren't drawn. The digest check still reports the verified base tree.
- **Linux (Arch, Ryzen 9 9900X, RTX 5070 Ti):** in play, the reefs' and the Forsaken Fortress's flags draw their art. Save states loaded with the switch off and then on show a Darknut's cape, the goal flag and a skull barrel's flag going from noise or plain colour to their real art. The base composite digest is the same as `main`'s.

Not checked: the Mac build.
