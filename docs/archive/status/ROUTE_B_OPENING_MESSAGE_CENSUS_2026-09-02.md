# Route B Opening Message Census

## Result

One private-disc process mounts authentic `/res/Msg/bmgres.arc` through
Aurora DVD and original JKR, resolves `zel_00.bmg`, and parses the eight
message entries selected by unchanged `dScnOpen_proc_c`. The archive expands
to 640,672 bytes and the BMG resource is 639,328 bytes.

All eight entries use textbox type 5 and draw type 2. Their payloads contain
between 53 and 84 text bytes and no `0x1a` control sequences. Debug, Release,
and strict ASan/UBSan produce identical output. This is an input and branch
census only; it does not claim that opening narrative execution passes.

## Source Closure

The original methods directly required by retail opening message setup and
execution total 3,913 lines:

| Owner | Lines |
|---|---:|
| `fopMsgM_msgGet_c` lookup methods | 54 |
| data processor construction, init, and glyph metrics | 185 |
| `stringLength` | 1,419 |
| `stringShift` | 65 |
| `stringSet` | 2,190 |

This is 38.34% of the 10,205-line translation unit. The two large parser
methods contain all gameplay control-code branches, so ordinary compilation
retains references to more than one hundred named call sites even though the
opening inputs reach only plain glyph processing. Compiling the whole source
with the opening frontier repeats unrelated meter, NPC, shop, and J3D header
admission and is not a bounded successor.

## Reframed Action

Keep `BW-P4-0084` active. Create a target-PC source partition that retains the
exact original reached definitions and gives every control-only dependency an
abort-on-reach fence. Measure adaptation density and classify unresolved
symbols before executing it. This is not permission to write a reduced BMG
parser or to infer message completion from the absence of control codes.

Evidence is preserved under
`local-research/evidence/route-b-opening-message-census-v1-20260902/`.
