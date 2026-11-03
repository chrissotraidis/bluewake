# Route B Opening Scene Composition - 2026-09-02

## Result

`BW-P4-0086` passes. One private-disc process mounts authentic `Opening.arc`,
`bmgres.arc`, `fontres.arc`, and `rubyres.arc` without retaining private
bytes. It constructs original `dScnOpen_proc_c` over original JKR heaps and
archives, original J2D layout/panes/fonts, all 22 opening messages, the
reconstructed retail message-state owner, and original pane helpers.

Every scene state 1-44 arrives at its source-derived tick. State 44 arrives at
tick 6,532. Historical Route A observations use two VI retraces per scene tick
and agree within their recorded one-retrace observation tolerance.

The probe calls unchanged `proc_draw` after every `proc_execute`. All 6,532
draws emit nonempty Aurora GX display lists, ranging from 9,792 to 19,936
bytes. Original stream prepare and play calls are recorded exactly once at
ticks 40 and 92. Those recording seams establish call order only; audio output
is not accepted.

Debug, Release, and strict ASan/UBSan pass. macOS LeakSanitizer is unsupported,
so the strict sanitizer run uses `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1`
and `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`.

## Falsification Sequence

The first composed execution reached state 1 at the exact tick, then aborted
inside the deliberate `J2DPrint` draw fence. This disproved the assumption
that the earlier layout-only J2D closure was sufficient for narrative draw.
The complete original 568-line `J2DPrint.cpp` owner was admitted instead of
weakening the fence or skipping text.

Patch 0039 changes 27 lines (15 additions, 12 deletions; 4.75%) for three host
portability shapes: bounded parser-length narrowing, native pointer-difference
arithmetic, and explicit conversion of C-library numeric results to retail
32-bit fields. Patch 0038 changes four preprocessor guard lines to expose
existing original pane and message-heap helpers. Patch stack 0001-0039 replays
from the pinned source.

## Acceptance Oracle

- State 1: tick 1.
- State 2/3: ticks 91/92; stream prepare/play: ticks 40/92.
- States 4/10/14/20: ticks 325/884/1,661/2,264.
- States 26/30/36/42: ticks 3,644/4,790/5,130/6,517.
- State 44: tick 6,532.
- Draw count: 6,532; every packet nonempty.
- GX packet range: 9,792-19,936 bytes.

## Reproduction

```sh
cmake --build build/route-b-aurora-gx \
  --target bluewake_route_b_private_opening_message_probe -j 8
build/route-b-aurora-gx/bluewake_route_b_private_opening_message_probe \
  <private-gzle01-disc-image>
```

Run only one private probe at a time. Use the matching Release and sanitizer
build trees for the other qualification tiers.

## Next Boundary

`BW-P4-0087` must visibly present this same source-native scene through one
Aurora Metal window. Draw at 60 Hz and call original `proc_execute` every
other presented frame to preserve retail 30 Hz scene cadence. Capture the
first narrative frame and state 44 while retaining the headless tick and
packet controls. This is presentation work, not permission to fabricate the
logo transition, claim audio playback, or admit J3D.
