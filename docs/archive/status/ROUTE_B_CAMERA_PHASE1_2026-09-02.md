# Route B CAMERA Phase 1 Qualification

**Date:** 2026-09-02
**Blocker:** `BW-P4-0105` / Route B play phase-4 stage creation
**Patch:** `patches/tww/0103-partition-camera-phase1.patch`

## Result

The CAMERA request queued by the Outset `RCAM` handler now resolves the retail
`g_profile_CAMERA`, allocates the real portable camera process, and executes the
unchanged first creation phase through the authentic standard-create runtime.
The phase publishes the camera in `g_dComIfG_gameInfo`, copies the window and
player IDs into the process parameters, calls the original
`JAIZelBasic::getCameraInfo`, clears the active window count, and enables
autofocus.

The second retail phase then asks for player slot 0. No player has been created
at this point in the qualified stage-loader order, so it returns `cPhs_INIT_e`.
That is a legitimate pending creation state: the CAMERA process remains owned
by `standard_create_request_class::base.mpRes` and is not yet present in the
executor list. The test observes that ownership directly. It would be incorrect
to require `fpcEx_SearchByID` to find the process before creation completes.

## Boundary

This result qualifies CAMERA profile lookup, allocation, phase dispatch, and
the exact phase-1 state contract. It does not qualify phase-2 `dCamera_c`
construction, list insertion, execute, draw, delete, or a usable gameplay
camera. Those paths remain abort-fenced.

The result changes how the next action is interpreted. `ACTR` follows `RCAM`
in the aggregate stage-loader table and is the next present Outset chunk, so
the next tier still returns to source-ordered stage decode, promotes the
repeated counted-record host-view construction into one shared stage-owned
helper, and qualifies those `ACTR` records. However, ACTR contains generic
stage actors, not the player. This `stage.dzs` has no `PLYR` chunk; the player
arrives later through `dStage_roomInit`, room-scene creation, and the room
loader's `PLYR` handler. Full camera-engine construction is deferred until
that authentic room-owned player dependency exists.

## Evidence

- Clean replay: patches 0001 through 0103 apply to pinned TWW source.
- Debug: 45/45 public tests pass.
- Optimized Release: 45/45 public tests pass.
- Strict ASan/UBSan: 45/45 public tests pass.
- CAMERA census unresolved imports: 13, consisting of qualified phase-1 owners,
  four deliberate phase-2/lifecycle fences, and framework tables.
- CAMERA census object SHA-256:
  `4dc6175e940d76f61c63787b951b5a72ecaaa22cac2d146d8a77b628b9f208ff`.
- Debug test SHA-256:
  `50618031ce612284b839d9a1552125f56c0db8ce012ac19a0fb68e55be6e524f`.
- Patch SHA-256:
  `9dfc574f2537ab1636e7d8ddf18038c063b900471fbc65ff967932abf8747b2b`.
- Repository audit: dependency lock valid; failure remains limited to the same
  20 tracked `local-research` files awaiting user disposition.

No BlueWake app or Simulator process was launched for this source-native tier.
