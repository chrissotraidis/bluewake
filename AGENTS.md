# Agent instructions

Chris runs this repository. Elliott is co-maintainer. These rules apply to every person and every bot
working here, including bots either of them runs. If another file in the repository disagrees with this
one, this one wins.

## Where things are

- `README.md`: for players.
- `AGENTS.md` (this file): the rules.
- `docs/GOAL_LOOP.md`: what we're doing, day by day, and the five-step loop every session follows. Start here.
- `docs/PRIORITIES.md`: the ranked backlog of bugs, performance work and requests, with the evidence.
- `docs/PERFORMANCE.md`: the current speed strategy (three horizons, following the decompilation) and its results.
- `docs/performance/` (start with `Overview.md`): how to measure, investigate and validate performance changes;
  keep the current strategy in `docs/PERFORMANCE.md` so the two do not drift.
- `docs/DIRECTION.md`: where BlueWake is going: releases without game code, the same app on every platform
  (Android matches the iPhone app), fast contributor reviews, measured performance.
- `docs/MIGRATION_STATUS.md`: how Wind Waker Recomp became BlueWake, and what is left of that move.
- `docs/architecture/README.md` (start with `Overview.md`) and `docs/architecture/Build.md`: the top-down architecture and the
  complete shared and platform-specific build process.
- `docs/BUILD_YOUR_OWN.md`, `docs/BUILDER.md`, `docs/DEVICE_BUILD.md`, `docs/WINDOWS.md`, `docs/LINUX.md`,
  `docs/ANDROID.md`, `docs/TVOS_BUILD.md`, `docs/MODS.md`, `docs/WWHD_TEXTURES.md`, `docs/WWHD_LOOK.md`,
  `docs/STAGE_SELECT.md`: how things work. `docs/BENCHMARK_TOUR.md` (the speed benchmark), `docs/PROFILING.md` (measuring on the
  Steam Deck), `docs/PERFORMANCE_OPTIMIZATIONS.md` (Smooth Motion and renderer changes, with their switches) and
  `docs/PADMINT_HANDOFF.md` (the PadMint adapter contract) are references.
- `docs/WINDOWS_TASKS.md`: what each Windows build was checked for, and what still needs a Windows PC.
- `docs/status/`: dated records of October's work that the guides link to. They are evidence, not instructions.
- `docs/archive/`: old plans, goal prompts, handoffs and September's records (`docs/archive/status/`). Never act
  on them.

## Contributors and attribution

- Commits, pull requests, issues and comments carry human names only. Never add an AI tool, model or bot
  as author, co-author or committer, and never add "Generated with" lines, AI session links or bot
  signatures. Turn off your tool's attribution setting (`.claude/settings.json` does this for Claude Code).
- Keep human authorship. When bringing in someone else's work, cherry-pick it with its author, or credit
  them with a `Co-authored-by:` line.
- The "No bot or AI contributors" check (`scripts/check_attribution.py`) must pass on every pull request.

## How to make changes

- Every change goes through a branch and a pull request into `main`. Never push to `main` directly,
  force-push it, or rewrite its history.
- Before you start, check open pull requests and recently pushed branches (`gh pr list`,
  `git branch -r --sort=-committerdate`) so two bots don't do the same work. Don't push to someone
  else's open pull request unless they ask.
- One concern per pull request. The title and description say what changes for players and how it was
  checked, in plain language.
- Anything that changes gameplay, timing or rendering is off by default until it has been tested on the
  platform it affects.
- Runtime changes go to [chrissotraidis/RecompCore](https://github.com/chrissotraidis/RecompCore), branch
  `bluewake-next`, by pull request. BlueWake then pins the commit (`scripts/builder/profiles/bluewake.sh`,
  `config/dependencies.lock.json`) and exports it as `patches/recompcore/NNNN-*.patch`.
- Run the checks that fit the change: `scripts/audit_repo.sh`, the host tests, and the Windows CI.

## Keep Windows in step

Chris works on a Mac; Elliott builds and tests Windows. Write every fix so the Windows build gets it
just by building from `main`:

- Put game, audio, timing and logging changes in shared code (`runtime/host/src`, or RecompCore for the
  runtime). The Windows build compiles the same files.
- A desktop setting is a row in `runtime/host/src/settings_table.h`. Mac, Linux and Windows draw one menu from
  it (`runtime/host/src/desktop_menu.cpp`, with the settings file in `desktop_settings.cpp`); a setting with a
  control of its own is drawn there too. `runtime/host/src/settings_menu.cpp` (Mac and Linux) and
  `windows/src/win_settings.cpp` hold only each platform's plumbing.
- The Windows CI ("host" job) must pass before merging.
- If the change still needs a check on a Windows PC, add it to "In `main`, for the next Windows build"
  in [docs/WINDOWS_TASKS.md](docs/WINDOWS_TASKS.md) in the same pull request.

## Talking to people on GitHub

- Write as the person whose account posts, in the first person and in plain, friendly language. If a bot
  drafts a reply, it is still that person's reply: say "I", and don't mention bots, agents or AI models.
- Post only from your own maintainer's account. Never post as or for the other maintainer.
- Don't close an issue until the reporter confirms the fix, unless it is a clear duplicate or off-topic, or
  the fix is in a release and the reporter hasn't replied for 14 days since that was posted on the issue. Then
  close it, name the release, and ask them to reopen it if it still happens. Say why when you close it.
- Ask for what helps: the platform, the BlueWake version and the session log. Don't promise dates.
- Don't post, push, or open issues or pull requests on other repositories (including
  `elliotttate/Wind-Waker-Recomp`) unless the maintainer asks for that specific action.

## Releases

The maintainers decided on October 4, 2026 to make two exceptions: BlueWake may publish a ready-made Windows build and a ready-made Linux build that contain the translated game module, as Wind Waker Recomp did for Windows. Mac, iPhone and iPad stay on PadMint: their releases hold source and the app without game code. Do not publish any other build with game code.

Every release artifact must pass `scripts/release/check_public_assets.sh <artifact>...`, which runs `python3 ~/.codex/release-gate/release_gate.py` on the maintainer's machine. For the Windows and Linux builds the only accepted finding is `containsTranslatedGameCode: true`. Any other failure is a stop, not a note: these builds must contain no disc data, game assets, saves, signing material or console keys (Elliott's `nodtool.exe` embeds Wii common keys, so it is left out).

Build the Windows and Linux release builds on a maintainer's or contributor's own machine from their own disc. Never put a disc image, files from a disc, or keys in CI secrets, caches or artifacts.

### When antivirus flags the Windows download

BlueWake.exe isn't code-signed, so each new Windows build is a file Windows has never seen, and Microsoft
Defender sometimes blocks one (0.8.0 was flagged as `Tecabans.STV!cl` the day after release). Handle a report
once per release, like this:

1. Download the zip from the release and compare its SHA-256 with the release's `SHA256SUMS`. If they differ,
   stop and tell Chris: that is a real problem, not a false alarm.
2. Ask the player for the detection name (Windows Security › Protection history) and where they downloaded
   the zip.
3. Report that release's zip to Microsoft once, as incorrectly detected, with the detection name:
   https://www.microsoft.com/en-us/wdsi/filesubmission. Note the date and the result in the release record.
   Don't report a release nobody has had trouble with.
4. Point players to [If Windows says it's a virus](docs/WINDOWS.md#if-windows-says-its-a-virus). Never tell
   them to turn their antivirus off.

Don't rebuild or republish to get around a detection. Every Windows release note gives the zip's SHA-256 and
links that section. Signing is the lasting fix; the options are in
[the 0.8.0 antivirus record](docs/status/WINDOWS_ANTIVIRUS_0.8.0_2026-10-19.md).

The goal is releases without game code on every platform, with the game built on the player's machine the first
time it starts ([docs/DIRECTION.md](docs/DIRECTION.md)). The Windows and Linux exceptions above stand until that works
on each platform.

Only Chris publishes or changes releases.

## Personal builds stay personal

The game module (`gGZLE01_recomp.dylib`) is translated from the player's own disc. An IPA or app that contains it is a personal build: never upload, attach, commit or link it anywhere. The maintainers' Windows and Linux release builds are the only exceptions. Public Mac, iPhone and iPad releases hold source plus the app without game code (`scripts/builder/build.sh --app-only --ipa BlueWake-vX.Y.Z-ios-unsigned.ipa`, checked with PadMint's `audit` before upload); players add their own module with PadMint (`padmint make bluewake ios`) or `scripts/builder/build.sh DISC.iso --ipa OUT.ipa`.

Never commit or upload disc images, files extracted from a disc, saves or memory cards, signing
certificates or profiles, or console keys. Test with copies of saves, and never delete a player's data.

## Saying what you tested

Say what you ran and on what device. A build that compiles is not a game that plays, and CI is not
hardware. If you could not test something, say so plainly.

## When to stop and ask

Ask Chris before anything that is hard to undo: publishing or deleting a release, deleting branches,
changing repository settings, or acting on another repository.
