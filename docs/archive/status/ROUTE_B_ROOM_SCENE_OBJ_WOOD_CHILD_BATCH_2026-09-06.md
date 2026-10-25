# Route B ROOM_SCENE Obj_Wood Child Batch - 2026-09-06

## Boundary

The exact standard ROOM_SCENE parent ran BG and KYTAG01 but cancelled the nine
Obj_Wood requests with the other 168 phase-3 children. Obj_Wood's transient
profile and bounded shared packet owner were independently qualified; the open
boundary was their authentic request disposal and room cleanup inside the
composed parent lifetime.

## Result

The native reloader still publishes all 179 requests. The private registry now
admits BG, KYTAG01, and Obj_Wood; exactly five `woodb` and four `woodbx`
requests are retained with the two persistent children. Every Obj_Wood request
runs through standard creation, copies its exact append position into one
active wood unit, links that unit to Room44, and then intentionally disposes
its proxy process and create request after the unchanged create method returns
`cPhs_ERROR_e`.

The proof observes nine ground-placement calls, nine active units at the exact
request positions, nine Room44 list entries, no surviving Obj_Wood process,
and no remaining child create request. The shared packet remains live while BG
and KYTAG01 execute. BG's unchanged room cleanup clears all nine wood units
before its collision/heap teardown; the global packet owner remains reusable
through the parent lifetime and is explicitly removed only after ROOM_SCENE
finishes map, dRes, archive, native-owner, and room-heap teardown.

No new TWW source patch was required. The exact lifecycle reuses the already
qualified bounded wood placement and construction-only collision seams.

## Evidence

- Debug private executable SHA-256:
  `b4874fb386858305974cf77f3e450b54c809935825ce0c4affd8cb6924863f02`.
- Release private executable SHA-256:
  `325d01edb628427eed6b2b7f2fadbae3b0d6d05a269a81a312c671ed67feaf67`.
- ASan/UBSan private executable SHA-256:
  `e69c300fe6e228af2e8beb249f8422a34e8fdf5894de97678ddfcac681767670`.
- Debug, optimized Release, and strict ASan/UBSan emit the identical result:
  `room-lifecycle room=44 archive=async-phase1 dzr=native requests=3+179 children=bg+kytag01-deleted wood9=published-disposed-cleared map=paired delete=delayed archive=unmounted owners=reset heap=reset pass`.
- The standalone Obj_Wood synthetic regression passes after composition.
- All 65 registered public tests remain green in Debug, optimized Release,
  and strict ASan/UBSan.
- Patches 0001 through 0133 replay cleanly from pinned TWW commit
  `03d27aa14389e648f51df2bc055ee3d53a3b67f2`.
- The protected recompcore files retain their required SHA-256 values.
- The repository audit remains failed only by the known 20 intentionally
  tracked `local-research/` evidence files; this checkpoint adds none.

## Limitations

The bounded wood owner does not implement retail ground queries, animation,
collision response, audio, update, or rendering. The proxy profile never
reaches execute/draw because source-authentic creation deliberately returns an
error after publication. The other 168 phase-3 requests remain cancelled.
Grass packet simulation/rendering, persistent actors, map drawing,
representative gameplay presentation, another BlueWake process, and another
Simulator remain outside this proof.

## Next Boundary

Retain all 119 already-qualified GRASS requests beside BG, KYTAG01, and the
nine Obj_Wood requests. Prove the exact 982 grass, 61 tree, and 130 flower
publications, intentional transient process disposal, and BG-owned Room44
cleanup before parent teardown. Continue cancelling the remaining 49 requests
and do not admit another persistent profile.
