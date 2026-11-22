# PRD — Show "Present without gamescope's frame layer" on Linux only

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md) · Follow-up to [#586 review](https://github.com/chrissotraidis/bluewake/pull/586#pullrequestreview-6011550248)

## Why

The switch only does something under gamescope (Steam Deck Game Mode), which only runs on Linux. On Mac and Windows it showed in the menu anyway and did nothing.

## What changes for players

- Linux: nothing. The switch is where it was, off by default.
- Mac and Windows: the switch is gone from the menu, and the settings file no longer keeps it.

## Success

- The switch shows on Linux only; every other setting shows where it did before.
