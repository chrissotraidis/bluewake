---
name: plan-engineering-work
description: Create or update a concise, repository-grounded PRD.md, TECH_PLAN.md, and PROGRESS.md, with BlueWake as the default project. Use for a planning trio, feature brief, or test/performance/refactor plan needing motivation discovery, ambiguity clarification, architecture links, measurable acceptance, and implementation workstreams that can run in parallel. Plan the work without implementing the product.
---

# Engineering Plan

Produce three real, synchronized Markdown artifacts. Default to practical edits, short tables, code snippets and compact diagrams. Read [references/artifact-contracts.md](references/artifact-contracts.md) before drafting; adapt its templates instead of copying empty sections.

## 1. Ground the work

- Infer PRD → TECH_PLAN → PROGRESS when the user asks for the planning trio. Reuse conversation context; do not restart discovery.
- Use BlueWake as the default project. On every invocation, consult the current [repository](https://github.com/chrissotraidis/bluewake), [docs](https://github.com/chrissotraidis/bluewake/tree/main/docs), and [issues](https://github.com/chrissotraidis/bluewake/issues), unless the user explicitly changes the project/context. Read the relevant source, architecture/performance guides, issue bodies and recent replies; inspect overlapping PRs. Use GitHub tools when available. Record the inspected revision and issue references; URLs or search snippets alone do not count as context review. Do not exhaustively read unrelated files/issues. If access fails, report the gap rather than implying a successful check.
- Start by asking for the motivation: what problem prompted this work, who experiences it, and what outcome would make it worthwhile? If the user already stated it, ask for concise confirmation rather than repeating discovery; never infer motivation solely from the requested implementation.
- Ask targeted questions whenever the goal, scope, constraints, baseline, acceptance or intended behavior is ambiguous. Do not resolve ambiguity by silently choosing an interpretation. Bundle motivation and the highest-impact uncertainties into 1–3 concise questions; reuse explicit answers. Continue independent source inspection while waiting, but keep dependent decisions open and do not finalize affected artifacts until answered. Routine formatting/mechanical choices do not need questions.
- Read the supplied PRD/artifact in full. Resolve its current identity/version through its provider; do not substitute an old chat summary.
- Read applicable AGENTS.md and repository instructions. Inspect the relevant implementation, tests, build/CI entry points and architecture documents before recommending changes. Use rg first locally; use connected repository tools for remote code.
- Record the repository revision, relevant file paths and exact source/document versions. Distinguish historical evidence from the current baseline.
- Check overlapping work when repository instructions require it. Respect runtime ownership and existing release constraints; do not invent approvals.
- If code or fixtures are unavailable, state the gap and limit affected decisions. Do not invent symbols, APIs, tests, profiling results or architecture documentation.
- Record confirmed motivation and clarification answers in PRD. Label unresolved questions as open rather than treating guesses as requirements. Do not force model-tier choices, framework installation or unrelated workflow setup.

## 2. Choose real destinations

- Default new artifacts to `docs/planning/YYYY-MM-DD-feature_name/`, relative to the repository root. Use the user's local creation date and a concise lowercase snake_case feature name, for example `docs/planning/2026-10-19-gamethread_parallelization/`. Honor an explicit destination; when updating an existing plan, preserve its directory unless the user requests a move.
- Create/update `PRD.md`, `TECH_PLAN.md`, `PROGRESS.md`. Preserve existing IDs, completion states and evidence; never reset a tracker while revising prose.
- For repository work, write within its checkout/worktree. Commit/push/open PRs only when authorized. Do not also copy git-backed work to Library.
- Without a writable checkout, ask for a writable export destination if none was supplied, then save the three Markdown artifacts there. Use Library when that integration is available and applicable. If no persistence route exists, explain the limitation and request a destination; do not claim the artifacts were saved or that the remote repository changed.
- Never claim a remote repository changed because files were generated locally. Deliver the files or concrete patch; identify the destination truthfully.

## 3. Separate the artifacts

| Artifact | Owns |
|---|---|
| PRD | Problem, scope, measurable outcomes, acceptance, constraints |
| TECH_PLAN | Inspected facts, file/symbol changes, contracts, examples, flow, tests, implementation order |
| PROGRESS | Stable milestone/task IDs, matching dependencies, actual state, measured results/evidence |

- Label code/APIs/files **existing**, **modify**, or **proposed**. Proposed snippets are implementation examples, not executed changes.
- Make snippets match actual surrounding code and conventions; use small before/after or patch examples. Avoid boilerplate skeletons unrelated to the change.
- For code-change plans, include at least one short, source-grounded code/diff snippet showing the central edit or contract. If source is unavailable, record that gap instead of inventing code. Do not expand a plan into a full implementation.
- Use Mermaid for useful branching, ownership or event order; keep each diagram focused. Explain distinctions outside it. Do not put fragile hyperlinks inside diagrams.
- Link milestone → implementation surface → task → acceptance/evidence.

## 4. Link architecture where it matters

- Discover the actual architecture directory/index; follow relevant entities and flows into code.
- Verify linked files, headings and source symbols by reading them. Use repository-relative links when artifacts will live in the repository; use revision-pinned URLs when portability or review provenance requires them.
- Place links beside affected FRs, file-change rows, flow explanations and milestone groups, not only in a bibliography. Prefer 1–2 useful links per location over repetitive link lists.
- Distinguish documented behavior from code findings and proposed behavior. Do not cite an entity diagram as evidence of thread safety or implementation detail it does not document.
- If architecture docs are missing/stale, identify the gap, cite real source instead, and add a documentation task only when needed. Do not fabricate anchors.
- Keep checked URLs/revisions, commands, fixture references, binary/log/report identities concrete. Never expose secrets, private fixture contents or signed download URLs.

## 5. Make acceptance falsifiable

- Reuse existing tests and runners before adding another framework.
- Separate source-only tests, fixture-dependent integration, hardware checks and performance measurements.
- Require nonempty/complete evidence and actual execution-path activation where relevant. Empty logs, serial fallback, missing scenes or absent metrics cannot silently pass.
- Define pass/fail/skipped/inconclusive; required unavailable evidence leaves qualification incomplete.
- State reference/baseline, reproducibility, units, workload, warmup, sample counts and comparison policy. Treat faster CPU utilization or a successful compile as insufficient evidence of product benefit.
- Preserve exactness where promised; identify intentionally changed semantics explicitly. Keep reference correctness separate from performance and manual observations.
- Record data isolation, cleanup, rollback/fallback, lifecycle and failure cases appropriate to the change. No extra safety bureaucracy unrelated to the feature.

## 6. Keep it lean and executable

- Target PRD ≤800 words, TECH_PLAN ≤1,200, PROGRESS ≤500 for moderate scope. These are prose budgets, not acceptance limits; exceed only for necessary contracts/snippets and state why.
- Remove repeated rationale, role-play personas, generic benefits, exhaustive setup questions and irrelevant UI/business sections.
- Prefer a file/change/FR table, short test matrix, proposed commands, small snippets and 0–2 useful diagrams. Do not add visuals merely to hit a quota.
- When shortening an existing draft, aim for ≥50% prose reduction while preserving contracts, stable IDs and evidence.
- Use 3–5 concrete tasks per milestone, soft ceiling 6. Declare real file/API/data dependencies; do not invent independence when milestones edit shared surfaces.
- Prefer implementation workstreams that can run in parallel whenever dependencies allow. Build a dependency graph rather than a default linear milestone chain. For each concurrent stream, name its deliverable, file ownership, prerequisites, interface contract and integration/verification gate; identify the critical path and concurrency limits in TECH_PLAN and mirror them in PROGRESS.
- Freeze shared interfaces before splitting dependent work. Run independent code, fixtures, platform integration or documentation streams together only when their actual dependencies permit it. Serialize conflicting edits or explain their coordination. If the plan must be serial, state the concrete reason. Planning implementation concurrency does not itself authorize spawning agents or changing runtime threading.
- Keep discovered fixes in the discovering milestone. Reconcile genuinely new scope before restructuring; keep completed history intact.

## 7. Reconcile, validate, deliver

1. Reconcile contradictions/clarifications into PRD; move implementation detail into TECH_PLAN without deleting acceptance.
2. Match TECH_PLAN implementation-order dependencies to PROGRESS headings. Keep milestones and tasks traceable.
   Confirm motivation/ambiguities are resolved and independent work is grouped into parallel streams with explicit integration gates.
3. Run `python3 <skill-dir>/scripts/validate_plan.py <planning-dir>`.
4. Inspect relative links/anchors, snippets and cited source manually. The validator cannot verify remote sources, semantic truth or real execution.
5. Check every marked completion has evidence; distinguish plan validation from software/hardware validation.
6. Persist all artifacts through their selected route.
7. Return a short result with artifact links and the first actionable milestone. Do not implement the plan unless separately requested.
