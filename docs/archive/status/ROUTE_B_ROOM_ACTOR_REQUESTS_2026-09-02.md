# Route B Room Actor Requests - 2026-09-02

## Boundary

The Room44 census showed that ACTR is a cohesive 172-request tier with its own
room-native owner. The existing stage ACTR path could not be reused because it
casts the base stage object to `dStage_stageDt_c` and maps only four stage
names. Requested profile execution remains outside this boundary.

## Result

Patch 0118 adds a room-owned decoded ACTR view to `dStage_roomDt_c` and ports
the retail ACTR request contract. Each 0x20-byte file record preserves its
parameters, position, angles, and set ID; admitted records receive room 44 and
the mapped profile's argument/GBA metadata before entering the standard create
queue. Saved records are skipped, append-allocation failure is harmless, and
unknown names release their append payload without creating a request.

The public contract covers all 25 Room44 ACTR resource names and 13 mapped
profiles. It also reconstructs the census-derived 172-record distribution and
asserts 172 new requests, including exactly 119 GRASS requests. Reset releases
and clears the native owner. No queued profile executes.

## Evidence

- Patch SHA-256:
  `f299d41c427015ed786ee7cbaead81460674912f446ef606501f77fd5650fa9c`.
- Debug ACTR object SHA-256:
  `a4808181ed2c3489a0d729b771f26374d5329c12711c11a565cbca8136e47200`.
- Debug focused executable SHA-256:
  `0f2fa984009fdb51975b9dd74dea48e086f926acd36f82934cd5442a176e1edf`.
- Release focused executable SHA-256:
  `bf5734f5b17ce95220fb30a454a18ce7785052093c956267db414f8d1afc9f86`.
- ASan/UBSan focused executable SHA-256:
  `cda3b9d0b2f087551844f1b9e2c57140965f5b477144c6cab7c5d911f3d3e90d`.
- Patches 0001 through 0118 replay cleanly from pinned TWW commit
  `03d27aa14389e648f51df2bc055ee3d53a3b67f2`.
- All 57 registered tests pass in Debug, optimized Release, and strict
  ASan/UBSan (`detect_leaks=0`, required by macOS ASan).

The repository audit retains its known P0 failure for 20 tracked
`local-research` evidence files; the dependency lock remains valid. No
BlueWake process or Simulator was launched.

## Next Boundary

Implement one room-owned scaled-object tier shared by the five TGDR records
and one SCOB record. Prove 0x24 decoding, scale propagation, KNOB00/KYTAG01
mapping, unconditional request behavior, TGDR `setDrTg` publication, allocation
failure, unknown-name cleanup, and reset lifetime. Keep aggregate reload, BG,
requested-profile execution, phase 4, BlueWake, and Simulator closed.
