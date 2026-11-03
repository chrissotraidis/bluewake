# Route B Stage Passive Metadata - 2026-09-02

## Boundary

The remainder census grouped seven present Outset handlers with the same
runtime ownership: RARO, Pale, Colo, Virt, SCLS, EVNT, and EnvR publish
stage-lifetime metadata and do not create a process. Their old retail
representations nevertheless cannot be used unchanged on 64-bit macOS.

SCLS, EVNT, and EnvR have byte-only entries but still depend on native counted
owners. RARO contains native vectors and angles; Pale, Colo, and Virt contain
big-endian numeric fields. Colo's change rate is also writable in original
environment code, so exposing an immutable endian wrapper there would be an
incorrect runtime contract.

## Result

Patch 0106 gives the stage owner native arrays for RARO, Pale, Colo, and Virt,
with exact serialized record types and explicit endian conversion. SCLS and
EVNT receive native counted wrappers over their immutable byte-exact entries;
EnvR publishes its byte-exact entry view directly. All four environmental
entry counts are published alongside their pointers. Stage reinitialization
releases every owned array and resets all wrappers.

The public test uses the authentic Outset counts: one RARO, 57 Pale, ten Colo,
37 Virt, 212 SCLS, 57 EVNT, and 52 EnvR entries. It checks every converted
numeric field and every preserved byte across the generated payloads, native
ownership, and pointer/count publication. It does not enter environment
lifecycle, event management, camera-arrow consumption, path loading, or actor
creation.

## Evidence

- Preserved DZS SHA-256:
  `5f5a29bd46e5132b7446cff9085939cf2373eb471eaafadbfecb2748be3be113`.
- Passive metadata object SHA-256:
  `603475bc18ff96f9d8b25516b9df6563ab7b007c78cdfa052d921eb135a58201`.
- Debug test SHA-256:
  `522c249d188cee78847d4913e7712e8b052c09cef958c93828e67e18131efd80`.
- Patch SHA-256:
  `d6b39c2e8c762ed541b610ca75c156c5c59e2ad85ac00e114a430dfa3c0813e7`.
- Patches 0001 through 0106 replay cleanly from pinned TWW commit
  `03d27aa14389e648f51df2bc055ee3d53a3b67f2` and reproduce every touched
  source blob.
- Debug, optimized Release, and strict ASan/UBSan pass 48/48 public tests.
- The repository audit validates the dependency lock and reports only the same
  20 tracked `local-research` evidence files awaiting user disposition.

No BlueWake app or Simulator was launched for this tier.

## Next Boundary

Qualify RPPN and RPAT together. The preserved payload contains 40 sixteen-byte
points and four twelve-byte paths; each serialized path carries a 32-bit point
offset relative to the point table. Build one stage-owned native graph and
prove all four ranges and exact point data before admitting SCOB or composing
the aggregate loader.
