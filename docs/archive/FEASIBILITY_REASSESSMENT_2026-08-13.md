# BlueWake Feasibility Reassessment — Second Opinion

**Prepared:** 2026-08-13
**Status:** Adopted as a second engineering opinion and the basis of the
current bounded-prototype decision.
**Scope:** Original GameCube *The Legend of Zelda: The Wind Waker*, initially
`GZLE01`, targeting Apple Silicon macOS before iOS/iPadOS.

This reassessment revisits the repository's 2026-08-09 conclusion using newer
DolRecomp, ModernGekko/RecompCore, SunPad, Dusklight, Aurora, and
`zeldaret/tww` evidence. It does not erase the original audit; it narrows
several categorical conclusions and defines an experiment that can test them.

## Evidence boundary

The author of this second opinion had no Wind Waker disc image. No Wind Waker
module was generated, linked, booted, or profiled. Claims about BlueWake
gameplay, complete REL coverage, interpreter burden, module size, and iPad
performance remain hypotheses until the prototype measures them.

Repository language therefore distinguishes:

- **Verified precedent:** capabilities demonstrated by inspected upstream
  projects or SunPad.
- **Engineering inference:** how those capabilities may transfer to Wind
  Waker.
- **Prototype evidence:** results produced locally from a user-owned disc.

Only the last category can pass BlueWake's product gates.

## Revised decision

> **PROCEED with a bounded, Mac-first SunPad-style static-recompilation
> prototype. WAIT on committing to a complete iPad product until authentic
> gameplay, dynamic REL behavior, no-JIT execution, and measured full-speed
> performance pass explicit gates. Retain the decompilation/Aurora route as
> the cleaner long-term alternative.**

BlueWake is not known to be impossible. It is technically plausible and worth
a controlled prototype, but it is materially riskier than SunPad. Complete
playability and original speed cannot be promised before live execution
exposes Wind-Waker-specific failures.

## What this changes

### A no-JIT Apple-mobile execution mode exists

The original report treated ModernGekko's JIT fallback as an unavoidable iOS
disqualifier. The reassessment points to SunPad's newer execution contract:

- DolRecomp translates game-code regions ahead of time into an arm64 module.
- ModernGekko/RecompCore supplies the Dolphin-derived GameCube compatibility
  runtime.
- The runtime PowerPC JIT is disabled on Apple mobile targets.
- Uncovered or retired regions use interpreter fallback.
- A portable software vertex loader replaces Dolphin's runtime-generating
  ARM64 vertex loader.

The precise description is **AOT-recompiled game-code regions running through
a Dolphin-derived compatibility runtime, with interpreter fallback and no
runtime PowerPC JIT on Apple mobile targets**. This is not an entirely
emulation-free architecture. For BlueWake, the open question is whether Wind
Waker's interpreter fallback is correct and cold enough to sustain original
speed.

### REL support is incomplete integration, not known impossibility

The reassessment reports that current DolRecomp can process REL files and
folders, assign stable build-time virtual addresses, apply REL
self-relocations, and resolve imports among modules compiled in one batch.
RecompCore contains REL metadata plus runtime-to-AOT address remapping.

The missing BlueWake seam is still substantial:

- `moderngekko-port build` handles `sys/main.dol`, not Wind Waker's complete
  REL collection.
- Generated DOL and REL trees need namespacing and composite packaging.
- A combined dispatcher and descriptor must cover the DOL and every REL.
- Original REL metadata must stay consistent with generated code.
- Load, unload, reload, address reuse, constructors, destructors, and indirect
  calls need runtime validation.

The earlier audit and reassessment disagree on the exact REL count (416 versus
415). The private disc-backed extraction manifest is authoritative; the
prototype expects 415 per the reassessment and must fail explicitly if the
inventory differs.

### Active-REL discovery needs a scaling test

RecompCore can reportedly discover active RELs by scanning guest RAM for
headers matching compiled descriptors. Repeating that operation across
hundreds of modules may be prohibitively expensive. The prototype should
measure it once, then prefer the smallest bounded registration seam:

1. observe `OSLink`/`OSUnlink` and register/unregister loaded sections;
2. read the game's maintained active-module list; or
3. expose an API receiving module ID, section index, runtime address, and
   size.

Generic address translation should remain intact; only active mapping
discovery should become game-specific.

### GPL is a compliance issue, not a signing primitive

GPL-family dependencies create source-distribution obligations and may
complicate Apple distribution. They do not themselves prevent code signing.
User-signed and sideloaded builds are technically possible; any public
distribution needs a separate license-compliance and legal review. This is an
engineering assessment, not legal advice.

## What remains valid from the original audit

- Wind Waker is materially harder than Sunshine because of its dynamic REL
  topology.
- Translating `main.dol` is not proof of authentic initialization.
- `sp00nznet/ww` is useful prior art but not normal boot-to-gameplay proof.
- `zeldaret/tww` plus Aurora/Dusklight patterns remains the cleaner,
  maintainable long-term architecture.
- The matching decompilation is incomplete; missing behavior cannot be
  promised on a weeks-long schedule.
- JAudio1, timing, dynamic actors, scene transitions, saves, long sessions,
  and mobile performance remain major risks.
- Compilation, installation, a live PID, or one rendered frame is not
  gameplay acceptance.

## Two-track strategy

### Track A — static-recompilation prototype

Use a private, user-owned `GZLE01` image to generate `main.dol` plus the full
REL collection with DolRecomp, package a composite arm64 module, and run it
through ModernGekko/RecompCore. Prove the iOS CPU contract first on Apple
Silicon macOS with runtime PowerPC JIT disabled.

This track can answer the largest questions now: translation coverage,
fallback burden, REL lifecycle correctness, authentic initialization, module
size, and performance.

### Track B — decompilation plus Aurora/Dusklight patterns

Continue monitoring and preserving the `zeldaret/tww` source-native route.
It remains the better long-term Apple architecture, but incomplete source
prevents a guaranteed complete port today. Symbols, traces, and regression
cases produced by Track A can inform this route later.

## Bounded prototype gates

### P0 — private input and manifest

- Initially support only `GZLE01` USA revision 0.
- Verify the user-owned disc and `main.dol` against pinned identifiers.
- Extract to an ignored private workspace.
- Inventory loose RELs and `RELS.arc` contents with filename, module ID,
  location, size, hash, sections, imports, and relocations.
- Commit no Nintendo data or game-derived generated module.

### P1 — complete translation audit

- Build current DolRecomp on Apple Silicon.
- Translate `main.dol` and all RELs, batching RELs for cross-module imports.
- Record instructions, unknown operations, fallback sites and reasons, SMC
  candidates, unsupported relocations, and generated/object sizes per image.
- Fail on missing modules, duplicate module IDs, unresolved imports, or
  unexplained relocation failures.

Stop if unpatchable hot-path fallback or unsupported dynamic behavior makes
no-JIT execution clearly uneconomic.

### P2 — composite module

- Namespace generated symbols across all images.
- Generate one dispatcher covering DOL and build-time REL ranges.
- Emit REL module and executable-section metadata plus DOL/REL chunk and SMC
  tables.
- Link one arm64 module or attached descriptor.
- Test complete, non-overlapping address coverage and representative DOL/REL
  dispatch.

### P3 — active-REL lifecycle

- Test link, unlink, reload, guest-address reuse, multiple executable
  sections, constructors/destructors, unresolved handlers, and indirect calls.
- Ensure mapping work is proportional to link events or active modules, not
  every compiled descriptor multiplied by all guest RAM.

### P4 — authentic no-JIT macOS gameplay

- Enforce the iOS-equivalent no-fallback-JIT configuration on Apple Silicon.
- Boot normally from the user's disc/extraction without captured state,
  memory seeding, scene bypasses, or synthetic stages.
- Record native dispatch, interpreter fallback, REL mappings, SMC retirements,
  exceptions, missing services, speed, and frame time.
- Progress through original boot/logo, title, file select, new-game intro,
  Outset Island, controllable Link, one real transition, and save/quit/reload.

**P4 is the first proceed-to-product gate.** UI and mobile-shell work should
not precede it.

### P5 — representative compatibility

Exercise Outset and interiors, sailing, Forsaken Fortress, a dungeon, a boss,
inventory/map/message UI, saves across a story flag, cutscenes, and field,
interior, battle, and cutscene audio. Remaining failures must be attributable
and must not imply a missing architectural subsystem.

### P6 — physical iPad proof

Only after P4, reuse reviewed SunPad Apple-host pieces selectively in a
BlueWake target. Prove normal boot to the same authenticated save, controllable
gameplay, a transition, lifecycle handling, and save-hash preservation through
backgrounding and restart.

### P7 — original-speed sustained performance

Initially target Wind Waker's normal 30 FPS behavior, not a 60 FPS patch.
Measure rendered FPS separately from emulated speed, frame-time distribution,
AOT/interpreter counts, CPU subsystem costs, memory/jetsam margin, thermals,
energy, and long-session stability across field, sailing, dungeon, boss, and
cutscene workloads.

### P8 — complete-game acceptance

Earn any “fully playable” claim with a clean new-game-to-ending run, every
dungeon and boss, required cutscenes, representative optional content,
death/retry, saves, suspend/resume, controller reconnect, long sessions, and
regression replay of every prior runtime failure.

## Stop criteria

Pause or stop the static track when measured evidence shows any of the
following:

- hot execution remains interpreter-dominated after reasonable fixes;
- REL mapping needs pervasive runtime rewrites rather than a bounded seam;
- authentic initialization depends on unmodeled behavior with no tractable
  replacement;
- module size, link time, memory, or signing constraints are impractical;
- representative content reveals systemic corruption; or
- physical iPad performance has no credible measured headroom.

A failed gate is a successful research result when its evidence is captured.

## Claims discipline

Until the corresponding gates pass, do not claim that Wind Waker runs on
BlueWake, Link is controllable, all RELs execute natively, interpreter use is
negligible, the game runs at full speed, or the iPad build is fully playable.

Acceptable current wording:

> BlueWake is a feasibility and prototype project for running the original
> GameCube Wind Waker through ahead-of-time recompiled arm64 game code and a
> GameCube compatibility runtime on Apple platforms. The architecture is
> plausible, but Wind Waker gameplay, complete REL coverage, original-speed
> iPad performance, and full-game compatibility have not been demonstrated.

## Dated upstream snapshot reported by the second opinion

These values were reported by the reassessment, not reproduced in this
repository. Refresh them before relying on exact revisions or percentages.

- `zeldaret/tww` `93a2a6522a0e8a05093194452aa99d176bbfcb32`
  (2026-08-12): 73.422485% matched code; 31,459/39,324 matched functions;
  DOL 12,666/13,763; RELs 18,793/25,561.
- DolRecomp `fa0cf619e8d7eb8cba7eaf55267a12caaebb46aa`
  (2026-08-10): reported arm64 build and 16 passing tests.
- ModernGekko `884c20505d7179160f8bb01d9db0f723c53b09cb`
  (2026-08-10).
- Previously inspected RecompCore revision with REL mapping:
  `e13ab348f13cd67879f6db6e9d7185410f8f62c6`.

Primary upstream references:

- <https://github.com/zeldaret/tww>
- <https://decomp.dev/zeldaret/tww.json>
- <https://github.com/ExpansionPak/DolRecomp>
- <https://github.com/ExpansionPak/ModernGekko>
- <https://github.com/ExpansionPak/RecompCore>
- <https://github.com/TwilitRealm/dusklight>
- <https://github.com/encounter/aurora>
