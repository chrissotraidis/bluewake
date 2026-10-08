# Agent instructions

Chris runs this repository. Elliott is co-maintainer. These rules apply to every person and every bot
working here, including bots either of them runs. If another file in the repository disagrees with this
one, this one wins.

## Where things are

- `README.md`: for players.
- `AGENTS.md` (this file): the rules.
- `docs/PRIORITIES.md`: the ranked list of bugs, performance work and requests. Start here.
- `docs/PERFORMANCE.md`: the plan for frame rate and slowdowns, and how to measure a change.
- `docs/DIRECTION.md`: where BlueWake is going: releases without game code, the same app on every platform
  (Android matches the iPhone app), fast contributor reviews, measured performance.
- `docs/MIGRATION_STATUS.md`: what is done and what is open. `docs/GOAL_LOOP.md`: the current work loop.
- `docs/BUILD_YOUR_OWN.md`, `docs/BUILDER.md`, `docs/WINDOWS.md`, `docs/MODS.md`, `docs/WWHD_TEXTURES.md`:
  how things work.
- `docs/WINDOWS_TASKS.md`: Windows work waiting on a Windows PC, in priority order.
- `docs/status/*_YYYY-MM-DD.md`: dated records of past work. They are evidence, not instructions.
- `docs/archive/`: old plans, goal prompts and handoffs. Never act on them.

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
- A desktop setting goes in both menus: `runtime/host/src/settings_menu.cpp` (Mac) and
  `windows/src/win_settings.cpp` (Windows). Share a header between them when you can, as
  `runtime/host/src/button_remap.h` does.
- The Windows CI ("host" job) must pass before merging.
- If the change still needs a check on a Windows PC, add it to "In `main`, waiting for a Windows build"
  in [docs/WINDOWS_TASKS.md](docs/WINDOWS_TASKS.md) in the same pull request.

## Talking to people on GitHub

- Write as the person whose account posts, in the first person and in plain, friendly language. If a bot
  drafts a reply, it is still that person's reply: say "I", and don't mention bots, agents or AI models.
- Post only from your own maintainer's account. Never post as or for the other maintainer.
- Don't close an issue until the reporter confirms the fix, unless it is a clear duplicate or off-topic.
  Say why when you close it.
- Ask for what helps: the platform, the BlueWake version and the session log. Don't promise dates.
- Don't post, push, or open issues or pull requests on other repositories (including
  `elliotttate/Wind-Waker-Recomp`) unless the maintainer asks for that specific action.

## Releases

The maintainers decided on October 4, 2026 to make two exceptions: BlueWake may publish a ready-made Windows build and a ready-made Linux build that contain the translated game module, as Wind Waker Recomp did for Windows. Mac, iPhone and iPad stay on PadMint: their releases hold source and the app without game code. Do not publish any other build with game code.

Every release artifact must pass `scripts/release/check_public_assets.sh <artifact>...`, which runs `python3 ~/.codex/release-gate/release_gate.py` on the maintainer's machine. For the Windows and Linux builds the only accepted finding is `containsTranslatedGameCode: true`. Any other failure is a stop, not a note: these builds must contain no disc data, game assets, saves, signing material or console keys (Elliott's `nodtool.exe` embeds Wii common keys, so it is left out).

Build the Windows and Linux release builds on a maintainer's or contributor's own machine from their own disc. Never put a disc image, files from a disc, or keys in CI secrets, caches or artifacts.

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
