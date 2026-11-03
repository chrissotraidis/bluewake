# BlueWake Research Log

Chronological log of research iterations. Each entry states the current blocking
question, what was done, and what was learned. Evidence detail lives in
[EXPERIMENTS.md](EXPERIMENTS.md); architecture judgments in
[ARCHITECTURE_OPTIONS.md](ARCHITECTURE_OPTIONS.md).

---

## Iteration 1 — 2026-08-09

**Blocking question:** What is the current landscape? Do the candidate
technologies (TWW matching decomp, GameCube static recompilation, Dusk's TP
native port, Aurora SDK layer) exist in a state that makes a native Apple
Silicon macOS/iOS Wind Waker port an engineering problem rather than a research
problem?

**Actions:**
- Created research environment (`docs/`, `docs/research/`, `ref/`, gitignored
  `local-research/` for logs and any local game data).
- Cloned and pinned 8 repositories (see [DEPENDENCIES.md](DEPENDENCIES.md)).
- Confirmed local toolchain: Apple Silicon (arm64), macOS 26.5, Xcode 26.6,
  CMake 3.27.1, Ninja 1.13.2.
- Launched deep source inspections of dusk, DolRecomp/ModernGekko, aurora, and
  zeldaret/tww.

**Key early findings (from search, to be verified against primary sources):**
- **Dusk/Dusklight shipped.** TwilitRealm released Dusk v1.0.0 (2026-05-09), a
  native port of Twilight Princess built from the *completed* zeldaret/tp
  matching decomp, targeting Windows/macOS/Linux/Android/iOS. This is the
  single most important precedent: it proves the "matching decomp → native
  host" architecture end-to-end on Apple platforms for a GameCube JSystem
  Zelda game. UNVERIFIED until repo inspection completes.
- **zeldaret/tww is active but incomplete** (~60% cited in May 2026 press;
  precise per-subsystem numbers being pulled from decomp.dev).
- **DolRecomp/ModernGekko exist and are public** (ExpansionPak org): static
  recompiler (PowerPC→C) + runtime for recompiled GameCube games. Maturity
  unknown; inspection in progress.

---

## Iteration 2 — 2026-08-09

**Blocking question:** Which of the four candidate architectures can actually
reach "native, non-emulation, JIT-free-iOS Wind Waker"? Sub-questions: Is
Dusk's runtime separable and reusable? Is DolRecomp/ModernGekko a credible
JIT-free path? Where does pure static recomp stall on this exact game?

**Actions:** Full source deep-dives of dusk (+tp diff), aurora,
DolRecomp/ModernGekko(+Template), sp00nznet/ww; local builds of DolRecomp
(E5: 16/16 tests pass, arm64) and aurora (E6 below: 238/238 tests pass,
arm64); decomp.dev history pull (E4); license verification across all repos.

**Findings (evidence in DUSK.md, AURORA.md, DOLRECOMP_MODERNGEKKO.md,
EXPERIMENTS.md):**
1. **Approach B falsified as a foundation.** ModernGekko is a Dolphin fork —
   recompiled chunks execute inside Dolphin with its JIT as load-bearing
   fallback. That is whole-system emulation (excluded), GPLv3, and has zero
   iOS story. DolRecomp's REL model cannot represent runtime OSLink for a
   416-REL game. sp00nznet/ww independently shows pure recomp of this exact
   title stalling at authentic initialization (Dolphin memory-dump seeding).
2. **Approach D confirmed.** Dusk = hard fork of tp decomp + ~62k-line
   portability pass, 100% recompiled C/C++, RELs statically linked, Aurora
   GX→Dawn→Metal, DuskDsp host-float DSP HLE, live iOS CI, CC0. Aurora
   verified against tww's actual GX call sites — every EFB-copy format WW
   uses is implemented; only real gap is GXPeekARGB (Picto Box).
3. **Approach A is the end-state; its only blocker is tww completeness**
   (73.4% code / 80.0% funcs at 2026-08-08; +2.3–3.8 pts/mo; ETA ~Q2–Q3 2027).
4. **Approach C (fine-grained hybrid) rejected:** endianness/pointer-width
   memory-model incompatibility between natively-compiled decomp code and
   guest-RAM recompiled code; no tool exists; N64Recomp's patch mechanism
   solves the reverse direction.
5. WW executable topology pinned from tww config: **main.dol + 416 RELs**
   (235 loose files/rels/, 181 in RELS.arc mmem/amem pools).
6. JAudio delta discovered: TP=JAudio2 vs TWW=JAudio1+JAZelAudio — Dusk's
   audio *library* layer is not reusable, only its DSP model.

**Decision state:** end-state architecture settled (A on D). Remaining
feasibility variable is *timing/subsystem coverage* of tww. → Iteration 3:
per-subsystem completeness of tww, especially player actor, stage loading,
JAudio1, JSystem revision deltas, and REL infrastructure.

---

## Iteration 3 — 2026-08-09

**Blocking question:** Is the tww decomp complete *where it matters* — i.e.
does the critical execution path (boot → title → stage → controllable Link →
transition → save) have source today, and what exactly is missing?

**Actions:** Full tww repo inspection (configure.py object-state parsing,
per-directory stub census, REL enumeration, build-system pins, activity
stats); local toolchain experiment E8 (configure + dtk native on arm64
macOS, build blocked only on user disc); Spotlight check E9 (no local disc).

**Findings (evidence in TWW_DECOMP.md):**
- Effective source coverage ≈ 89.2% of functions; **4,250 functions have no
  source** (10.8%), concentrated in: NPC/enemy actor RELs (3,459 across 102
  of 414 RELs), menu/map/HUD cluster (~460), JAudio1/JAZelAudio (166).
- **The critical path is essentially complete TODAY:** m_Do machine layer,
  f_pc/f_op frameworks (100%), SSystem (100%), REL infrastructure (100%),
  JKernel/JUtility/J2DGraph (100%), d_stage/d_save/d_com/d_event,
  d_s_logo/d_s_title/d_s_play scenes, and the entire player actor
  (34,554 lines, zero stubs). 312/414 actor RELs stub-free.
- The 15 NonMatching SDK units are all GX/GF — replaced by Aurora anyway.
- No self-modifying code; REL loading fully understood and replaceable by
  static linking (dusk precedent).
- Gate 0 proven on this machine for every leg (E5/E6/E8).

**Decision:** Research question is now answerable. Verdict = **BOUNDED
PROTOTYPE REQUIRED, leaning PROCEED** (architecture fully de-risked by
Dusk; critical-path source exists; what remains unproven is that the
TWW-specific spine survives the portability pass and reaches authentic
title/gameplay with gaps stubbed — an experiment, not research). Producing
FEASIBILITY_REPORT.md, IMPLEMENTATION_PLAN.md, HANDOFF.md.

---

## Falsification review (standing, per anti-self-deception rules)

Checked and dispositioned:
- *Hidden interpreter fallback in candidate runtime* → found (ModernGekko =
  Dolphin JIT fallback) → that architecture rejected; chosen architecture
  has zero guest-code execution.
- *Missing REL support* → real in recomp path; irrelevant in decomp path
  (static link, loader fully decompiled).
- *Self-modifying code* → none observed; byte-perfect static rebuild.
- *Unsupported GX/DSP behavior* → GX verified call-site-by-call-site vs
  Aurora (one gap: GXPeekARGB/Picto Box, bounded). DSP: DuskDsp model
  proven for TP; JAudio1 library layer must come from tww (166 stubs
  remaining upstream) — flagged as the top schedule risk, not a blocker.
- *x86-only assumptions* → none in chosen stack (all components built and
  tested on arm64 in E5/E6/E8).
- *Dynamic code loading on iOS* → none required (all-static build proven by
  Dusklight's ios-default preset).
- *License problems* → chosen stack is CC0+MIT; GPLv3 stack rejected.
- *Aggregate-percentage deception* → addressed by per-subsystem stub census;
  gaps are real but concentrated in non-critical-path code.
- *Economic rationality* → residual risk noted: TwilitRealm is rumored to
  target WW next (UNVERIFIED); BlueWake may duplicate a better-resourced
  team's work. This is a strategic, not technical, consideration.

---

## Iteration 4 (closing) — 2026-08-09

**Event:** Research findings reviewed with the project sponsor. Sponsor
constraint established: the complete game must be delivered entirely
in-house within weeks. Per the evidence (4,250 sourceless functions;
reverse-engineering timescales; recomp shortcut disqualified), no
investigated architecture can meet that constraint at any team size.

**Decision recorded:** **WAIT** (Exit Condition B), for schedule reasons —
the technical verdict (bounded-prototype-leaning-PROCEED on a ~1-year
horizon) is unchanged and preserved in FEASIBILITY_REPORT.md.
`WAITING_FOR.md` created with seven concrete promotion triggers (upstream
match thresholds, audio/menu stub closure, TwilitRealm announcements,
aurora GXPeekARGB, constraint relaxation) and a monthly check cadence.
Research loop closed; repository published as the durable record.

---

## Iteration 5 — external second opinion adopted 2026-08-13

**Event:** A second engineering assessment revisited the static-recompilation
route using newer DolRecomp/ModernGekko/RecompCore and SunPad evidence.

**Correction:** The 2026-08-09 audit's categorical rejection was too broad.
A no-runtime-PowerPC-JIT Apple-mobile mode has been demonstrated in SunPad,
DolRecomp has batch REL support, and RecompCore has runtime REL address
remapping. BlueWake still lacks composite DOL+REL packaging, scalable active
REL discovery, and any Wind-Waker-specific runtime proof.

**Evidence boundary:** The reassessment had no Wind Waker disc and built or
ran no Wind Waker module. Its conclusions are a credible hypothesis and
prototype design, not gameplay or performance evidence.

**Decision:** Replace blanket WAIT with **PROCEED to a bounded, Mac-first
static-recompilation prototype**. Retain the decomp/Aurora route as the
cleaner long-term track. Keep full-product and iPad commitment behind the
gates in `../archive/FEASIBILITY_REASSESSMENT_2026-08-13.md`.

---
