# Route B OBJ_TORIPOST Link Census - 2026-09-06

## Boundary

Patch 0136 made the complete Toripost translation unit compile, but its Debug
object retained 107 unique undefined symbols. That raw count did not
distinguish already-qualified Route B infrastructure from actor-specific idle
and closed gameplay owners, so it was not sufficient to design an honest
profile execution tier.

## Result

A temporary fail-closed link probe referenced original
`g_profile_OBJ_TORIPOST` and successively admitted only existing qualified
Route B owners. The process/static-REL, actor core, collision root, room state,
stage resource, J3D model-create/model-draw, and foundation set reduced the raw
107-symbol Debug object surface to 74 unresolved symbols. Adding the existing
source-authentic shared collision and matrix runtime reduced it to 57.

The 57-symbol frontier classifies as follows:

| Owner family | Symbols | Disposition |
| --- | ---: | --- |
| dRes and solid-heap ownership | 6 | required by create/delete |
| McaMorf and BCK animation | 5 | required by create/wait/draw |
| collision, movement, cull, matrix, shadow | 15 | split live initialization from closed response |
| letter, save, item, and day/night policy | 13 | bounded fresh-save predicates; delivery closed |
| event, talk, present, particle, and audio | 15 | identity/init partly live; interaction closed |
| unrelated linked-owner diagnostics | 3 | exclude from actor closure |

The critical result is that the original morph owner is only four direct
Toripost symbols—constructor, `play`, `calc`, and `entryDL`—plus the one
`dLib_bcks_setAnm` transition function. Route B already partitions original
`m_Do_ext.cpp` for J3D model creation and draw. Extending that source partition
to retain original McaMorf behavior is smaller and more reusable than copying
or simulating the morph object in Toripost-specific test code.

The deliberately failing temporary executable was removed after the census;
no failing build target, synthetic proxy, private artifact, or new TWW patch
is part of this checkpoint.

## Evidence

- Raw Debug actor object: 107 unique undefined symbols.
- Existing qualified owner baseline: 74 unresolved symbols.
- Baseline plus source-authentic shared collision/matrix runtime: 57 unresolved
  symbols, classified above.
- Full default builds and all 65 public tests remained green in Debug,
  optimized Release, and strict ASan/UBSan before and after the experiment.
- Exact Room44 parent, standalone Stone2, and standalone BG regressions remained
  green in all three private configurations.
- The protected recompcore files retain their required SHA-256 values.
- No BlueWake process or Simulator was launched.

## Limitations

This is link-closure evidence, not a Toripost runtime claim. It does not prove
the exact `Toripost.arc`, BDL/BCK parsing, morph frames, collision state,
mailbox state, draw submission, or deletion. The counts are Debug-link facts;
Release and sanitizer instrumentation change raw object counts and will be
requalified after the owner slice links.

## Next Boundary

Promote a narrowly opt-in original `mDoExt_McaMorf` source slice from
`m_Do_ext.cpp`, including the constructor and the `setAnm`, `play`, `calc`, and
`entryDL` methods needed by Toripost. Reuse the qualified original J3D model
create/draw owners and add the exact `dLib_bcks_setAnm` transition without
admitting unrelated `m_Do_ext` or `d_lib` behavior. Require focused
construction/animation/draw evidence in Debug, optimized Release, and strict
ASan/UBSan before returning to the actor profile.
