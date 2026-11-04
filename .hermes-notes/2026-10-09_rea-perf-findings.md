# REA pass on BlueWake perf (2026-10-09) — native entries are REGISTER-stale, not just hash-stale

## Method
REA MCP unavailable here (needs Node >=22, we have v20; Hopper/Ghidra absent). Did binary/source
forensics by hand: ran the already-ported Linux native comparison tests against
build/linux-native/composite/gGZLE01_recomp.so (glibc 2.38 vs container 2.41, loads fine).

## New finding (advances beyond "0/15 hashes stale")
native_search_test case 0 (strcmp): mismatch at CPUState byte 3536 = cycle_observation_suffix.
Native leaves 0xCB6682A0 (seed garbage), translation leaves 2 (its mtlr). So the native's hardcoded
"leaves suffix 2" assumption is broken against the CURRENT translation.

native_mtxcalc_test case 0 (J3DMtxCalcBasic): bytes 4-31 (gpr[1..7]) and 128-151 (fpr[0..2]) differ.
The native calcTransform computes different registers/matrix than the current translation.

=> Re-hashing native_entries.py alone will NOT re-enable these natives. The natives' register and
cycle-accounting assumptions were certified at 552ce1f (RecompCore 0b86924); the pin has since moved
0b86924 -> 35e037f -> 132a8d7a. Regenerating hashes against the current translation would certify
WRONG natives. The natives themselves need re-derivation (Elliott's certified-native work), then
re-hash. Do not hand-edit hashes (defeats byte-compare safety).

## Perf levers confirmed current
- Top lever = game-thread instruction gap (lean block copies, 158M vs 110M instr/retrace, ~30%).
  Blocked on Chris's "plays the same" acceptance decision + Elliott. Cannot be done solo.
- --no-cold: already measured NEGATIVE on the bird scene (2.2% slower, byte-identical). Done, closed.
- Native entries: ~7.4% of game thread at Dragon Roost (strcmp 6.4 + searchName 2.5). Blocked as above.
- Quick wins (runtime only, no module rebuild): Smooth Motion pacing step-down stuck (lever 3),
  renderer-fallback message (lever 4) — assigned to Codex in PERFORMANCE.md plan.

## Verified working
tests/posix Linux port builds+runs; v2 handshake (bluewake_composite_direct_calls_v2) wired in
native_search_test.c already (can_skip=false to match pre-migration cycle accounting). The .so at
132a8d7a exports all needed symbols. Layout probe: sizeof(CPUState)=3552, cycle_observation_suffix
@3536, fpr @128, gpr @0 (confirmed via offsetof against ref/recompcore GXRuntime include/core/cpu.h).
