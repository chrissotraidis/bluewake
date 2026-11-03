# Route decision, opened 2026-09-22

Opened under docs/GOAL_LOOP.md section 8.1. The trigger is not a crash or a stall in wall
terms: it is that the bounded-fix program on the shipping route has produced a run of measured
refutations rather than wins, ending with a body-side candidate refuted by the emitter's own
structure (docs/archive/status/CURRENT.md, 2026-09-22). This page is the dossier section 8.1 requires.

## 1. The state, measured

| what | value | how |
| --- | --- | --- |
| headless play window | 395.9 M instructions a retrace | scripts/bench_instructions.sh |
| rendered play window | 487.1 M instructions a retrace, renderer +90.6 M (+22.9%) | scripts/bench_rendered.sh |
| rendered wall | median 40.0 ms (25.0 fps), p99 58.4 ms | scripts/bench_report.py on a frame-timing run |
| the emitted bodies | 73.5% of the main thread; 27.24 host instructions per guest cycle | owner shares; scripts/bench_chunk.sh |
| the per-block constant | ~5% of the window, about 1% extracted this session | the boundary census |
| the target | 60 retraces a second = 16.667 ms; the gap is 1.7x headless and 2.4x rendered | |

## 2. The bounded fixes that are exhausted, with their numbers

- **The access path.** One redundant clause measured and removed (~0.6% on the route). The
  whole-path ablation (unchecked) is unsound and diverges; the shipping accessors are already
  static inline in the generated header, so there is no accessor call to remove and only the
  load-bearing clauses remain.
- **The cycle accounting, 16% of the emitted body**, measured by sound ablations: prepaid 9.8,
  no-guard 6.0, no-suffix 6.4, cheap-budget 3.3, no-pc +0.7 (removed is slower). The test cannot
  be removed without a second body, which measured +2.4% against, and a block-local downcount
  cannot be initialised once because the computed-goto table makes every instruction an entry.
- **The rest of the emitter-side list**, each measured: the pc store (negative), branch hints (a
  14% layout regression), the register file (null), __restrict (null), the pc cache size (null),
  the probe's inlining (already inlined, and two-arity outlining cost +0.53%).
- **The host-side per-block path**: the edge-service refresh call (-0.25, landed), the alias-state
  revalidation (-0.42, landed), the overlap guard's memo (null), the DSP dispatch guard (-0.37,
  landed), the DSP frame (null), the DSP callbacks (null).
- **The renderer**: the FIFO translation is on a worker and verified; what is left there is a
  bzero from a payload assignment and a spread of small copies, worth a few tenths of a percent
  of the window between them.

## 3. The affected surface

The emitted bodies: 73.5% of the main thread and roughly 81% of the rendered window's
instructions. The remaining 1.7-2.4x lives there. Everything else measured - the renderer, the
edge service, the device service, the dispatcher - is a few percent each and is either landed or
priced.

## 4. The routes available under the project's ground rules

The ground rules forbid a runtime PowerPC JIT and captured emulator state as normal
initialization, and they allow static recompilation, source reconstruction, compatible subsystem
replacement and evidence-based route pivots. So the options are not JIT-versus-translation:

1. **Route A, a better static shape.** The emitter's cost per guest instruction is 27.24 and the
   measured structural floor of the current shape is roughly 20-22 of those (the cycle accounting
   plus the access clauses plus the guards). Getting to authentic speed needs about 13. A better
   static shape - register-resident guest state across a basic block, or a code layout that does
   not re-materialise it per operation - is the only remaining A-side idea, and it is a
   re-emit of every chunk, not a trim.
2. **Route B, the source-native route.** The pin already carries the tww decomp under ref/tww, a
  268-patch series under patches/tww, the GXRuntime/aurora host stack, and this project's own
   Route B history through world integration. Its cost per guest operation is set by compiled C
   rather than by a translator, and it needs no dispatcher at all. What it owes is coverage: the
   decomp's completeness for the scenes the campaign needs, and the remaining port work its
   patch series records.
3. **Both, as now.** Route A ships and Route B accumulates, which is what the tree has been
   doing; the question this dossier asks is whether that is still the cheaper order.

## 5. What a route pivot would preserve

Per section 8.3: the topology and dependency locks, the public-safe manifests and the coverage
catalogue, the Apple host and runtime adapter interfaces, the normalized input, settings, importer,
save and diagnostics work, the test routes and reference digests, and the compatible clean host
services all survive a change of execution core. What is invalidated is the evidence tied to the
composite: the instruction counts, the digests and the censuses on this page.

## 6. The falsifiable spike that decides it

One measurement, not an opinion: **compile one function of the play window from the tww source and
measure its host instructions per guest operation under the same gate the A-side uses.** The
function to pick is the one the rendered capture says dominates the guest bodies -
func_803256E0, which drives the GX path and is 2.53 percent of the rendered main thread - or, if
that one is entangled with the GX front end, the next in the list.

- If B's cost per guest operation is at or below about 10 host instructions, the 1.7x is
  reachable on B and the project should promote it: the campaign is then about coverage, not
  about speed.
- If it is above about 15, B buys nothing on this host and Route A's re-emit is the cheaper
  path to authentic speed.
- Between those, the spike reports the number and the decision waits for the second function.

The spike is falsifiable in the project's own terms: it produces an instruction count for a named
function on a named route, and either of the two thresholds above settles which route to fund.

## 7. Amendment, later the same day: the spike needs a mapping step first, and the decomp's
## coverage is not the obvious wall

Section 6 proposed compiling func_803256E0 from the tww source. Attempting it found two things
worth recording before the spike is run.

**The emitted symbols are chunk boundaries, not function starts.** func_803256E0 and the other
seven names in the rendered capture are the recompiler's partition entries - 4,096-instruction
chunks - so none of them is a function entry in the guest's own symbol map: all seven are absent
from ref/tww/config/GZLE01/symbols.txt, while that map is comprehensive at 24,275 symbols and
does cover their neighbours (GXInitGX at 0x80320354 for the 0x8032 region, the d_wpot_water
statics at 0x80240038 for the 0x8024 region). So the hot *code* is inside those chunks rather
than at their entry, and the spike's first step is a mapping: a deeper sample under the hot chunk
gives the inner guest pcs, and those pcs go through the decomp's map to the source.

**And the coverage that the spike was meant to test looks better than the dossier assumed.** The
map covers the engine and SDK regions as well as the game's own - GXInitGX, GXCPInterruptHandler,
GXInitFifoBase are in it - so route B's distance to a running play scene is set by the port work
its 268-patch series records rather than by missing source. That does not change the decision the
spike is for, but it changes what the decision is about: the question is the port's completeness,
not the decomp's coverage.

**The amended spike.** (1) Map the hot chunk's inner pcs to the decomp's functions, which is one
sample and one lookup. (2) Compile one of those functions from source with the decomp's own flags
and count its host instructions. (3) Divide by the guest operation count of the same function,
read from the DOL. The thresholds in section 6 stand: at or below about 10, route B's speed is
settled and the campaign is about port coverage; above about 15, route A's re-emit is the cheaper
path.

## 8. The spike's result, and the decision

Step one (section 7) named the subject: **J3DSys::reinitTevStages**, hot at about 475,000
returns over the certified route, self-contained, and with a real symbol in the decomp.

Step two measured both sides of it.

- **Route B**, from the compiled object already in build-route-b-private-release: **177 host
  instructions for 194 guest instructions, 0.91 per guest instruction.**
- **Route A**, from the emitted C for the same guest range: **1,119 emitted statements plus 194
  labels, 5.8 emitted operations per guest instruction**, and the chunk-level instrument puts the
  emitted body at 27.24 host instructions per guest instruction, so A's true figure lies between
  those two.

Same function, same guest work: **0.91 against 5.8 to 27.2, six to thirty times cheaper**, far below
the section 6 threshold of ten. Caveats: both A figures are static counts and A's is a lower bound,
B's static count ignores its loops, and this function is data-shuffling rather than the worst case
for a port - but none of that closes a six-times gap.

**Decision: promote route B.** Its speed is settled with room to spare. What stands between the
project and a running play scene is the port's coverage - the 268-patch series and the scenes it
does not yet reach - not the cost of executing guest code. Route A remains the shipping route and
its remaining trims are tenths of a percent against a measured 1.24x ceiling, so continuing to
spend on them buys speed the product will never see.

## 9. Follow-up, the same day: the handoff's coverage is an enumerated owner list

Section 8 left "the port's coverage" as a claim with a name and no size. The first campaign step
measured it, and the measurement is in [CURRENT.md](CURRENT.md) under 2026-09-22.

The LOGO-to-opening handoff is requested and unanswered: the scheduler diagnostic's LOGO scene
passes its whole DVD-wait guard - queue drained, no object resource outstanding, no reset - calls
`dComIfG_changeOpeningScene`, and stays in action 10 because `fpcPf_Get(fpcNm_OPENING_SCENE_e)`
returns NULL against a static rel registry holding the LOGO scene alone. Registering the opening
scene turns the failure into a link failure, and the unresolved set *is* the play scene's owner
list: `cDylPhs::Link`/`Unlink` (**now landed**, `route_b/src/native_dyl_phase.cpp`), the
`JStudio::TObject_*` and `TFunctionValue_*` families, the `JStudio_J*` `TAdaptor_*` constructors,
`dStage_Create`/`Delete`, `dStage_roomControl_c::checkDrawArea`, `dSnap_*`, `fopDwIt_Begin/Next`,
the grass/tree/wood/flower/magma packet owners and `daObjTribox::Act_c::reset`.

Three of those are 64-bit portability rather than missing source - `jstudio-object.cpp`,
`functionvalue.cpp` and `d_stage.cpp` do not compile under the Route B owner flags while
`fvb.cpp`, `fvb-data-parse.cpp`, `fvb-data.cpp`, `jstudio-control.cpp` and `JAIAnimation.cpp` do -
so the campaign's first entries are width repairs, not reconstruction. Section 8's caveat stands:
this is a campaign, and the size of it is now legible instead of assumed.
