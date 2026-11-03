# Independent review adoption — 2026-09-09

> Historical research: the Route B prototype this note describes (a native build from the decompilation) was later removed, so the `route_b` paths it names no longer exist. Current work is in [GOAL_LOOP.md](../GOAL_LOOP.md).

The implementation agent has read the complete
[independent report](INDEPENDENT_DEEP_DIVE_REPORT_2026-09-09.md) and preserves
it unchanged. This note records accepted findings, local verification, and
corrections to the proposed experiments. The initial ingestion occurred during
the user-requested pause. The user subsequently resumed implementation under
`../status/REORIENTATION_2026-09-09.md`; the goal is active.

## Corrections established during resumed execution

The report's F8 assertion is incorrect: `J3DJoint::entryIn` skips shapes with
`J3DShpFlag_Hide` (0x1), while the report reasoned from the different packet
flag `J3DShpFlag_Hidden` (0x10). Material sorting may merge shape packets under
another material packet. The initial recommendation below to preserve an
unconditional top-level material identity assertion is superseded. The current
oracle independently computes clipping and checks exact visible shape ownership
through merged lists, requiring visible BG geometry on this route.

Resumed work repaired source replay/stage ownership, original MULT placement,
native material textures, empty matrix operations, DRW1/EVP1 serialization,
weighted Link matrices and one retail-confirmed camera interpolation operand.
Actual first/final Metal frames show Link beside original Outset buildings.
Source availability does not guarantee runnable semantics: the camera expression
still present in current upstream differed from the retail instructions.
Full visual fidelity, live input, normal scene scheduling and PRD acceptance
remain open. See `../status/ROUTE_B_WORLD_INTEGRATION_2026-09-09.md` for evidence
and qualification state. The following sections preserve the ingestion snapshot.

## Locally checked findings at ingestion

- HEAD/worktree inspection matches the reported unfinished world integration.
  `route_b/CMakeLists.txt` explicitly removes
  `bluewake_route_b_player_stage_runtime_objects` from the world probe and
  adds `bluewake_route_b_initial_room_request_objects`. The runtime include
  defines the eight symbols in the latest available link failure. Why the
  object was removed remains inferred; successful corrected linking is untested.
- `bash scripts/prepare_route_b.sh --check` exits 1 with
  `Route B patched file differs from the patch series: src/d/d_stage.cpp`.
  It also prints pre-existing patch whitespace warnings. The unregistered
  WORLD_ROOM_MEMORY block is present in that file; disk patch 0210 is incomplete.
- `ref/aurora/lib/gx/fifo.cpp::publish` returns without publication when no
  frame is active or when recording a display list. Captures do not establish
  command-processor execution. Existing documentation records actual native
  **2D** opening presentation on September 2; J3D presentation remains unobserved.
- The world probe checks identity of material packets from models 0/1/3.
  This does not establish visible shapes. Preserve that ownership assertion
  and add independent clipping evidence when implementing the world milestone.

The reviewer's whole-unit compile counts, upstream changes, and process
snapshot were read but not independently rerun or fetched during ingestion.
Treat them as attributed review evidence. Timestamps establish recorded file
state, not who edited it or which user authorization applied.

## Corrections before executing the report

**E2 cannot parse the entire capture with the proposed reader.**
`ref/aurora/lib/gx/dl.cpp::Reader::next` accepts only
`GX_AURORA_DRAW_INDEXED` in its Aurora opcode branch. Native LOAD_ARRAYBASE,
texture, and other valid command-processor subcommands can therefore produce
`unsupported Aurora subcommand` without any malformed J3D output. The reader
also treats CP writes as passthrough and caches layouts derived from supplied
descriptors; it does not reproduce the command processor's changing VCD/VAT
state. Its public failure accessor is `failed()`.

Revise E2 to inspect a bounded command-processor execution seam with the real
state and dependencies, or use the reader only on supported display-list
segments with correct state at their boundaries. First discriminate unsupported
test-tool coverage from malformed game output. Do not patch a J3D writer just
because this reader rejects a valid Aurora command. If a headless processor
seam requires substantial new infrastructure, proceed to the existing visible
path when implementation and the applicable launch constraints permit it.

**Captures are not portable replay artifacts.** The stream includes native
host pointers. Byte-only dumps may support structural inspection, but a second
process cannot dereference those addresses. Prefer validation in the producing
process before resources or transformed arrays change; a separate replay tool
would require explicit resource serialization and pointer rebinding.

**Reconcile patches without resetting the dependency file.** Preserve the
complete current diff and reconstruct the registered series in an isolated
index/tree. Compare it with the working file and make a targeted edit/export.
The report's checkout-one-file-then-rerun-preparer recipe is not assumed safe:
patches span multiple files, so partial restoration can prevent whole-patch
application against other already-patched files. Check reconstruction before
altering the shared dependency checkout.

**Compilation is not composition acceptance.** Whole-unit compile results
justify trying coherent owners; undefined symbols and runtime ownership still
need classification. The report says fourteen stage-cast diagnostics at eight
sites but lists additional pointer-narrowing locations; remeasure exact sites
before sizing or implementing that portability change.

**Do not invent permission requirements from the advisory report.** A macOS
window is distinct from a Simulator instance. At implementation resume, apply
the user's actual one-process/Simulator constraints to current process state;
do not close any unrelated application or rely on historical PIDs. This
ingestion turn launches nothing because implementation remains paused. GUI
authorization should be assessed from the actual resumed request, not a blanket
rule that every window needs a separate question.

## Adopted order for the next implementation run

1. Preserve WIP, reconcile the patch series, complete 0210 using the existing
   stage-runtime owner, restore that owner to the world probe, and remove its
   competing initial-room-request composition. Check duplicate definitions and
   the full-unit include path before moving bodies. Accept only a reproducible
   patch check and a measured link result; no new runtime pass is presumed.
2. Run one focused strict world experiment; record the first actual failure,
   collision-owner identity, packet ownership, and capture size. Keep clipping
   and visible-output evidence separate. Resolve a failure at its owning
   subsystem instead of introducing another success stub or tier by habit.
3. Observe real renderer consumption using corrected E2 or existing E3.
   Keep the evidence boundary explicit: structural parsing, processor execution,
   observed pixels, and live input are different milestones.
4. Prove actual Link plus Room44 and live movement/camera/collision in one
   continuous session, then compose scheduler-driven ROOM_SCENE/PLAYER and the
   original frame/Painter owners. Replace diagnostic phase calls and cancelled
   requests as the corresponding real owners are qualified.
5. Prefer full scene/stage units and generalize private asset preparation for
   vegetation. Record concrete compile/link blockers for any retained tier.
   Evaluate upstream audio bodies as bounded backports with source verification;
   an empty deletion method cannot qualify populated audio-object teardown.
6. Continue toward normal boot, transitions, save/reload, real audio, performance,
   and complete PRD acceptance. Keep Route A frozen as an oracle; the review
   supplies no demonstrated reason to reopen its optimization route.

For each iteration name one observable, its falsification condition, the first
failure, and the next experiment on either outcome. Follow the existing
three-same-shape anti-stall rule. Tier/seam counts and production census counts
are supporting diagnostics, not acceptance metrics: neither a smaller count nor
a larger test suite substitutes for authentic behavior. Reassess unchanged
product observables before spending another iteration on the same proxy.

## Verification and remaining work at ingestion

The source-preparer failure was reproduced read-only; implementation experiments
E1/E2/E3 are pending. No gates advance. BW-P4-0126 remains the parent integration
blocker with two concrete subproblems: stage-owner/patch reconstruction and
unobserved J3D processor/presentation. These are engineering work, not proven
external blockers. The report already exists in the repository as an untracked
file and is preserved; this adoption note and ledger updates are also uncommitted.
