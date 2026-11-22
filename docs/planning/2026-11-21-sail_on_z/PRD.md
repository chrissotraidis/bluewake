# PRD — Sail on Z

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md)

## Why

To sail, the player puts the sail on X, Y or Z and presses that button. Most players want that button for something else on land, so they keep moving the sail on and off a button.

## What changes for players

- A new switch in **Gameplay › Sailing**: **Sail on Z (on the boat, Z raises and lowers the sail)**, off by default.
- With it on, while Link is on the boat and has the sail, **Z is the sail**.
- **The player's own Z is never changed.** Whatever they put on Z is still there off the boat, and in their saves, including a save made on the boat.
- X and Y are untouched, and a sail the player put on X or Y still works there.

## Not in scope

- Choosing another button for the sail.
- Anything off the boat.

## Success

- On the boat with the sail, Z raises and lowers it, whatever is on Z.
- Off the boat, Z uses the player's own Z item. A save made on the boat and loaded back has the player's Z item on Z.
- With the switch off, nothing changes.
