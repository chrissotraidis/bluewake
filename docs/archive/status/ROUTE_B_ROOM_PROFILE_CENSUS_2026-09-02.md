# Route B Room Profile Census - 2026-09-02

## Boundary

The complete Room44 activation now submits BG plus 178 actor requests, but no
requested profile has executed. This census compiles each of the 16 profile
translation units independently with the strict Route B target-PC flags and
classifies the first construction frontier.

## Result

Five profiles compile unchanged. Eleven stop at bounded source-portability
errors rather than a broad missing subsystem.

| Profile | Records | Source lines | Native size | Strict compile | First frontier |
| --- | ---: | ---: | ---: | --- | --- |
| BG | 1 | 470 | 1,056 | Pass | 64 owner symbols; room models, animation, collision |
| GRASS | 119 | 218 | 904 | Pass | 22 symbols; grass/tree/flower batch owners |
| KAMOME | 10 | 1,582 | 2,320 | Fail | pointer stored in `u32` |
| Obj_Wood | 9 | 81 | 1,416 | Pass | 37 symbols; wood batch owner and collision typeinfo |
| Obj_Lpalm | 8 | 218 | 1,000 | Fail | pointer stored in `u32` |
| TSUBO | 8 | 3,647 | 2,888 | Fail | signed/narrow aggregate initializers |
| Lwood | 6 | 200 | 984 | Fail | pointer stored in `u32` |
| KB | 3 | 2,640 | 3,600 | Fail | pointer width, narrow constants, camera view |
| KANBAN | 2 | 1,189 | 2,448 | Fail | signed/narrow aggregate initializer |
| KN | 2 | 601 | 1,848 | Pass | 62 symbols; KN archive, model, collision, child fanout |
| BRIDGE | 2 | 1,535 | 70,440 | Fail | signed/narrow aggregate initializer |
| Stone2 | 1 | 812 | 2,400 | Fail | typed aggregate and camera view |
| OBJ_TORIPOST | 1 | 1,056 | 3,208 | Fail | pointer width and narrow constant |
| OBJ_IKADA | 1 | 1,644 | 5,768 | Fail | pointer width and narrow constant |
| KNOB00 | 5 | 944 | 1,088 | Fail | missing `std::tolower` declaration |
| KYTAG01 | 1 | 130 | 920 | Pass | 7 symbols; environment-light wave state only |

`KYTAG01` is the smallest honest execution cohort. It has one request, no
archive load, no model or collision construction, no child request, and the
smallest link frontier. Its create method installs one wave influence into the
already-qualified environment-light owner and completes synchronously.

GRASS and Obj_Wood are the next cohort, but both are batch contributors that
populate shared play-state packets and intentionally return an error result so
their proxy actor is discarded. BG is essential to visible geometry but is a
larger room-resource, J3D, animation, heap, and collision closure. KN loads an
object archive and can queue child actors, so neither should be conflated with
the first profile execution proof.

## Evidence

- Strict flags match `bluewake_route_b_target`, including pointer conversion
  errors and native-width shortening errors.
- Preserved compile logs and checksums:
  `local-research/evidence/route-b-room-profile-census-v1-20260902/`.
- SHA-256 of the preserved checksum manifest:
  `5f0282bdf6d11b9e5b08c4b3feb94485859f49edb493f4b54fb5250337b98c6e`.
- All 60 registered tests remain green in Debug, optimized Release, and
  strict ASan/UBSan from patch 0121 qualification.

No BlueWake process or Simulator was launched.

## Next Boundary

Install only the original KYTAG01 profile into the native registry and execute
the exact Room44 request through standard creation. Prove native actor
allocation, append propagation, synchronous completion, one environment wave
registration, steady execute/draw behavior, and deletion cleanup. Keep BG,
the other 177 reload requests, phase 4, BlueWake, and Simulator closed.
