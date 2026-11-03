# BlueWake priorities

The ranked backlog and its evidence. Owner: Chris. Updated October 24, 2026. The next actions and Sunday's review
are in [GOAL_LOOP.md](GOAL_LOOP.md); the speed strategy is in [PERFORMANCE.md](PERFORMANCE.md).
Older measurements remain in the dated records, including [October 15](status/PRIORITIES_2026-10-15.md).

## How it's ordered

Bugs that break play come first, followed by low or uneven performance, then narrower bugs and new features.
Keep *reported*, *reproduced*, *fixed in main*, *shipped* and *reporter confirmed* distinct. Close an issue only
under the [maintainer rules](../AGENTS.md#talking-to-people-on-github). What 0.9.0 includes, and its checks, are in
[status/RELEASE_0.9.0.md](status/RELEASE_0.9.0.md); nothing is published before Chris's review.

## The list

| Priority | What | Current evidence | Next / done when |
| --- | --- | --- | --- |
| 1 | **Camera jitter with direct right-stick control** — #401, #430 | #430 now supplies a Windows 0.8.0 log with Smooth Motion off; repeating the interpolation toggle is not a useful next request. Draft [#455](https://github.com/chrissotraidis/bluewake/pull/455) reports a Linux reproduction and missed first-block hook; it is in the 0.9.0 test candidate. | Review the dispatch change; Windows/Mac direct-control on/off and normal camera play checks. Reporter confirmation before closing either report. |
| 2 | **Native generation blocks the next performance comparison** — [#451](https://github.com/chrissotraidis/bluewake/issues/451), #179 | The generator only rewrites `_at` accesses while lean deadlines emits `_ato`; the reporter also found missing block leaders. Rounds 4, 5 and 7 are present but opt-in. This is a generation problem before it is a request for more hashes. | Reproduce the default pipeline and establish preparation provenance. Preserve deadline accounting, decline behavior and translation/hash guards; generate, compare guest state, then measure one candidate. |
| 3 | **Low and uneven speed on Deck, PCs and Android** — #215, #427 | Merged Deck records #452/#453 distinguish cache-flush and GX costs; #454's Outset draw record (plans before batching, no speedup claimed) is merged. The #433 author withdrew the 25–27% host-flag slowdown after interleaved runs. #427's 0.8.0 reporter sees improvement but still reports drops. | Use one retained state (#450), alternate builds, separate game-thread, worker and GPU limits for the measured scene. GroundCross #436 and compiler plan #437 closed unmerged; #438's matched tuning trial reports no gain. No new defaults based on those experiments. |
| 4 | **Android build quality and sustained performance** — #75, #238, #449 | Experimental own-disc port in `main`; Android already has lean/behaviour-timing defaults. [#445](https://github.com/chrissotraidis/bluewake/pull/445) now rejects full training that never reaches control. Its tests establish profile acceptance, not faster gameplay. | Fresh `--retrain` baseline on the actual phone, recording module and app training separately, settings and cold/warm conditions. Keep the screen-recording GPU-loss report #238 separate. Android HD import #347 still needs phone/lifecycle checks. |
| 5 | **Mac and iOS speed defaults** — draft #431 | [Mac/iPad recipe measurements](status/APPLE_RECIPE_TOUR_2026-10-20.md) show 64–81% more game frames per second in the measured routes; hands-on play is pending. | Chris's Mac/iPad play checks on Sunday. Re-evaluate #392's slow-chunk flags on that recipe, with matched training. |
| 6 | **Smooth Motion flicker and sun shadows** — #136, #371, #398 | #398's combined runtime includes Dawn and both rendering fixes; #371's intermediate pin still omits Dawn. Contributor rendering evidence is platform-specific. | Preserve the accepted runtime fixes in each pin; Mac/iPad visual checks on Sunday and Windows checks in [WINDOWS_TASKS.md](WINDOWS_TASKS.md). |
| 7 | **Linux build repair** — #433 | RecompCore#44 corrects the Dawn BMI2 fix's Apple/Android callback compatibility; downstream draft #446 passes Linux/AppImage, Windows host, audit, attribution and sanitizers. | Renderer/cache check, then runtime PR and downstream pin in order. Keep #433 open for reporter confirmation. No claimed Deck speedup from the host flag. |
| 8 | **HD textures on every platform** — #336, #443, #444, #347 | #443's content-bounds fix is merged after parser tests; #444's key files, pasted keys and `keys.txt` handling are merged. Android menu integration remains draft #347. | A Windows menu import and an iPhone or iPad import with a `keys.txt`. Do not ask players to upload keys or disc content. |
| 9 | **Menu and control follow-ups** — #316, #188, #402, #442 | The shared desktop menu (#384–#388) is merged. #442 requests controller-only background input on Windows 0.8.0. Independent settings for free camera versus first-person/item aim are requested in #402. | Sunday controller-menu pass and #316's iPad tap test. Keep background controller input separate from keyboard/mouse and pause-on-focus-loss behavior; investigate gyro drift from the requested device evidence. |
| 10 | **Wind Temple waits and stage sweep** — #318 | Opt-in loose REL loading (#331) and save-state waiting (#350) exist; [record](status/WIND_TEMPLE_2026-10-15.md). | F5/F8 during a real R04/R08 wait before a default decision; the broad stage sweep on a chosen candidate. |
| 11 | **Cold first Picto photo** — draft #342 / RecompCore#39 | Separate from the subject-recognition fix in #267, whose original reporter now confirms the photo quests. | Resolve the candidate's integration and perform its cold-cache platform checks. Do not treat recognition confirmation as cold-cache acceptance. |
| 12 | **Distribution and later features** — #56, #75, #100 | Linux AppImage tooling exists; Android remains an own-disc source build. Cross-platform iOS building is draft #100. | A future release candidate needs its own platform, packaging and publication checks. A code-free first-run builder remains the longer-term direction, not a promised APK or release date. |

## Confirmations and outstanding reports

- **#267:** the original reporter confirms the Picto quests now work, and this issue is now closed; the cold-first-photo follow-up remains separate.
- **#325:** the Linux resize reporter confirmed and closed the issue on October 22.
- **#263:** the sea-chart reporter had already confirmed the fix, and Chris closed it. The cache-flush fallback remains enabled;
  a measured no-gain experiment is not a reason to remove a correctness fix.
- **#427:** performance improved for the reporter on 0.8.0, but drops remain. Leave it open.
- Other control, sound, portable-mode and older scene reports remain subject to their own reporter evidence;
  a source merge does not by itself close them.

## Cache-flush measurement follow-up

[jkoehler11's #452](https://github.com/chrissotraidis/bluewake/pull/452) measured the same Deck build (`51bee38`
with local preparations), headless and unpaced, warped from one save state, in two interleaved rounds each way.
Forest Haven ran at 33.85/34.25 game FPS with fallback and 34.01/34.20 without it (means 0.2% apart); the sea by the
Fortress at 31.72/32.12 with it and 31.81/32.02 without (0.0%). The thread was 99% busy in both variants, and
`[texture-cache] fallback writeback=on` identifies the enabled runs. No cost was measurable in these two scenes;
the correctness fallback stays enabled. This does not erase the earlier Mac observation of roughly 1–3% cost.

## Contributor coordination

| Work | Current handoff |
| --- | --- |
| Native sets / #451 | Establish the preparations used by the recorded leaves with jslay88 and Elliott; jkoehler11 offered to rerun generation and benchmarks. No generated code, module or disc uploads. |
| Deck measurement | Keep the controlled results and closed experiments linked from #215. Update #421 for the new generator blocker; retain #448 as a completed negative-result record rather than another implementation plan. |
| Rendering stack | pdale-boop's #371/#398 and RecompCore#40/#42; retain the Dawn compatibility fix when reconciling the final pins. |
| Importer | jkoehler11's #443/#444; review their stack and retain platform acceptance tasks. |
| Android | LiquidAzir's port and measurements; gather sustained measurements on more phones before changing defaults. |
| Chris | Sunday gameplay/visual review. A public release requires a later explicit decision. |

Not planned now: European/Japanese discs (#60), Intel Macs (#48) and Switch (#62). The Wii U-style interface (#57)
belongs to the HD-look work. CPUs without AVX2 retain their existing build requirement (#77).
