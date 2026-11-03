# Route B Play Phase 4 Message Heap - 2026-09-02

## Result

Patches 0084-0085 extend the exact retail play `phase_4` route through:

1. `fopMsgM_createExpHeap(0x73EA1)` for the USA build;
2. its non-null retail assertion; and
3. `dComIfGp_setExpHeap2D(heap)` publication.

The function returns immediately afterward. `dStage_Create`, graphics tick
rate, HIO creation, attention, vibration, actor initializers, and every later
phase branch remain unentered.

## Ownership

The retained original message wrapper remains a one-line delegation to
`JKRCreateExpHeap(size, mDoExt_getGameHeap(), FALSE)`. Patch 0084 promotes only
the original target-PC `gameHeap` storage and getter from `m_Do_ext.cpp`; its
unrelated extension runtime remains excluded. The independent owner test
proves non-null allocation, parent ownership, and consumed parent space.

The composed phase test starts from a real JKR root, assigns it to the original
game-heap owner, constructs the qualified production-layout play owner, and
runs every earlier world/view operation. Its bridge records heap publication
as the eleventh state call and verifies the stored child pointer and parent.
No aggregate `g_dComIfG_gameInfo` is fabricated.

## Strict Sanitizer Finding

The first genuinely halting UBSan matrix exposed an older native ABI defect in
the already promoted demo owner. `JStudio::TCreateObject::mNode` follows a
virtual pointer, but its intrusive-list template retained retail offset `-4`
on a host with eight-byte vptrs. Patch 0086 uses `-sizeof(void*)` in the two
matching target-PC template instantiations and keeps `-4` for PowerPC. This
corrects owner and iterator semantics rather than suppressing alignment checks.

## Verification

- Debug: 31/31 tests passed.
- Optimized Release: 31/31 tests passed.
- Strict ASan+UBSan: 31/31 tests passed with unsupported macOS leak detection
  disabled and halting address/undefined-behavior checks enabled.
- Patch replay check passes through patch 0086.
- No BlueWake process or additional simulator was launched.

## Next Boundary

`BW-P4-0104` is resolved. `BW-P4-0105` begins at unchanged `dStage_Create`.
Its aggregate closure must be measured once before selecting a bounded owner;
tick-rate and HIO setup remain separate later boundaries.
