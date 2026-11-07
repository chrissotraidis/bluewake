# PRD — The direct-call gate and FP checks on Android

[Progress](PROGRESS.md) · [Technical plan](TECH_PLAN.md) · Builds on [#552](https://github.com/chrissotraidis/bluewake/pull/552), [#553](https://github.com/chrissotraidis/bluewake/pull/553) and [#555](https://github.com/chrissotraidis/bluewake/pull/555)

## Why

Two speed changes reach only the Linux build today, and #555 and #556 bring them to Windows and Apple:

- #552's direct-call gate: +5.5% on the Steam Deck, and +8.0% on the desktop.
- #553's fp_checks pass: +3.05% on the Steam Deck, and +3.2% on the desktop.

The Android builder (`scripts/android/build.py`) is built on the Windows builder. It imports `scripts/windows/build.py` and runs its source preparations unchanged. Once #555 teaches the Windows builder fp_checks, Android can have it too. It needs its own option for the gate's training, because it trains on the phone with its own launch settings.

## What changes for players

- **Android builds prepare fp_checks by default**, as Linux and Windows builds do. `--no-fp-checks` turns it off.
- **The gate stays opt-in at run time.** It's built into every module. `--direct-gate` trains with it open, and `install.py --env BLUEWAKE_DIRECT_GATE=1` opens it in play.

## Not in scope

- Making the gate a default.
- #554's huge pages. See [TECH_PLAN](TECH_PLAN.md#huge-pages-on-android): on Android today they would do nothing, and they shouldn't be reachable there.
- A menu switch. #531's Config rows are for the desktop menu.

## Success

- The focused builder tests pass on Linux CI.
- An Android A/B shows the gain is real on a phone: interleaved runs with `scripts/android/measure.py`, checkpoints identical. **Not yet measured.**
