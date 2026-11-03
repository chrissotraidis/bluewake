# Wind Waker Recomp is now BlueWake

Elliott Tate's [Wind-Waker-Recomp](https://github.com/elliotttate/Wind-Waker-Recomp) and BlueWake are one
project, and BlueWake is where development continues. Bug reports, requests and pull requests go to
[BlueWake's issues](https://github.com/chrissotraidis/bluewake/issues). Updated October 15, 2026.

How the move happened, step by step, is in [the migration log](status/WIND_WAKER_RECOMP_MIGRATION_2026-10-04.md);
the detailed evidence is in [the reconciliation ledger](status/FORK_RECONCILIATION_2026-10-02.md).

## Done

- Elliott's Windows port, rendering, camera, controls, settings, save states and speed work, through his v0.4.0
  release and his later branch, are in `main` with his authorship (#37, #38, #40, #43, #45, #50, #52, #53).
- His work since October 3 is being ported by hand, one pull request each: draw fusion (#225), faster single
  loads (#228), return dispatch and the watch-list fix (#234, #235). The rest is in [GOAL_LOOP.md](GOAL_LOOP.md).
- The shared runtime is [chrissotraidis/RecompCore](https://github.com/chrissotraidis/RecompCore), branch
  `bluewake-next`; `config/dependencies.lock.json` pins the exact commit.
- All 28 open issues moved on October 4 (#54 to #80, and #13), with the originals closed and linked. The
  authors of its open pull requests were asked to reopen them here; the Android port is now #93.
- Wind-Waker-Recomp's README, issue form and download links point here, and its releases were unpublished
  (kept as drafts) on October 4. Elliott is a BlueWake collaborator.
- Releases: Windows builds from `main` since 0.5.0 (October 5), the latest 0.7.0. Mac, iPhone and iPad: the app
  without game code, and you build the game with PadMint. Linux: built from source since 0.7.0; a ready-made
  AppImage is allowed and waits for a contributor's own-disc build ([AGENTS.md](../AGENTS.md#releases)).

## Open

| Item | Who | State |
| --- | --- | --- |
| Wind-Waker-Recomp's repository description and archiving | Elliott (admin), with Chris | Not archived; its description still says "Personal source fork of BlueWake for The Wind Waker on iPhone and iPad". |
| Elliott's work since October 3: natives rounds 4, 5 and 7, the upload and vertex changes | BlueWake | Each change's state is in [PERFORMANCE.md](PERFORMANCE.md#the-plan); the order is in [GOAL_LOOP.md](GOAL_LOOP.md). |
| Elliott's HD renderer work (lighting, shadows, the GameCube/HD switch) | Elliott | Where it lives isn't known yet. BlueWake rebuilt the look itself, off by default ([WWHD_LOOK.md](WWHD_LOOK.md)), and #108's hotkey is in `main` as F2. |
