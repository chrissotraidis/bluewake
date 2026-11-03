# Route B Stage SCOB View - 2026-09-02

## Boundary

The preserved Outset SCOB chunk contains 50 scaled actor records. Its retail
records combine the same invalid 32-bit/little-endian host representation as
ACTR with a three-byte scale. This is also the final present stage handler
with process effects: every record immediately submits a standard-create
request.

## Result

Patch 0108 decodes all 50 records into stage-owned native storage and queues
their unchanged append payloads on the authentic current process layer. The
preserved names are `LOD01` through `LOD49`, with `LOD45` repeated by the final
record. All map to retail profile name `fpcNm_LODBG_e`; the host lookup admits
only that bounded observed family.

The public test proves exact source order, parameters, positions, angles, set
IDs, scales, room/argument/GBA defaults, layer ownership, and parent IDs. It
also covers the nonuniform first 14 scales, the final duplicate's nonzero Y
coordinate, and all 50 queued requests. No requested profile is executed.

## Evidence

- Preserved DZS SHA-256:
  `5f5a29bd46e5132b7446cff9085939cf2373eb471eaafadbfecb2748be3be113`.
- SCOB object SHA-256:
  `75a4cc4cd19e8455b90fe29c7f6e772c7268a90e5a7f0d773b29f508f33d7ede`.
- Debug test SHA-256:
  `00e9748fb6357ba5cca726d323252cdb5bbe1d8fc8c00237512c459d0d6624b6`.
- Patch SHA-256:
  `cad6c772b1f78b972165026bc268124af3e4c75d3369738922ececef78cc8d00`.
- Patches 0001 through 0108 replay cleanly from pinned TWW commit
  `03d27aa14389e648f51df2bc055ee3d53a3b67f2` and reproduce every touched
  source blob.
- Debug, optimized Release, and strict ASan/UBSan pass 50/50 public tests.

No LODBG profile, aggregate loader, room creation, BlueWake app, or Simulator
was launched for this tier.

## Next Boundary

Compose the 15 qualified present Outset chunks through one authentic
source-ordered stage-loader tier. Prove the complete native stage state and
the exact aggregate request queue while keeping queued profiles unexecuted.
This is a composition qualification, not permission to enter initial-room
creation.
