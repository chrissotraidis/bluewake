# Route B Stage Environment Requests - 2026-09-02

## Result

Patches 0089-0090 retain unchanged `dKankyo_create` and its unchanged
one-line `fopKyM_Create` manager owner. The test initializes the real process
runtime, executes the owner, and walks the real standard-create queue. It
finds exactly four requests in retail order:

1. `fpcNm_KANKYO_e`
2. `fpcNm_KYEFF_e`
3. `fpcNm_KYEFF2_e`
4. `fpcNm_ENVSE_e`

Every request belongs to the current process layer and carries null create
callback, callback data, and append data. This qualifies authentic request
enqueue ownership, not profile linking, process construction, or execution.

The first source reading incorrectly described ten requests. Direct inspection
of the four-call body and the executable queue oracle corrects the ledger.

## Census

The stage owner object imports only `fopKyM_Create`; SHA-256 is
`6202074eb484e7d5aaa7af341ecc3a7983f34e25c51906cbf9959a725e163303`.
The broad manager unit compiles but imports nine symbols because unrelated
helpers share the file. Patch 0090 isolates the unchanged manager call, whose
real closure is `fpcLy_CurrentLayer` plus `fpcSCtRq_Request` in the already
qualified process runtime.

The four original profile units total 4,190 lines. `d_kyeff2.cpp` compiles
unchanged and retains six owners: four weather lifecycle functions,
`g_fopKy_Method`, and `g_fpcLf_Method`; object SHA-256 is
`f06da2445ecde8a981efd31221004530f375820308cb445da6166fa15177ca54`.
`d_kyeff.cpp`, `d_envse.cpp`, and `d_kankyo.cpp` stop at the deliberate
target-PC incomplete `camera_process_class` policy from patch 0005. Adding
`d_camera.h` was tested and removed because that class is explicitly excluded
on target PC. `KYEFF` and most reached `KANKYO` accesses need the portable
camera base view; `ENVSE` uniquely reaches the nested camera engine.

## Adaptation

- Patch 0089: 35 changed lines across the 3,693-line stage source/header
  surface, 0.95%.
- Patch 0090: 10 changed lines across the 143-line manager source/header
  surface, 6.99%.
- Both patches change partition/include ownership only; the two entered retail
  function bodies are unchanged.

## Verification

- Debug: 33/33 tests passed.
- Optimized Release: 33/33 tests passed.
- Strict ASan+UBSan: 33/33 tests passed with unsupported macOS leak detection
  disabled and halting address/undefined-behavior checks enabled.
- Patch replay passes through patch 0090.
- The repository audit retains only the known `local-research` provenance
  failure; dependency-lock validation passes.
- No BlueWake process or Simulator was launched.

## Next Boundary

Qualify original `KYEFF2` profile creation first. Retain its real profile and
four weather lifecycle owners with the generic leaf/environment method tables,
then execute one real standard request through completion. Do not use a
synthetic profile, claim the other three camera-blocked processes, or compose
aggregate stage creation yet.
