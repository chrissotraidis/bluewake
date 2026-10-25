# Route B Vertical Boot Gate

**Date:** 2026-09-01

**Blocker:** `BW-P4-0076`

**Scope:** native application root to authentic retail logo-scene create

## Question

Do the passing native process/framework slices compose into a bounded vertical
boot path, or has horizontal compile coverage hidden a broad whole-engine port?

The first authentic path is rooted in `f_ap_game.cpp`. Its unmodified
`fapGm_Create` initializes the process managers and requests
`fpcNm_LOGO_SCENE_e`. The corresponding retail profile is
`g_profile_LOGO_SCENE` in `d_s_logo.cpp`; its create method enters the real
three-phase logo initialization. Resolving and entering the method is compile
evidence only. Authentic completion of `phase_0` is the first accepted vertical
target.

## First Compile Frontier

`scripts/route_b_vertical_boot_probe.py` applies the same Apple Clang C++20,
exact-width prelude, and strict pointer-narrowing policy as the passing Route B
foundation. It also emits the complete preprocessing dependency census even
when semantic compilation fails.

| Target | Unique dependencies | First strict failures |
|---|---:|---|
| `f_ap_game.cpp` | 45 current (63 initial) | **Passes** after width-safe HostIO payloads and narrow application ownership |
| `d_s_logo.cpp` | 287 | GX display-list width, serialized J3D texture offsets, MSL/libc++ conflicts, JStudio/J3D pointer payloads, native heap pointer arithmetic |

The logo source's dependency owners include 53 Dolphin SDK headers, 44 game-
state headers, 32 SSystem headers, 14 JStudio headers, 13 J3D base headers, 11
J3D animator headers, 13 JKernel headers, plus J2D, JAudio, JParticle, machine,
and process owners. These are preprocessing dependencies, not a
claim that every owner must be linked, but they disprove the idea that the
first retail scene is one more narrow `f_op` wrapper.

## Application-Root Result

The first half of the vertical path now passes. The original TWW
`fapGm_Create`, `fapGm_Execute`, HostIO registry, and counter compile and link
under the strict native ABI gate. HostIO pointer payloads retain native width,
the fixed-address VI register declaration no longer creates storage in every
native object, and `fpcPf_Get` resolves profiles through Route B's static REL
registry instead of indexing the absent retail profile array.

An executable smoke runs the original application root. It initializes the
real process, scene, overlap, camera, and draw managers, registers the original
HostIO object, issues the authentic `fpcNm_LOGO_SCENE_e` request, and advances
native frames until that request invokes a profile create method obtained from
the static registry. The registered method is deliberately synthetic test
scaffolding; this proves request/profile plumbing only and is not retail logo
or gameplay evidence.

The vertical probe now reports 45 dependencies and zero strict diagnostics for
the application root. The retail logo source remains unchanged at 287
dependencies and remains the active compile frontier.

## Decision Boundary

Do not patch the diagnostics in discovery order. The next tier must partition
the logo implementation by real ownership and establish its unresolved-symbol
closure. A bounded result must retain the original retail profile and method
behavior while placing platform resource, graphics, controller, card, reset,
and audio operations behind explicit services. Success-returning stubs,
fabricated phase completion, and Route A guest-memory calls are prohibited.

If that partition still requires broad simultaneous admission of J3D, JStudio,
game state, and JAudio before one authentic create phase can execute, the gate
fails near-term feasibility. Route B must then be recorded as a long-horizon
reconstruction program and whole-route selection reopened; the loop may not
return to unrelated generic framework leaves.

## Phase-Zero Ownership

The first source audit shows that `phase_0` is smaller than the complete logo
translation unit but is not a synthetic process callback. It owns:

- video/reset-mode observation;
- asynchronous dynamic-link completion;
- initialized JAudio1 state and first-wave readiness;
- real `System` and `Logo` archive mount requests; and
- archive-heap diagnostics.

`phase_1` then synchronizes those resources, installs the two toon textures,
and allocates game/archive heap storage. `phase_2` creates J2D logo objects,
initializes particles, mounts the shared Link/message/stage resource corpus,
loads static waves, configures frame timing/fade, and installs reset input.

The next partition may remove the broad `dolzel` PCH and define typed platform
services, but those services must perform the real operations. A callback that
always reports dynamic link, audio, or resource mounting complete would only
manufacture phase progress and is prohibited.

## Reproduction

```sh
python3 scripts/route_b_vertical_boot_probe.py \
  --output /tmp/bluewake-route-b-vertical-boot.json
```

Exit `2` means the measured vertical sources still require native port work;
the JSON report remains the evidence artifact. Exit `0` means both sources
pass the strict compile frontier and link closure can be measured next.

Qualified report:
`local-research/evidence/route-b-vertical-boot-v1-20260901/report.json`,
SHA-256
`9f6411e7eb1a6d79fe6dcb141ed4ca74ec5be394ddacea9a0b0116a41e093f25`.

## Phase-Zero Partition Result

The profile boundary is no longer synthetic. Route B now registers the real
GZLE01 `g_profile_LOGO_SCENE`, invokes the original phase-handler algorithm,
and runs a narrow native implementation of the retail phase-zero control
flow. It waits for static dynamic-link registration and first-wave readiness,
requests `System` with the default mount direction followed by `Logo` from the
tail, emits the archive-heap diagnostic, and waits at resource synchronization.
Phase one and unsupported lifecycle methods fail closed.

The focused executable deliberately injects staged test services. It proves
profile shape, control flow, wait behavior, mount order, and the exact phase
boundary; it does not prove that resources were read, archives were mounted,
audio initialized, video mode changed, or heap state created. Consequently
this result does not complete authentic phase zero and does not increase the
25-30% product estimate.

The remaining phase-zero frontier is now concrete:

1. native `System`/`Logo` archive mount and synchronization over imported data;
2. explicit archive-heap and worker-thread ownership;
3. real host VI/reset/progressive observations and renderer-mode application;
4. real JAudio initialization and first-wave readiness.

Resource mounting is first because it provides the first asset-bearing boot
observable and forces the heap/thread contract without requiring phase-one
J2D/J3D admission. A success-returning mount callback remains prohibited.

## Native Archive Owner

The first resource tier is bounded. `ObjectResources` preserves the retail
64-object capacity, request/reference state, mount direction, and incremental
synchronization while owning archive bytes in native memory. Its parser
supports raw RARC and bounded outer Yaz0 expansion, validates every metadata
table and payload range, and makes read/format failure sticky. The real logo
profile now loads public synthetic `System` and compressed `Logo` archives
through this service before stopping at phase one.

This removes the success-returning mount/sync test double, but the reader still
serves a public fixture. The next authenticity tier is an Aurora DVD-backed
reader over the user's private GZLE01 image. Evidence may record only safe
metadata and hashes. JKR/J3D resource object construction is phase-one work
and is deliberately not smuggled into this validation tier.

That authenticity tier now passes. A DVD-only target links pinned Aurora
nod/DVD/FST without GX, Dawn, J3D, or full Aurora core, verifies GZLE01, and
accepts both private archives. `System.arc` is 34,176 expanded bytes/7 entries;
`Logo.arc` is 170,304 bytes/12 entries. One valid root `..` entry exposed the
standard `0xFFFFFFFF` directory sentinel, now covered by a public regression.
No private bytes were persisted. Archive sourcing itself is no longer the
blocker.

The successor donor census found no Aurora implementation behind the consumed
VI/reset/progressive declarations, and Dusklight implements them as stubs.
Route B therefore needs a BlueWake-owned Apple-host policy for cold launch or
restart, progressive-capable presentation and persisted preference, and held-B
input. This must be a default subsystem, not callbacks that return constants
only for the smoke.
