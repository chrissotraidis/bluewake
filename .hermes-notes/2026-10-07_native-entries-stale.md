# Native entries and lean memory: certified hashes are stale (0/15 hooks)

## Summary

Building with --native-entries --lean-memory on Linux produces a module that
prints [native-entries] certified native entries enabled but inserts ZERO hooks.
The A/B on the Steam Deck showed flat performance (no [native-search],
[native-mtxcalc], [native-fifo], or [native-bg] counters appeared).

## Evidence

build/linux-native/logs/native-entries.log:

  native entries: 0/15 certified, 0 new hooks in 0 chunks

Every entry declines with "not the certified translation; not hooked":

  strcmp: not the certified translation (chunks_dol/chunk_0203_text1_8032D6E0.c); not hooked
  dStage_searchName: not the certified translation (...); not hooked
  cTgIt_JudgeFilter: not the certified translation (...); not hooked
  J3DMtxCalcSoftimage::calcTransform: not the certified translation (...); not hooked

The same build certified the OTHER native sets correctly:

  native game math: 12 certified entries, 12 new hooks in 6 chunks
  native vector: 9 SDK vector functions certified in 1 chunks

So this is not a translator-wide break or a Linux-ordering bug - it is specific
to scripts/windows/native_entries.py.

## Root cause

The certified body hashes in native_entries.py were recorded at commit 552ce1f
(2026-10-04) and have not been refreshed since. The current translation output
no longer matches them, so certify() rejects every entry.

The pinned RecompCore SHA changed between 552ce1f (0b86924) and now (35e037f),
and the DolRecomp/translation pipeline has accumulated changes in that window.

## Secondary issue: --lean-memory also does nothing

build/linux-native/logs/lean-memory.log:

  lean memory accesses: 0 in 0 chunks

fast_blocks.py emits prepaid copies (197,458 blocks in 812 chunks, confirmed),
but lean_memory.py's parse_copy requires a ctx->cycle_observation_suffix = Nu;
store to qualify an access, and none of the copies satisfy its conditions.

## What is NOT the fix

Hand-editing the hashes would defeat the certification safety property: a hook
is only inserted where the translation hashes to a value the native was
byte-compared against (tests/native_search_test.c, tests/native_mtxcalc_test.c).
The correct fix is to re-run those comparison tests against a fresh reference
module and regenerate the hashes from the verified translation.

## Requested action

Owner: certified-native work (Elliott). Please refresh the certified hashes in
scripts/windows/native_entries.py against the current translation (and re-check
lean_memory.py against the current fast_blocks.py copy shape).

Repro (Linux builder):

  python3 scripts/linux/build.py <GZLE01.iso> --out build/linux-native \
      --native-entries --lean-memory --retrain

Or, cheaply, without a full build:

  python3 scripts/windows/native_entries.py --hashes build/linux-native/composite-src
