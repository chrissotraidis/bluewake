# PRD — Five save state slots

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md)

## Why

A save state today is one keypress: F5 writes a new `quick-<retrace>.bwstate` every time, and F8 loads the last state saved or loaded (after a relaunch, the newest file). There's no way to keep a few spots and go back to a chosen one, such as a boss door and a sea chart, without managing files by hand.

## What changes for players

- The menu's footer gains **Save state slots...**. It lists **5 slots**, each with the time it was saved, or "Empty".
- **Save** writes the game over that slot; **Load** puts it back (greyed out while the slot is empty). Either closes the menu, and the game does it at its next clean point, as F5 and F8 do.
- A status line says what happened: "Saved to slot 2", "Slot 3 loaded", "Slot 4 is empty", or that the game was busy.
- **F5 and F8 are unchanged**, and don't touch the slots.

## Not in scope

- New keys or controller shortcuts for slots: slots are in the menu only (owner's choice). F1, F2 and F7 are already taken, and number keys can be mapped to game buttons.
- Slot names, thumbnails, deleting a slot from the menu.
- The iPhone, iPad and Android menus, which are native and don't have the desktop footer.

## Success

- On Linux, Windows and the Mac, each of the 5 slots saves, survives a relaunch, and loads back the state it holds, independently of the others and of F5/F8.
- No change to F5/F8 or to `BLUEWAKE_SAVE_STATE` / `BLUEWAKE_LOAD_STATE`.
