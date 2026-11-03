# PROGRESS — Parallel guest execution

Status: In progress (audit done, Track A chosen; `entry()` narrows the async window to opaque, non-envmap materials; pose job ≈ 1–3 % after #541's Deck counts; next: audio lead)
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M0: Baseline and audit
- [x] M0-T01: Reentrancy and guest-state audit (fixed CPU/MEM1 ABI, globals, write journal, worker pool)
- [x] M0-T02: Track decision: A first, B second choice with a reopen criterion
- [x] M0-T03: Guest-PC profile of Outset with subsystem attribution (contended run)
- [ ] M0-T04: Repeat on an idle machine; all five tour stops; real FPS
- [x] M0-T05: Pass sampler variables through `bench_tour.py`; commit the attribution script (`scripts/pc_sample_attribute.py`)

### M1: Decline census (depends: M0)
- [x] M1-T00: Decline reasons from source (see TECH_PLAN, *Decline reasons*)
- [ ] M1-T01: Reason codes for `key-s`, `transform-simple` declines (skinning: done in #541)
- [ ] M1-T02: Rank reasons by translated time they leave behind

### M2: Native widening (depends: M1)
- [x] M2-T00: Found existing natives for multi-key transforms (`native_anim.c`), mtx-calc (`native_mtxcalc.c`) and audio channels (`native_jas.c`); blockers: behaviour-timing leaders, `_ato` accesses, hashes (TECH_PLAN)
- [ ] M2-T01: Port `native_gx_gen.py`/`native_leaf_gen.py` to the behaviour-timing block shape and `_ato` accesses; generator runs on the current composite
- [ ] M2-T01b: Re-run the sets' differential tests, record hashes; Deck A/B
- [ ] M2-T02: Deck A/B, checkpoints and pictures identical

### M3: Overlap window (parallel with M1)
- [x] M3-T01: Check `entry()`/`entryIn` for matrix reads: reads values only for Z-sorted buffers and env/proj-map texture matrices
- [ ] M3-T02: Read trace of an eligible model's matrices between update and flush in Outset
- [ ] M3-T03: Count eligible models per frame and joints per model; go/no-go for M4

### M4: Async pose job harness (depends: M2, M3 go)
- [ ] M4-T01: Off-gameplay harness: concurrent jobs equal serial results; break-even size

### M5: Opt-in gameplay proof of concept (depends: M4)
- [ ] M5-T01: Submit in `mDoExt_modelUpdateDL`, join at flush, behind a flag
- [ ] M5-T02: Deck A/B; keep or reject

### M6: Audio lead
- [x] M6-T00: Source and sampler survey (PRD, *Audio lead*)
- [x] M6-T01a: Sampler counts per guest thread (`PATH.threads`); standalone check of the sampler block
- [ ] M6-T01: Idle-machine run: audio thread share on Outset and the other tour stops
- [ ] M6-T03: Any async audio path: opt-in behind `BLUEWAKE_ASYNC_AUDIO=1`, off by default; serial audio is the default and the oracle
- [ ] M6-T02: If ≥ 8 %: port the `JASChannel` natives from `--native-entries`; measure the game thread's first audio-state access after each wake

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
| --- | --- | --- | --- | --- |
| M0-T01 | `main` `ee991d3`, RecompCore `0fe472a`, generated source of `m437-a` | Source reading | — | Translated code bound to one global CPU and RAM; see PRD |
| M0-T03 | `m437-a-14c558d62d1c` | `bench_tour.py --mode headless`, `BLUEWAKE_PC_SAMPLE` 2700–4200, tww `symbols.txt` | Desktop, **concurrent build running** | J3D 21.9 %, collision 14.1 %, audio 9.3 %; FPS figures invalid |
| M0-T03 | same | `[native-*]` exit counters | same | `key-s` 77 % declined, `transform-simple` 76 %, skinning 9 % |

## Decisions

| Date | Decision |
| --- | --- |
| 2026-11-03 | Track A (native async jobs) first; Track B kept as second choice, reopened only by the PRD criterion |
| 2026-11-03 | Async audio, if built, is opt-in (`BLUEWAKE_ASYNC_AUDIO=1`), off by default; serial audio stays the default and the oracle |
| 2026-11-03 | Host DSP excluded (HLE by default, ≈ 1 %). Audio lead is the guest `JASystem` thread; gated on a per-thread measurement |
| 2026-11-03 | Skinning dropped from this plan: #541 measured it (≈ 1.5 % of the Deck game thread, threading ≈ +0.5–1 %). Estimates lowered to Deck cycle counts: serial widening ≈ 1–2 %, async pose job ≈ 1–3 %. Next lead: audio |
| 2026-11-03 | Native widening comes before any async job. An async pose job is all-or-nothing per model: one declining joint keeps the whole model serial, so M4 is gated on M2 covering multi-key transforms and large-model skinning. |
| 2026-11-03 | Async pose job eligibility adds: every material in a material-sorted or non-sorted buffer, no env/proj-map texture matrix |
