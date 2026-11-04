# PRD — The #552 gate and #553 fp_checks on the Apple builder

Status: **Wired, on by default, unmeasured.** No Mac, iPhone or iPad numbers yet. Nothing here is a measured gain on Apple hardware.
Source: [#553](https://github.com/chrissotraidis/bluewake/pull/553), with [#552](https://github.com/chrissotraidis/bluewake/pull/552) merged in.
Scope: the Apple builder (`scripts/builder/build.sh`, `--platform macos|ios`). Excluded: Windows, and #554's huge pages, which are Linux-only.
[Technical plan](TECH_PLAN.md) · [Tracker](PROGRESS.md)

## Problem

#552 and #553 sped up the Steam Deck by +5.5% and +5.0%. Only the Linux builder can reach them:

| Change | Linux builder | Apple builder before this PR |
|---|---|---|
| #553 fp_checks pass | On by default, `--no-fp-checks` | Never runs |
| #552 gate | On by default since `ed834a53f`: the module is built with `BLUEWAKE_DIRECT_GATE_DEFAULT` and asks the app to open the gate. `--no-direct-gate` leaves it out | No option. The module and host hold the gate, but it stays closed unless `BLUEWAKE_DIRECT_GATE=1` is set, and training plays with it closed |

## Goal

The Apple builder gets the same switches as the Linux builder, with the same defaults:
- fp_checks is part of the default module preparation set, and `--no-fp-checks` removes it;
- the gate is in the default set for macOS and iOS, and it needs the direct calls. The module opens it, so iPhone and iPad need no setting. `--no-direct-gate` leaves it out, and training plays with it open;
- `BLUEWAKE_DIRECT_GATE=0` closes it on a Mac for A/B runs, and `=1` opens it on any module.

## Not goals

- Huge pages (#554): they're Linux-only.
- Claiming a gain on Apple hardware before it is measured there.
