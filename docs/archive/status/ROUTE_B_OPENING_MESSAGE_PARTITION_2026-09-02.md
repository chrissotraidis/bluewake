# Route B Opening Message Partition - 2026-09-02

## Result

The target-PC partition compiles the exact original opening-reached message
definitions from `f_op_msg_mng.cpp` in Debug, Release, and strict ASan/UBSan.
It retains approximately 4,117 original source lines. The finalized patch
records 20 insertions and one deletion, including partition guards around the
two exact font-centering helpers retained for the execution tier.

The original object has 91 unresolved imports. Sixty-five are gameplay
control-code methods that the authentic eight-message census proves absent.
They are supplied only as abort-on-reach link fences. A relocatable link of
the original object and fence object leaves 26 ordinary runtime imports plus
`abort`; those archive, font, HIO, game-state, and C runtime owners are not
stubbed.

## Reproduction

```sh
cmake --build build/route-b-aurora-gx \
  --target bluewake_route_b_opening_message_objects -j8
cmake --build build/route-b-aurora-gx-release \
  --target bluewake_route_b_opening_message_objects -j8
cmake --build build/route-b-aurora-gx-sanitize \
  --target bluewake_route_b_opening_message_objects -j8
bash scripts/prepare_route_b.sh --check
```

## Superseded Boundary

The execution boundary passed and is recorded in
`ROUTE_B_OPENING_MESSAGE_RUNTIME_2026-09-02.md`. The next blocker is the
incomplete scene-owned `dScnOpen_message_c::set_message/exec` behavior, not
another parser partition.
