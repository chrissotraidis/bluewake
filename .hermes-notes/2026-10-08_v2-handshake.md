# Native tests: v2 direct-calls handshake migration (in progress)

## What was done
native_search_test.c and native_mtxcalc_test.c migrated from the v1 handshake
(bluewake_composite_direct_calls, a hard stub returning 0) to v2
(bluewake_composite_direct_calls_v2 with a can_skip callback). The callback
returns false (direct-call skips off) to match pre-migration behavior (v1
returned 0, translation went round the loop, byte-exact comparison needs
identical cycle accounting).

## Result
Both tests now compile and get PAST the "direct calls or edge filter
unavailable" gate. They now fail further in, at the byte-exact CPUState
comparison:
- native_search_test: case 0 strcmp, byte 3536 (external_read fn-ptr region)
  differs: got A0 82 66 CB want 02 00 00 00.
- native_mtxcalc_test: case 0 J3DMtxCalcBasic, bytes 4-24 (fpr region) differ.

## Diagnosis
Natives report 0/0 (all declined) because the 15 certified hashes in
native_entries.py are stale (0/15, issue #179), so the native never runs and
the "native" result == input state. The mismatch is the TRANSLATION drifting
from what the tests were certified against at 552ce1f (RecompCore pin 0b86924
-> 35e037f -> 132a8d7a). Same root cause as the 0/15 hash staleness, now
surfacing at the CPUState-tail level.

## What this means
v2 migration is necessary and correct (removes the first blocker), but the
tests still can't pass until the translation is re-certified against a fresh
reference module (Elliott: re-run tests/native_*_test.c against a fresh
BlueWake module and regenerate hashes). Not hand-editable.

## Remaining on v1
native_bg_test.c, native_entries_test.c, native_search_entries_test.c,
native_search_judge_test.c. Same migration needed; same translation-drift wall.
