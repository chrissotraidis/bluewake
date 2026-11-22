# PROGRESS — Sail on Z

Status: **written, not built yet.** Waiting for CI and a hand check on the boat.
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md)

## Story

**What we tried.** The owner asked for an opt-in that binds the sail to Z, so there's no need to keep putting the sail on a button, without changing the player's own binds.

**What we found.** The game keeps the buttons twice: the save's inventory slots and the play state's item numbers, which Link reads. Only the play copy needs to say "sail".

**Decision.** On the boat, put the sail in the play copy's Z every update. Off the boat, put the save's Z item back. Never write the save's copy.

## Log

- 2026-11-21: layout checked on the owner's save state; option, setting and menu reload written. Not built yet (owner asked to hold builds).

## Next

- CI.
- The hand check in the technical plan, including the HUD's Z icon.
