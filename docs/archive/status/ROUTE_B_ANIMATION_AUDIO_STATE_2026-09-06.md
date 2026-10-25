# Real BAS resources and animation-audio state — 2026-09-06

Base `8fb2f50`; implements the first coherent audio owner selected by
`ROUTE_B_PLAYER_OWNER_REVIEW_2026-09-06.md`. BW-P4-0126 remains active.
This proves resource/state behavior, not sound output or phase three.

## Ownership and reconstruction

Patch 0178 keeps BAS as an explicitly serialized view: its header remains
eight bytes and each frame record remains 32 bytes. Multi-byte integers and
floats use endian-aware fields. The unused serialized word at header offset
four is opaque u32 data, not a widened/relocated host pointer. Searches found
no dereference of that field in the relevant original owners.

Link already owns a 0x200-byte sound-animation buffer allocated in its actor
heap. Original setSeAnime copies the embedded BAS block there, then calls
original initActorAnimSound. Keeping the serialized layout preserves that
copy/reuse contract without a second allocation or dangling decoded view.
New native assertions check offset ordering and that all advertised records
fit within the copied extent. The predicate uses byte reads and division to
avoid alignment assumptions and count multiplication overflow. This is not
a complete hostile-archive validator; upstream allocation/decompression bounds
remain separate owners. The original init API itself has no size parameter.

JAIZelAnime::setPlayPosition was empty in the pinned source. Its reconstructed
state behavior follows the verified private DOL range documented in the owner
review: null data returns without writes; otherwise scan to the first start
frame ordered-greater-or-equal to the requested position, or entry count, then
write dataCounter and mCurrentTime. Both retail direction branches perform
the same writes. Native NaN comparison behavior preserves the scan decision;
this is behavioral reconstruction, not MWCC byte matching or a claim to model
PowerPC FPSCR exception flags.

The explicit animation-state composition uses the original constructor,
initActorAnimSound and stop bodies, plus reconstructed setPlayPosition. It
replaces the init abort fence for PLAYER only. Existing sound-emission and
sound-stop diagnostic boundaries still abort on use. The upstream empty
startAnimSound/setSpeedModifySound methods are not admitted as successful
behavior. Tests cover initialization with inactive sound slots, not active
voice stopping/recycling or actual mixing/output.
The init body now reads a slot's frame-data pointer only when its sound pointer
is non-null, avoiding an unused indeterminate-pointer read for inactive slots.

The prior review's five unchanged static actor-service files are composed as
a group in the same build. This preserves their original main-DOL boundary;
it does not qualify complete actor profiles or every helper in those files.

## Independent and authentic-resource evidence

The new public animation-state test uses explicitly encoded big-endian bytes
and precomputed expected positions. It covers forward/reverse initialization,
duplicate times, before/on/after boundaries, infinity, unordered entry/request
values, zero count and null data. Null/truncated/insufficient record extents
are rejected by the structural predicate. Sound emission is not invoked.

The private PLAYER probe reads compressed resources through original archive
readIdxResource, including the archive's BCKS group. The first BCK-only query
found no BAS resources and failed; adding the actual alternate group fixed
the selection, not the format or expected values.

Across 91 real embedded BAS resources, 186 records match independently decoded
header fields, sound IDs, frame/pitch floats, flags and byte fields. Copies
use an aligned 0x200-byte owned buffer like Link. Original initialization plus
reconstructed positioning produce 1,662 matching state comparisons. Actual
Link phase two and event identity then still pass. This is real asset/state
integration but not a claim that Link's makeBgWait or setSeAnime ran.

## Verification and remaining work

All 76 public tests pass Debug, optimized Release and strict ASan/UBSan.
The real BAS checks, actual Link phase-two/identity and PLAYER model checks
pass all modes. Accumulated private camera, attention, sea, DZB, heap/morph,
BG, Stone2 and room lifecycle probes were rebuilt and replayed in all modes.
Full original JAIAnimation.cpp also passes a strict syntax-only probe with
the new serialized field types; full audio execution is not inferred.

TWW preparation through 0178 and Aurora preparation pass. Ten player-asset
Python tests pass. Protected source and lock hashes are unchanged. Repository
audit retains only the known 20 tracked research files. No GUI, Simulator,
live save, sound output, gameplay or performance test was run.
Private evidence: ignored evidence/route-b-animation-audio-state-20260906.
All-mode production censuses retain four/41/52 symbols for phase two,
phase-two/three and phase-two/execute (intentional linker exit 2). These are
static dependencies, not executed calls or completion percentages.

Next continue the selected animation-audio subsystem: reconstruct and verify
its missing emission methods against the verified DOL, retain exact source
ownership for sound requests/slots, and connect the remaining audio/runtime
owners toward actual phase-three execution with real Room44 ground. Do not
substitute empty emission bodies, skip initialization, or turn symbol counts
into completion percentages. No external blocker or P4 promotion.
