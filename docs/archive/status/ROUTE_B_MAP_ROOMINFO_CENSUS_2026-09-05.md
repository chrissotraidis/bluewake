# Route B Map Room-Info Census - 2026-09-05

## Boundary

Measure the real `dMap_c::setImage`/`deleteImage` owner reached by the exact
Room44 phase-4/delete call pair. Identify its compile status, retained link
frontier, exact archive resources, and already-qualified room metadata before
partitioning any map code.

No map implementation is promoted by this checkpoint. Map calls continue to
terminate at the ordered observers qualified by patch 0128.

## Source and Content Findings

The complete 2,100-line `d_map.cpp` compiles unchanged under the strict Route
B target in Debug, optimized Release, and ASan/UBSan configurations. The
full object has 69 unique undefined symbols.

A dead-stripped executable retaining only `dMap_c::setImage(44, 44, 321.5)`
and `dMap_c::deleteImage(44)` still has 47 undefined symbols. Twenty-eight are
GX entry points retained by static draw-object vtables and texture setup. The
rest group into assertion/runtime support, endian conversion, dRes lookup,
room-control stay/data access, the game-info singleton, and C++ runtime
symbols. This proves full-unit linking would admit unrelated map draw state
and is not a bounded solution.

The exact 714,816-byte title Room44 archive contains exactly three map
resources:

- `m128.amp`, 35,456 bytes;
- `m128.bti`, 21,568 bytes;
- `s128.bti`, 7,264 bytes.

Its already-qualified room DZR contains one 56-byte `2DMA` record. It contains
no `FLOR` node, so the source's room-first floor lookup falls back to stage
metadata and ultimately floor 128 when no floor table is present. The map
resource names and the `2DMA` map ID therefore agree on the reached
`m128`/`s128` resource family.

## Partition Decision

The next patch must retain only the real room-info lifetime:

- floor-128 selection and the existing native `2DMA` view;
- exact three-resource lookup;
- `dMap_RoomInfo_c` initialization, entry, and delete reset;
- `dMap_RoomInfoCtrl_c` existence/entry/delete behavior;
- `dMap_c::setImage`, current-room publication, and `deleteImage`.

Static map drawing, AGB communication, icon/cursor owners, and all virtual
draw methods must be fenced or dead-stripped. The partition may use narrow
bridges to already-qualified game-info/room-data/resource owners, but must not
replace room-info state transitions with observers.

## Verification

- Full source census object compiles in Debug, optimized Release, and strict
  sanitizer configurations.
- Patch replay through 0128 remains clean.
- The exact private probe asserts the three named resources before the
  phase-4/delete lifetime completes.
- The prior 64-test public matrix remains green in all three configurations.

## Natural Stopping Point

`BW-P4-0110` remains active. Implement a target-PC source partition around the
real room-info/controller methods, link it below the exact phase-4 map seam,
and replace the observer-only set/delete proof. Do not link the complete static
map/draw graph.
