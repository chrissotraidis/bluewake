# 2026-09-01 Independent Review Disposition

**Processed:** 2026-09-02

This disposition records verification of the review against the repository and
subsequent Route B evidence. The review remains advisory; `CURRENT.md` and the
first entry in `BLOCKERS.md` remain authoritative.

| Finding | Disposition | Verified evidence and correction |
|---|---|---|
| F1 | ACCEPT | `route_b/include/bluewake/route_b/object_resources.hpp:22-42` exposes no archive-entry pointer, while `ref/tww/src/d/d_resorce.cpp:144-374` requires one; `ref/tww/src/JSystem/JAudio/JAIInitData.cpp:1-121` is marker-free and called by `JAIBasic::initInterfaceMain`. The four BlueWake owners remain labeled probes pending the user's keep/delete decision; none is the production resource or draw owner. |
| F2 | ACCEPT_WITH_CORRECTION | `ref/tww/src/d/d_s_logo.cpp:684-724,737-958` and `ref/tww/src/d/d_drawlist.cpp:655-666` confirm the bounded first-quad path. Full phase-two execution still needs authentic request seams, but the draw contract can be falsified independently; the 2026-09-02 packet tier now proves that path without J3D/JStudio/JAudio. |
| F3 | ACCEPT | The cycle-contract and performance ledgers record Route A's rejected architecture-scale speed shapes (`docs/archive/status/PERFORMANCE.md:3-25,78-111,236-253`). Route A remains frozen as the behavioral oracle. |
| F4 | ACCEPT_WITH_CORRECTION | `ref/aurora/lib/dolphin/os/` has no retail thread/message-queue/alarm service and `ref/aurora/lib/dolphin/vi/vi.cpp:1-55` is minimal. BlueWake has since promoted native thread, queue, and recursive-mutex owners and run the original DVD worker; alarms and full scheduling semantics remain open. |
| F5 | ACCEPT_WITH_CORRECTION | `ref/tww/src/d/d_s_logo.cpp:650`, `ref/tww/src/JSystem/JAudio/JAIZelBasic.cpp:1506-1508`, and `ref/tww/src/JSystem/JAudio/JAIBankWave.cpp:99-124` confirm the phase-zero gate and real source graph. Original group-2 DVD-to-ARAM transfer now reaches BankWave status 2 without a readiness callback; audible playback and DSP policy remain open. |
| F6 | ACCEPT_WITH_CORRECTION | Patches 0008-0013 use exact-width big-endian serialized fields and assert `SDIFileEntry` at 0x14; the authentic original-resource tier passes at 438/4,475 changed lines (9.79%). Patch 0028 extends that contract to authentic ARAM archive lookup at 4.13%; broad resource/J3D adaptation remains closed. |
| F7 | ACCEPT_WITH_CORRECTION | The static REL registry is sufficient for the admitted logo route (`route_b/src/static_rel_registry.cpp:1-80`), but the non-trivial actor initializer census remains required before broader Outset static linking. |
| F8 | ACCEPT_WITH_CORRECTION | `scripts/audit_repo.sh` now reports 20 tracked `local-research` files: the original 18 plus two later evidence READMEs. No file was removed and no policy was changed; disposition remains `ASK_USER`. |
| F9 | ACCEPT_WITH_CORRECTION | `CURRENT.md:3-5` now has a sole-authority rule and older claims are historical, but the large chronological ledgers remain TD-006 rather than fully archived. |
| F10 | ACCEPT | No unscripted human Route A session has been recorded; `docs/archive/status/GATES.md` and TD-002 retain that qualification. It is useful oracle evidence but does not block the active Route B frame tier. |

## Resulting Order

The review's actions 1 through 4 have now produced passing owner-level
evidence: original resource/phase one, original JUT/J2D/Aurora GX presentation,
promoted host OS threads and queues with the original DVD worker, and original
group-2 DVD-to-ARAM wave completion. Original phases zero through the bounded
first-frame portion of phase two compose in one process, and unchanged
`nintendoInDraw` owns 2D-opaque submission. The active successor is the
original asynchronous ARAM archive command and then the complete phase-two
preload fanout before title/J3D admission.

Open user decisions remain the tracked-evidence policy, permission/order for
the Dolphin Zelda HLE DSP seam, and whether to retain or delete the four
superseded BlueWake scaffolding services.
