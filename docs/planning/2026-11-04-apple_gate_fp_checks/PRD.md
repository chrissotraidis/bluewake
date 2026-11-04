# PRD — The #552 gate and #553 fp_checks on the Apple builder

Status: **Wired, unmeasured.** No Mac, iPhone or iPad numbers yet. Nothing here is a measured gain on Apple hardware.
Source: [#553](https://github.com/chrissotraidis/bluewake/pull/553), with [#552](https://github.com/chrissotraidis/bluewake/pull/552) merged in.
Scope: the Apple builder (`scripts/builder/build.sh`, `--platform macos|ios`). Excluded: Windows, and #554's huge pages, which are Linux-only.
[Technical plan](TECH_PLAN.md) · [Tracker](PROGRESS.md)

## Problem

#552 and #553 sped up the Steam Deck by +5.5% and +5.0%. Only the Linux builder can reach them:

| Change | Linux builder | Apple builder before this PR |
|---|---|---|
| #553 fp_checks pass | On by default, `--no-fp-checks` | Never runs |
| #552 gate | `--direct-gate` trains with the gate open; the gate is opt-in at runtime | No option. The module and host already hold the gate, so `BLUEWAKE_DIRECT_GATE=1` works at runtime, but training always plays with it closed |

## Goal

The Apple builder gets the same switches as the Linux builder, with the same defaults:
- fp_checks is part of the default module preparation set, and `--no-fp-checks` removes it;
- `--direct-gate` trains with the gate open;
- the gate stays opt-in at runtime.

## Not goals

- Turning the gate on by default anywhere.
- Claiming a gain on Apple hardware before it is measured there.
