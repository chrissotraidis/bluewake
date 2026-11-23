# PROGRESS — Five save state slots

Status: **written, not built yet.** Waiting for CI and a hand check on a build.
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md)

## Story

**What we tried.** The owner asked for multiple save states, 5 slots, picked from the menu only, with F5/F8 left as they are.

**What we found.** The save/load path already takes any file path and has one place for each: the hotkey save and the hotkey load. A slot is just a fixed file name there.

**Decision.** Slot files `slot-1.bwstate` to `slot-5.bwstate` beside the quick states, reached through a popup from a new footer button. No new keys.

## Log

- 2026-11-21: host side, menu popup, footer wrapping and the Linux guide written. Not built yet (owner asked to hold builds).

## Next

- CI on all desktop builds.
- The hand check in the technical plan, on Linux first.
