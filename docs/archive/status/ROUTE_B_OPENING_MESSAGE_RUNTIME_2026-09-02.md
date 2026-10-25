# Route B Opening Message Runtime - 2026-09-02

## Result

One private-disc process mounts authentic `Opening.arc`, `bmgres.arc`,
`fontres.arc`, and `rubyres.arc` through Aurora DVD and original JKR. Original
`J2DScreen::set` supplies the retail text-pane metrics. Original `JUTResFont`
constructs both fonts, and the exact original message manager runs lookup,
`dataInit`, `stringLength`, `stringShift`, and `stringSet` for all 22 IDs
`0x579..0x58e` reached by the eight chained calls in `dScnOpen_proc_c`.

Every control-only abort fence remains unreached and every message emits
nonempty text. Their page offset is zero because none crosses a page boundary;
the retail caller also ignores this return value. Debug, Release, and strict
ASan/UBSan pass.

The initial probe covered only the eight explicit `set_message` IDs. Static
recovery of `exec` showed that four calls chain consecutive IDs, correcting
the authentic parser surface to 22. The expanded probe passes without a new
control dependency.

## Falsification Tiers

The first run reached `JUTResFont: Unknown data block` before BMG lookup. After
size-preserving BFN field conversion, both fonts initialized and the first BMG
lookup failed. Converting serialized BMG header and entry fields then cleared
all eight inputs. These ordered failures localize both adaptations; no private
payload is rewritten or retained.

Patch 0035 partitions only exact `dMeter_msg_HIO_c` global construction and
proves the retail opening shift is zero. Patch 0036 owns serialized BFN/BMG
endianness. Patch stack 0001-0036 replays from the pinned source. The scoped
foundation matrices pass 10/10 in Debug, Release, and ASan/UBSan.

## Reproduction

```sh
cmake --build build/route-b-aurora-gx \
  --target bluewake_route_b_private_opening_message_probe -j8
build/route-b-aurora-gx/bluewake_route_b_private_opening_message_probe \
  <private-gzle01-disc-image>
```

Run only one private probe at a time. Release and sanitizer use the matching
`route-b-aurora-gx-release` and `route-b-aurora-gx-sanitize` build trees.

## Message State Owner

Patch 0037 reconstructs incomplete `dScnOpen_message_c::set_message` and
`exec` from the checked-in GZLE01 instruction ranges and private DOL constants.
It is behaviorally equivalent, not claimed byte-matched. The eight chains end
on the correct final IDs in exactly 233, 482, 731, 572, 1,349, 1,100, 233, and
1,310 scene ticks, matching both the DOL formula and historical Route A oracle.

## Next Boundary

Completed by `BW-P4-0086`; see
`docs/archive/status/ROUTE_B_OPENING_SCENE_COMPOSITION_2026-09-02.md`. The next
boundary is paced native macOS presentation of the same original scene. Do
not widen parser, font, archive, audio, or J3D scope.
