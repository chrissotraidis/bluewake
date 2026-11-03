# Route B JAudio Position Reset - 2026-09-02

> **2026-09-06 correction / implementation WIP:** The empty-body contract below
> is historical source-composition evidence, not authentic reset behavior.
> Locked GZLE01 DOL inspection shows all three routines are 12 bytes:
> `li r0,0; stw r0,offset(r3); blr`, with offsets 0x1b80 (sea), 0x1dd0
> (river), and 0x1ec0 (window). Patch 0206 now clears the corresponding native
> members; the test requires those resets and preservation of all other bytes.
> Corrected focused tests and integrated private/public matrices pass all modes;
> see ROUTE_B_PLAYER_DRAW_2026-09-06.md for the verified increment.

## Result

Patch 0098 partitions the pinned TWW JAudio source so the KANKYO creation
closure can own `JAIZelBasic::zel_basic` and exactly the three reached
position-reset methods without linking the broad audio control layer.

The source limitation matters: `initWindowPos`, `initSeaEnvPos`, and
`initRiverPos` are checked-in `Nonmatching` empty bodies. This milestone does
not claim recovered retail reset semantics. It proves the authentic pinned
source contract: the real singleton resolves to the selected object and all
three calls preserve every byte of fully dirtied object storage.

Patch density is 12 additions and one deletion across the two 1,962-line
translation units (0.66%). No method body changes. All 40 public tests pass in
Debug, optimized Release, and strict ASan/UBSan.

Debug SHA-256 values are:

- `JAIZelAtmos.cpp.o`:
  `2453bc05c47c70aacc49f0552d0a50aff90995ec3da7cf713901df666ee2970d`
- `JAIZelBasic.cpp.o`:
  `126b380906c9dec801778e9b8ed37044cfb419636d629c952e86a0ef14213e56`
- composed test:
  `41a1defd359645888a7f7dff52521f68d3f1d847c95b870ab49e47457ba9a4dc`

## Next Boundary

Qualify unchanged `dSv_event_c::isEventBit` and
`dSv_player_collect_c::isSymbol` against the real game-info owner, then own
`g_regHIO`. Only after those three owners pass may the loop re-link and execute
one real KANKYO request. Broad audio and save mutation/loading remain closed.
