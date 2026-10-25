# Independent BlueWake deep-dive prompt

> Historical research: the Route B prototype this prompt describes (a native build from the decompilation) was later removed, so the `route_b` paths it names no longer exist. Current work is in [GOAL_LOOP.md](../GOAL_LOOP.md).

You are an independent senior engineer reviewing BlueWake. Investigate the repository deeply and return an evidence-backed technical report that another implementation agent can save in the repository and use to redirect its goal-based implementation loop. Challenge the existing approach where the evidence warrants it. We need actionable diagnosis and a credible path to a playable native game, not another recap of passing probes.

## Assignment and authority

Repository: `the repository root` (or your checkout of `chrissotraidis/bluewake`). Snapshot inspected September 9, 2026: HEAD `6949c27`, with unfinished local changes. Recheck HEAD and the entire working tree; a remote-only checkout may lack essential WIP, dependencies, logs, and private inputs.

This assignment authorizes investigation and a report. The previous implementation effort was stopped at the user's request. Do not resume its implementation loop, commit, push, alter existing implementation, close applications, or launch a GUI as part of this review. Read-only inspection and bounded headless diagnostics are appropriate; preserve existing build evidence and use isolated outputs if rebuilding. If required inputs are unavailable, finish the source-based investigation and identify exactly what evidence the local agent must collect. Do not pretend an unavailable runtime was tested.

The goal is an authentic Wind Waker application using the user's GZLE01 data: macOS first, normal boot, live input, correct rendering/audio, transitions, save/reload, original-speed behavior, and ultimately complete-game acceptance. iPhone/iPad remain later requirements. Do not reduce those requirements to make the plan look successful.

## Recover the actual state first

Read applicable repository instructions, `docs/archive/PRD.md`, and the operating rules/core iteration/anti-stall/route-pivot sections of `docs/GOAL_LOOP.md`. Read the first authority banners of `docs/archive/status/CURRENT.md` and `docs/archive/status/BLOCKERS.md` before consulting their extensive historical entries. Then inspect:

- `docs/archive/status/NEXT_MODEL_HANDOFF_2026-09-06.md`
- `docs/archive/status/ROUTE_B_ACTIVE_VERTEX_BINDING_2026-09-06.md`
- `docs/archive/status/ROUTE_B_PLAYER_COMMAND_EMISSION_2026-09-06.md`
- `docs/archive/status/PLAYABILITY_CRITICAL_PATH_2026-09-06.md`
- `docs/archive/status/GATES.md`, `TECH_DEBT.md`, and `FINISH_LINE.md`
- The dated independent reviews and execution-route decisions relevant to Route A versus Route B, discovered under `docs/status/`.

README and older plans contain stale milestones. Even the latest handoff omits later WIP visible in the current filesystem. Separate policy authority from factual freshness; reconcile claims against code, diffs, build configuration, and evidence. Do not read every historical log indiscriminately.

## What the implementation agent has accomplished—and has not

Route A is the older static-recompilation route using a Dolphin-derived compatibility runtime. Recorded evidence covers authentic boot, Outset interaction, and a transition, but performance and visual correctness remain inadequate. Do not describe those historical results as new measurements or transfer them to Route B.

Route B is the current source-native TWW/Aurora investigation. Its last qualified checkpoint records 180 headless original Link input/update/collision/camera/draw-command frames, real resources and lighting, and native J3D array bindings. Recorded captures are 85,024 bytes on first/last frames, with exact binding records for 21 body shapes. Public tests were 82/82 per configuration; private/oracle checks passed. Production create/phase-three/runtime link censuses still had 4/41/52 expected unresolved symbols. These are recorded results, not rerun during preparation of this prompt.

This proves bounded diagnostic execution. It does not prove GPU presentation, a live-controller session, a continuously integrated world, normal Route B boot, complete teardown, or playable acceptance. An optional `--visible` Metal path compiled but was never run according to the latest qualified record. Main-body arrays in the qualified replay were resource-backed; transformed-pointer coverage was a separate metadata-only fixture whose bytes were never drawn.

The active integration blocker is BW-P4-0126 / ROUTE_B_PLAYER_CAMERA_RUNTIME. The concern is that repeated source tiers, symbol closure, and increasingly large probe matrices may be advancing diagnostics faster than the actual application. Determine whether that concern is supported, and how to correct it without sacrificing authentic behavior.

## Concrete unfinished work and discrepancies to investigate

Inspect the complete tracked/untracked diff, especially:

- `route_b/CMakeLists.txt`
- `scripts/prepare_route_b.sh`
- `tests/route_b_player_init_services.cpp`
- `tests/route_b_player_phase_three_fences.cpp`
- `tests/route_b_private_camera_run_probe.cpp`
- `tests/route_b_world_geometry_services.cpp` (untracked)
- `patches/tww/0209-compose-world-teardown-authentic-tiers.patch` (untracked)
- `patches/tww/0210-world-room-memory-tier.patch` (untracked)

The new `bluewake_route_b_private_world_render_probe` composes original Room44 BG and PLAYER paths, intending to share the BG collision owner, draw world geometry, and use original deletion. It has no qualified world runtime result.

The handoff reports eight missing symbols: `dStage_escapeRestart`, `JAIZelBasic::seDeleteObject`, tree/grass/magma/flower deletion, `dStage_roomControl_c::getMemoryBlock`, and `dWood::Packet_c::delete_room`.

However, the currently available `/tmp/bluewake-world-render-sanitize-build.log` has a September 7 modification time and ends with a different eight-symbol failure:

1. `dStage_RoomCheck(cBgS_GndChk*)`
2. `dStage_changeScene(int, float, unsigned int, signed char)`
3. `dStage_chkPlayerId(int, int)`
4. `dStage_restartRoom(unsigned int, unsigned int)`
5. `dStage_turnRestart()`
6. `dStage_mapInfo_GetOceanX(stage_map_info_class*)`
7. `dStage_mapInfo_GetOceanZ(stage_map_info_class*)`
8. `dStage_nextStage_c::set(char const*, signed char, short, signed char, signed char)`

Determine the actual current failure and whether tier/preprocessor selection removed previously available definitions. An old log alone does not establish current reproducibility.

Patch 0209 is registered in the preparer and introduces a tier containing an empty `JAIBasic::deleteObject(void*) { /* Nonmatching */ }`. Trace whether it is linked/reached, what original deletion must do, and whether this quietly substitutes missing behavior. Do not certify authentic teardown merely because it links.

Patch 0210 is visibly unfinished: its hunk contains `inc d_com`, `inc d_stage`, and `check defined(X) here`; it is not registered in the preparer's patch list. Assess patch validity, dependency-checkout drift, and clean reconstruction of the WIP. Preserve the original files; propose concrete repairs in your report.

## Questions your investigation must answer

1. What is the shortest source-grounded path from this checkpoint to an observed native frame containing actual Link and Room44 geometry, then live movement/camera/collision in one continuous session? Distinguish that diagnostic milestone from normal boot and product acceptance.
2. For every current missing owner, identify its original definition, state, initialization/destruction dependencies, existing reusable implementation, and exact target/macro changes needed. Prefer coherent owners over more copied fragments. Explain where full translation-unit composition is blocked and where it is merely being avoided.
3. Audit the rendering path through J3D, native arrays, GX command capture/consumption, Aurora, and Metal presentation. What is proven, what remains unobserved, and what focused experiment would expose the first real renderer failure? Check pointer width, bounds, endian contracts, deformed arrays, matrices, packet lifetime, and resource ownership where the path actually depends on them.
4. Audit the world probe's shared collision ownership, scene/player/camera registration, resource conversion, draw-list order, room scheduling, and teardown. Does it require all three Room44 models every frame despite legitimate clipping? Establish a source-based visibility oracle. Inspect the 2 MiB capture bound, fixture cancellation, forced state, raw storage, fail-on-use fences, and `_Exit` where present; distinguish useful isolation from behavior that cannot enter the application.
5. Identify the concrete path from selected-scene diagnostics into normal startup, process scheduling, transitions, save/reload, and audio. Which existing probes/components can be composed, and which production integration is missing?
6. Is the current architecture/goal loop producing material progress? Examine recent representative iterations against its three-same-shape anti-stall rule. Recommend a bounded next milestone, any subsystem reframe, and stop criteria. If recommending a route change or revisiting Route A, explain what new evidence overturns prior decisions; avoid unsupported rewrites and calendar estimates.
7. What actually needs to unblock the agent: code work, missing evidence, upstream implementation, private data, a user decision, or an unavailable tool? The pending visible-test/Simulator constraint does not by itself prevent headless investigation. State exactly what can proceed independently.

## Evidence and validation expectations

Use code as primary evidence, with file paths, line numbers, symbols, and exact revisions. Inspect the local pinned `ref/tww`, `ref/aurora`, and relevant donor sources when available. If outside research materially helps, use primary upstream sources, cite URLs/revisions, and distinguish newer upstream capabilities from the pinned checkout.

Discover exact configure/run commands from CMake and the qualified evidence. Existing private trees are `build/route-b-aurora-gx-{sanitize,debug,release}`; public trees are `build/route-b-{sanitize,debug,release}`. Strict runtime settings are `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1` and `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`. Use focused experiments that discriminate hypotheses; reserve full regression matrices for changes warranting qualification. Do not treat expected link-census failures as passing production integration.

Never modify protected `ref/recompcore/GXRuntime/src/core/cpu_exception.c` or `ref/recompcore/GXRuntime/src/hle/hle_core.c`. Native Aurora changes belong to `ref/aurora` and must eventually be reproduced through patches. Keep private game bytes/assets/captures out of the report and Git. Do not stop other processes or run more than one BlueWake process/Simulator. Historical PID/session identifiers are not current authorization or process evidence.

## Required report

Return one self-contained Markdown report suitable for saving as `docs/research/INDEPENDENT_DEEP_DIVE_REPORT_<actual-date>.md`. Include:

1. **Verdict:** demonstrated capability, primary integration bottleneck, and recommended direction.
2. **Snapshot and access:** HEAD, dirty files, dependency revisions, available inputs, commands actually run, and unavailable evidence. Separate verified facts, historical claims, hypotheses, and unknowns throughout.
3. **Findings ranked by impact:** evidence, causal mechanism, affected files/symbols, confidence, and concrete remedy. Include the handoff/log discrepancy and patches 0209/0210 explicitly.
4. **Dependency map:** current failure through coherent subsystem owners to visible world/live input and then authentic application integration.
5. **Next three experiments:** for each, hypothesis, exact command or proposed minimal edit, expected observation, falsification condition, required inputs, bounded stopping condition, and next branch on success/failure.
6. **Implementation plan:** ordered changes and acceptance checks, what to defer, regression scope, and criteria for a stable checkpoint. Prioritize demonstrated product behavior over test counts.
7. **Unblock table:** issue, evidence, action, responsible party, whether external input is truly required, and independent work available meanwhile.
8. **Goal-loop correction:** measurable milestone per iteration, repeated-shape detection, escalation criteria, and evidence required before claiming visible, playable, normal-boot, performance, or complete acceptance.
9. **Copy-ready continuation brief:** a concise handoff the implementation agent can use after the user resumes implementation. Identify which findings it should validate locally and proposed updates to CURRENT, BLOCKERS, GATES, TECH_DEBT, and GOAL_LOOP. Preserve unresolved disagreements rather than turning hypotheses into authority.

Be direct about faulty assumptions and wasted work. Every major recommendation should identify code or an experiment that could prove it wrong. Your report will be ingested and preserved by the implementation agent before it revises its plan; it should be useful without access to your conversation.
