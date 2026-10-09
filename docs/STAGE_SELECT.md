# The developers' stage select

Wind Waker still has the menu its developers used to jump to any room: 40 lines covering every dungeon,
island, sea square, interior and 48 cutscenes, 468 places in all. Nothing in the finished game opens it.
BlueWake can, in English, from your own loaded file, so Link arrives with your hearts, items and boat.

It is a developer tool and off unless you turn it on. It changes no game code; it only steers the game's
own scene changes and writes English over the menu's Japanese text while the menu is open.

## Turning it on

Set two environment variables before starting BlueWake:

```
BLUEWAKE_STAGE_SELECT=1
BLUEWAKE_STAGE_SELECT_NAMES=<repository>/config/stage_select
```

Without the second one the menu works with its Japanese names.

Then:

- **Hold R while starting a file** (from choosing the quest log until the screen fades): the stage select
  opens instead of your save point. Without R the file loads as usual.
- **F7 during play** opens it from wherever you are. **F7 in the menu** takes you back: to the entrance
  of the room you were in, with your time of day and day of the week. The game does not keep Link's exact
  position.

## Controls

The menu lists them on screen.

| Button | What it does |
| --- | --- |
| D-pad Up/Down | Pick a line |
| A / B | Next / previous room on that line |
| R / L | Choose a different spawn point in the room (only ones the room has) |
| X / Y | Time of day: normal, fast, morning, noon, evening, night, or a fixed hour |
| D-pad Left/Right | Day of the week |
| Z | Which half of the story Link's cutscene animations are set for: before or after Aryll's rescue |
| Start | Go |

## Notes on the names

Some names end in a note:

- **(before)** and **(after)**: the cutscene needs Link's cutscene animations from before or after
  Aryll's rescue at the Forsaken Fortress. The game keeps only one set loaded (`/res/Object/LkD00.arc`
  or `LkD01.arc`, chosen by story flag 0x2D01), and playing a cutscene from the other half crashes.
  Press Z until the line at the bottom says the right half. This is the developers' own "demo 23"
  switch (controller 3's A in the original menu). It sets a real story flag, so put it back before you
  save a real file.
- **(crash)**: the place's default spawn point crashes when started cold (Wind Temple R04 and R08: the
  room's doors ask about Link before he is fully created). Pick another spawn point with R/L.
- **(hangs)**: a developer test room that loads but never lets Link in.

## For tests and benchmarks

The same mechanism can skip the menu entirely. With these set, BlueWake starts a quest log by itself and
goes straight to a place:

| Variable | Meaning |
| --- | --- |
| `BLUEWAKE_WARP` | `stage:room:point[:layer]` (the game's own codes, for example `Siren:0:0`), a menu line and room (`6.0`), or a name from the names files (`Valoo roars`). A name that fits several rooms lists them and goes nowhere. |
| `BLUEWAKE_WARP_FILE` | `1`, `2` or `3`: press A for you until that quest log starts. An empty log stops it. |
| `BLUEWAKE_WARP_TIME` | `0`–`23`, or `morning`, `noon`, `evening`, `night` |
| `BLUEWAKE_WARP_STOP_AFTER` | End the run that many play frames after Link appears (for sweeping many places) |

Every place in the menu was warped to this way, headless, on Windows, from a late-game file: 451 of 468
arrive. The rest are the places with notes above, plus a few that crash by chance through a host bug in
how a finished disc read calls back into the game (the run stops at `pc=0x8180FFF0`). Which places that
bug hits changes with timing; it needs its own fix.

## Names files

`config/stage_select/` holds three, loaded in order; a later file replaces the names of the rooms it
covers and keeps everything else:

1. `names.tsv`: our own names, each line's place (stage, room, spawn point, layer), the room's spawn
   points, and the notes.
2. `names-randomizer.tsv`: dungeon rooms named after their Wind Waker Randomizer item locations (MIT,
   see `NOTICE-randomizer`).
3. `names-decomp.tsv`: other rooms named after the actors in them, from the zeldaret/tww decompilation.

A name is at most 31 letters; the menu shortens long ones to fit the screen next to their line's name.
