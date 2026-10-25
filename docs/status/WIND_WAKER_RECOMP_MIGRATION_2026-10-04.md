# Wind Waker Recomp migration log

> Dated record, moved from `docs/` on October 11, 2026. The migration is done; what remains is in [MIGRATION_STATUS.md](../MIGRATION_STATUS.md).

This page records how Elliott's [Wind-Waker-Recomp](https://github.com/elliotttate/Wind-Waker-Recomp)
is being folded into BlueWake: what was decided, what has changed and where, and what is still open.
Current status in one table: [migration status](../MIGRATION_STATUS.md).

## Why

Wind-Waker-Recomp began on September 30, 2026 as Elliott's personal fork of BlueWake (from BlueWake
commit [`31b8a72`](https://github.com/chrissotraidis/bluewake/commit/31b8a722fee33457585df336093f70eea07f6382)).
It added a Windows port, Smooth Motion, save states, wall climbing, controller haptics and many fixes, and
its Windows builds quickly became the most used version of the project. Two repositories meant two issue
trackers, two sets of fixes to keep in step, and players unsure where to report problems.

Chris (BlueWake) and Elliott agreed to work on one project together: BlueWake, with Elliott as
a maintainer with equal say. Elliott added Chris as a collaborator on Wind-Waker-Recomp so the move can be
announced there.

## What has been done

| Date (JST) | Change |
| --- | --- |
| October 2-3 | Elliott's work through v0.4.0 merged into BlueWake with his commit authorship ([#37](https://github.com/chrissotraidis/bluewake/pull/37), [#38](https://github.com/chrissotraidis/bluewake/pull/38)) |
| October 3 | Session logs name the cause of slowdowns; Smooth Motion no longer pauses on single hitches ([#39](https://github.com/chrissotraidis/bluewake/pull/39)-[#41](https://github.com/chrissotraidis/bluewake/pull/41)) |
| October 3 | Elliott's later Windows branch merged: save-state crash fix, faster shader compilation, Smooth Motion for cloth, the wider training and his optimization defaults ([#43](https://github.com/chrissotraidis/bluewake/pull/43), [#45](https://github.com/chrissotraidis/bluewake/pull/45)) |
| October 3 | Fixes for reports on his tracker: camera flipping in water, an FPS counter that read 60 while Smooth Motion was paused, and Windows menu parity ([#44](https://github.com/chrissotraidis/bluewake/pull/44)) |
| October 4 | BlueWake publishes the Windows build ([#47](https://github.com/chrissotraidis/bluewake/pull/47)); Elliott's October 3 work merged ([#50](https://github.com/chrissotraidis/bluewake/pull/50), [#52](https://github.com/chrissotraidis/bluewake/pull/52)); rules for contributors and bots in AGENTS.md ([#53](https://github.com/chrissotraidis/bluewake/pull/53)) |
| October 4 | Wind-Waker-Recomp's ten releases unpublished (kept as drafts) and its download links pointed to BlueWake ([adac1e6](https://github.com/elliotttate/Wind-Waker-Recomp/commit/adac1e616376ab0d7d57eda51d82acf4dc607079)) |
| October 4 | The 28 open issues moved to BlueWake (27 recreated, one already there) and the 3 open pull request authors invited over (see below) |
| October 3 | Wind-Waker-Recomp's README and issue form point to BlueWake ([7b6a2c3](https://github.com/elliotttate/Wind-Waker-Recomp/commit/7b6a2c3aafa7619229a2dd9316251872f1cb9c9d)); the notice was then reworded to use first names only ([e4e1401](https://github.com/elliotttate/Wind-Waker-Recomp/commit/e4e14010b5c942d09eab0efd6b57454a734b80cf)) |

## Changes made on Wind-Waker-Recomp

Kept small and reversible, and Elliott's commit history is unchanged. Elliott gave Chris write access on
October 2 and said to make any changes needed.

- **README:** a notice at the top saying the project is moving to BlueWake, where to report bugs, that
  open issues will be moved, and that Windows saves carry over. "Questions or bugs?" points to BlueWake.
- **Issue form:** blank issues are off, the first option opens BlueWake's issue form, and the bug template
  begins with a pointer to BlueWake.
- **Repository description and website:** need repository admin access, which only Elliott has. Proposed:
  "Moving to BlueWake: github.com/chrissotraidis/bluewake. The Wind Waker static recompilation for
  Windows, Mac, iPhone and iPad", with the website set to BlueWake.

- **Issues and pull requests (October 4):** a comment from Chris on each open issue with its new
  BlueWake link, and on each open pull request asking its author to reopen it against BlueWake. Later
  that day the 28 issues were closed as moved; the pull requests stay open for their authors.
- **Releases (October 4):** Chris unpublished all ten releases (v0.1.0-macos.1 to v0.4.0), so players get
  their downloads from BlueWake. They are now drafts with their files intact; Elliott can publish any of
  them again from the Releases page. The README's download links point to BlueWake ([adac1e6](https://github.com/elliotttate/Wind-Waker-Recomp/commit/adac1e616376ab0d7d57eda51d82acf4dc607079)).

These are the only commits Chris's account has pushed to Wind-Waker-Recomp; every other commit there is
Elliott's own (GitHub's push events show the pushing account).

## Moving issues and pull requests

GitHub cannot transfer issues between repositories owned by different accounts, so they are recreated:

1. Check each open issue against BlueWake: some are already fixed or answered there (the
   [stability plan](../archive/status/STABILITY_PLAN_2026-10-03.md) sorts them).
2. Open it in BlueWake with its title and text, "Originally reported by @user in
   elliotttate/Wind-Waker-Recomp#N", and the label `from-wind-waker-recomp`. Tagging the reporter
   notifies them.
3. Comment on the original with the new link (and, where BlueWake already has a fix, what changed and how
   to confirm it), then close it as not planned here.
4. Pull requests: ask each author to reopen against BlueWake, keeping their authorship. If an author
   cannot, a maintainer ports the change and credits them as co-author.

Status: done October 4. Of the 28 issues open then, 27 are BlueWake
[#54-#80](https://github.com/chrissotraidis/bluewake/issues?q=label%3Afrom-wind-waker-recomp), and #19
was already BlueWake #13. The three pull request authors have been asked to reopen against BlueWake.

## Still to decide or do

- **Releases.** Decided October 4: BlueWake publishes the ready-made Windows build on its
  [Releases page](https://github.com/chrissotraidis/bluewake/releases/latest), the same as Wind-Waker-Recomp 0.4.0. Mac, iPhone and iPad stay on PadMint
  ([rights](../../RIGHTS_AND_LICENSES.md)). Wind-Waker-Recomp's releases were unpublished on
  October 4 (see above).
- **Windows testing** of BlueWake from a player's own disc: [checklist](../archive/status/WINDOWS_ACCEPTANCE_2026-10-04.md).
- **Remaining code:** Elliott's second set of native functions, his `lean_memory` step, and his
  newest Wind-Waker-Recomp commits on October 3 (WWHD texture import, Mac rendering fixes, smooth HUD
  interpolation).
- **The fork afterwards:** it stays available while the move is in progress; archiving it is planned once
  the issues are moved, Windows is checked and both maintainers agree.
- **A joint announcement** on Discord and X inviting testers and developers.

## Credits

Elliott's commits keep his authorship in BlueWake, and jointly adapted changes credit him as
co-author. Contributors to either repository keep their credit when their work moves.
