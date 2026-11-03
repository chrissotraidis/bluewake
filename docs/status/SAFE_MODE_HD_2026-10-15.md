# Safe startup: HD look off

October 15, 2026. Local branch `codex/safe-mode-hd-off`, based on `origin/main`
`0db549f` (PR #339). Merged through [#343](https://github.com/chrissotraidis/bluewake/pull/343) after green Windows/Linux CI.

## Problem and change

The reported Mac reproduction after #339 was
`BLUEWAKE_SETTINGS=none BLUEWAKE_SAFE_MODE=1 BLUEWAKE_HD_LOOK=1`: the safe-mode
line announced Fast sound and 4:3, then `[hd-look]` announced every effect on.
Mac/Linux's reset omitted the HD master and per-effect variables. Windows also
kept the saved `hd_look`, which its later `apply_hd_look()` reads through the
session settings.

Both menus now reset the master to off at startup and clear the eight
`BLUEWAKE_HD_*` effect overrides. Clearing, rather than forcing each override to
zero, lets F2 or the menu turn the effects back on in the same session. There is
no permanent safe-mode check in the reload path and no shared renderer change.

Mac/Linux also remove the overrides from the reset file's unmanaged keys, after
a successful backup. Windows resets the saved master only after backup, and
resets the session master even if backup fails. Existing backup and session-only
startup behavior is preserved. Normal startup and its overrides are unchanged.

## Checks on the Mac

M3 Max, macOS 26.6.2, Apple clang 21.0.0. The source fixtures use synthetic
settings; the graphical check below uses a copied card and isolated settings.

- `python3 tests/test_safe_mode_hd.py`: passed, 16 cases. The fixtures compile
  production startup, settings read/write and HD reload functions with C++17,
  `-Wall -Wextra -Werror` (unused globals in extracted code are excluded).
  Platform/UI and renderer calls are stubbed. Both menus cover normal explicit
  overrides, safe startup, injected backup failure, and HD off → on → off while
  the safe-mode flag remains set. Mac/Linux also cover `BLUEWAKE_SETTINGS=none`;
  Windows also covers recovery from `launch.pending` and an unrelated settings
  edit preserving the saved HD choice after backup failure.
- Backups match the original synthetic settings byte for byte. A failed backup
  leaves the file unchanged at startup. Successful recovery writes the master
  off and removes stored Mac/Linux effect overrides while keeping unrelated keys.
- The same suite against source from `0db549f`: the 12 recovery cases fail on
  HD remaining on; all four normal-mode cases pass. The temporary baseline source
  files were removed after the check; no checkout or ref was changed for it.
- `tests/settings_state_test.cpp` compiled and passed with C++17 and
  `-Wall -Wextra -Werror`, using the existing Aurora headers read-only.
- `tests/launch_marker_test.c` compiled and passed with C11 and
  `-Wall -Wextra -Werror`.
- `bash scripts/audit_repo.sh` and `git diff --check`: passed.

Local logs and the two standalone host-test binaries are in the ignored
`build/safe-mode-hd-fixtures/` directory. The fixture script generates and removes
its C++ files and binaries in a temporary directory.

## Graphical Mac check

A fresh build of `107de805`, using the existing `ee99bbc` runtime pin, was run
at Outset on the M3 Max with Metal. The isolated settings started with the HD
master and bloom/AO/sun-shadow overrides on. `BLUEWAKE_SAFE_MODE=1` backed up
that file byte for byte, saved the reset and logged all eight effects off.
The Display menu showed HD off. Closing F1 and pressing F2 enabled the effects;
pressing it again disabled them, confirmed by the image and reload log. The
copied card's SHA-256 was unchanged after exit.

The local build is `build/app-macos` in the existing menu-review worktree;
private logs/settings are under `build/safe-hd-app-oct15`. No renderer changes
or personal saves were used for this check.

## Still to check

Windows/Linux CI pass. Native Windows/Linux runs remain separate gates. The fixture
uses POSIX environment operations to stand in for Windows `_putenv_s`; it is
not a Windows build. The hardware check is in [Windows tasks](../WINDOWS_TASKS.md).
No release was made.
