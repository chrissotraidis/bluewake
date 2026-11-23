# BlueWake priorities

The ranked backlog and its evidence. Owner: Chris. Updated November 22, 2026, with 0.13.0 frozen. The dated plan is
[GOAL_LOOP.md](GOAL_LOOP.md), the speed strategy and its results [PERFORMANCE.md](PERFORMANCE.md). The October 24
version of this list is in Git history and in [the October 15 record](status/PRIORITIES_2026-10-15.md).

## How it's ordered

Bugs that break play come first, then low or uneven speed, then narrower bugs, then new features. Keep *reported*,
*reproduced*, *fixed in main*, *shipped* and *reporter confirmed* apart. Close an issue only under the
[maintainer rules](../AGENTS.md#talking-to-people-on-github).

## The list

| Priority | What | Where it stands | Done when |
| --- | --- | --- | --- |
| 1 | **Freezes and crashes in play**: leaving Dragon Roost Cavern's room 9 ([#621](https://github.com/chrissotraidis/bluewake/issues/621)), the Wind Temple's R04 and R08 ([#318](https://github.com/chrissotraidis/bluewake/issues/318)), the Forsaken Fortress soft lock (#76, from a 0.4.0 report) | #621: a zeldaHeap header overwritten 4 bytes past a `d_a_door10` on a 0.12.0 Mac build. The value looks like the door's lift (`m358`), stored 0x14 bytes too far. The reporter has a heap watch and offered to test. #318: loads with opt-in loose RELs (#331). #76: asked to retest October 24, no reply | #621: the bad write's retrace and PC, then the translated store or host write behind it. #318: a real R04/R08 wait checked before a default. #76: closed under the 14-day rule if still silent |
| 2 | **Android crashes**: Android 17 at the first swapchain ([#622](https://github.com/chrissotraidis/bluewake/issues/622)), after the Dolby screen (#575), after choosing the disc (#476), screen recording (#238) | Android is experimental and built from source. #622 is deterministic on a Pixel 8a, inside Dawn's `CreateAndroidSurfaceKHR` | #622 reproduced and fixed in RecompCore's Dawn or surface code, then a phone run from a clean build |
| 3 | **Speed where the game thread is the limit**: Steam Deck ([#215](https://github.com/chrissotraidis/bluewake/issues/215)), slower PCs ([#427](https://github.com/chrissotraidis/bluewake/issues/427)), Android ([#560](https://github.com/chrissotraidis/bluewake/issues/560)) | 0.11.0: +16 to 22% on the Deck, 15 to 20% on a Ryzen 7 5700U. 0.13.0 adds #586's switches, off: +9.1% in Deck Game Mode at 3x with the HD look and Smooth Motion. Busy Outset still near 26 on the Deck; Android heavy scenes at 66 to 70% | A Deck run from 0.13.0's AppImage decides whether #586's switches become the Game Mode default. Then profile the game thread at busy Outset. Android: #551 once it's bit-identical |
| 4 | **Native sets** ([#451](https://github.com/chrissotraidis/bluewake/issues/451), #179) | #593 fixed the generators on its branch and re-certified round 7, but measured no Deck gain and closed unmerged. Rounds 4, 5 and 7 stay opt-in | Only with a measured gain on a slow machine. No new hashes before that |
| 5 | **Rendering bugs**: module art linked low (#545), clouds and waves flickering (#136), shadow flicker and the AO aura (#491), Dragon Roost Cavern flicker on Windows (#483), missing textures (#505) | #545: the modules linked high are fixed in 0.12.0 (#583); the low ones are #584, which needs a rebase. The others wait on their reporters with the current build | #584 rebased, checked on a Mac, off by default. Ask each older reporter to retest on 0.13.0 |
| 6 | **Camera and input**: after a bomb, the right stick zooms ([#644](https://github.com/chrissotraidis/bluewake/issues/644)), input delay (#493), the camera stick (#624), background input (#442), gyro aim (#188) | The camera jitter (#401, #430) was confirmed fixed by both reporters and closed November 22. #644 came from #430's reporter on 0.10.0 and isn't reproduced. #624: asked what they meant, November 22 | #644 reproduced or the reporter retests on 0.13.0 |
| 7 | **Widescreen and cutscenes**: ultrawide bars (#619), cutscene framing (#597) | 0.13.0 has the 3440x1440 and 2560x1080 presets (#623). #597: no bars when Z-targeting, and one opening cutscene that goes back to bars; asked which | #619's reporter confirms on 0.13.0. #597's opening cutscene found |
| 8 | **Requested features with a pull request**: speed up while a key is held (#620 → #629), the Steam library on SteamOS (#581 → #626, draft), Android's performance panel (#535 → #559), the Wind Waker menu style (#618, draft) | #629: changes requested (the right-stick click is taken). #559: needs a rebase | Each merges after its review, one per pull request |
| 9 | **Requests without one**: a higher resolution multiplier (#544), updating from inside the app (#506), graphics backend choice (#484), PAL discs (#507), 60 FPS without Smooth Motion (#434), free camera and aiming (#402, #259), keyboard prompts (#358), controller save-state keys (#357), custom mods (#152) | | Ranked again when someone takes one |
| 10 | **Distribution**: Linux (#56), PadMint reports (#104), Windows antivirus (#422) | The AppImage ships since 0.10.0. The antivirus steps are in AGENTS.md | #56 closed once a Linux player confirms the AppImage |

## Waiting on reporters

Asked to confirm a fix on November 15, after 0.12.0: #598, #597, #600, #152, #69, #523. Their 14 days run out on
November 29. #598's reporter confirmed the blur and suggested showing "Distance blur starts" as a factor (1.0x to
10x). #619 and #620 get their replies when 0.13.0 is published.

## Waiting on Chris

- Publishing 0.13.0, after the Windows zip and AppImage from his PC ([WINDOWS_BUILD_0.13.0.md](status/WINDOWS_BUILD_0.13.0.md)).
- His own drafts from October 24: #342 (the first Picto photo with cold shaders, RecompCore#39) and #347 (Android HD
  import). Both conflict with `main` and need a decision to refresh or close.

Not planned now: Japanese discs (#60; PAL is requested again in #507), Intel Macs (#48) and Switch (#62). CPUs without AVX2 keep their build
requirement (#77).

