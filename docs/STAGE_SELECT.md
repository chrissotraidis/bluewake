# The developers' stage select

Wind Waker still has the menu its developers used to jump to any room: 40 lines covering every dungeon,
island, sea square, interior and 48 cutscenes, 468 places in all. Nothing in the finished game opens it.
BlueWake can, in English, from your own loaded file, so Link arrives with your hearts, items and boat.

It is a developer tool and off unless you turn it on. It changes no game code; it only steers the game's
own scene changes and writes English over the menu's Japanese text while the menu is open.

## Turning it on

In the settings (F1): **Developers' stage select (F7)**, under **Gameplay** on every desktop build. It's
off by default and takes effect the next time BlueWake starts.

The Linux and Windows builders put the English names in a `stage_select` folder beside the app, and
BlueWake finds them there. On a Mac, set `BLUEWAKE_STAGE_SELECT_NAMES=<repository>/config/stage_select`
for them; without it the menu shows its Japanese names.

`BLUEWAKE_STAGE_SELECT=1` in the environment still turns it on, whatever the toggle says, so existing
launch commands and `.bat` files keep working.

Then:

- **Hold R while starting a file** (from choosing the quest log until the screen fades): the stage select
  opens instead of your save point. Without R the file loads as usual.
- **F7 during play** asks first (see below), then opens it from wherever you are. **F7 in the menu** takes
  you back: to the entrance of the room you were in, with your time of day and day of the week. The game
  does not keep Link's exact position. From your file's own play, F7 does nothing while an event runs (a
  cutscene, a conversation, a door): leaving one halfway can mark it seen without giving you what its end
  gives. Once you've warped, it always works, so it gets you out of a cutscene that ends with nowhere to
  go (Storm Ganon part 2 waits forever at its end).
- **On a controller**, Back (View on Xbox, Share or Create on PlayStation, ⧉ on the Steam Deck) opens
  BlueWake's settings, and the button under the stage select's toggle does what F7 does: **Go to the stage
  select** in play, **Back to where I was** in the menu. Before a file has started it is greyed out: hold R
  as you start one instead.

## Saving is off after you use it

Once the stage select has opened, the game can't save until BlueWake restarts. Warping can leave a file
in a state the story never reaches by itself (a cutscene's flag set, a scene marked seen without its
reward, items given or not), and the safest fix is not to write any of it. Until you restart, the
memory card answers as if it had been taken out, and the game's no-card message is reworded to say why:
the pause menu's Save says "The game cannot be saved. Saving is off because the stage select was used.
Restart BlueWake to save again." Your card is fine, and the log says the same. (Refusing only the writes
was worse: the game either retried on its own forever or called the card damaged.) Restart BlueWake and
your file is exactly as it was before you opened the menu. Save first: nothing after opening it can be
kept, including the save the game offers after the credits (the one that starts the Second Quest).
Save states (F5) don't get around it. A state taken after the stage select was used still can't be
saved from when it's loaded later, even after a restart: the rewritten message is in the game's memory
the state holds, and BlueWake looks for it whenever the game mounts the card.

So that this never costs you progress, **F7 during play asks first**: "Go to the stage select?", with
**Cancel** (Esc, or B on a controller; it has the focus) and **Go to the stage select** (or F7 again).
Save first if you have progress to keep. Holding R as a file starts doesn't ask, since the file has just
loaded. The settings menu's stage select section says the same, and says when saving is off.

If you'd rather keep saving and take the risk, start BlueWake with `--stage-select-saves` (or set
`BLUEWAKE_STAGE_SELECT_SAVES=1`, as on a Mac). Saving then stays on, F7 doesn't ask, and the settings
menu says so. Back up your card first: a save made after warping can leave your file somewhere the story
can't go on from. Save states taken with it on can be saved from later, even without it.

The stage select is in the desktop apps (Mac, Linux, Windows and the Steam Deck). The iPhone and iPad app
doesn't have it.

## Controls

The menu lists them on screen.

| Button | What it does |
| --- | --- |
| D-pad Up/Down | Pick a line |
| A / B | Next / previous room on that line |
| R / L | Choose a different spawn point in the room (only ones the room has) |
| X / Y | Time of day: normal, fast, morning, noon, evening, night, or a fixed hour |
| D-pad Left/Right | Day of the week |
| Z | Which half of the story Link's cutscene animations are set for: before or after the Helmaroc King fight (set for you on marked cutscenes) |
| Start | Go |

## Notes on the names

Some names end in a note:

- **(before)** and **(after)**: the cutscene needs Link's cutscene animations from before or after the
  Helmaroc King fight at the Forsaken Fortress, where the game switches sets. (The menu's bottom line says
  "before rescue" and "after rescue": the flag is set at the end of Aryll's rescue, and the next scene,
  the fight, loads the second set.) The game keeps only one set loaded (`/res/Object/LkD00.arc` or
  `LkD01.arc`, chosen by story flag 0x2D01), and playing a cutscene from the other half crashes. Every
  cutscene on the Cutscenes line is marked, by where it falls in the story (ZeldaSpeedRuns' "Flags and
  Triggers" page lists which cutscenes use which set). Picking a marked cutscene sets the flag for it, so
  it plays; F7 back to where you were puts your file's own setting back. Z is the developers' own "demo
  23" switch (controller 3's A in the original menu) for everywhere else.
  Some cutscenes still won't start when your file is missing the story progress they expect. The stage
  select doesn't fake that progress.
- **(crash)**: the place's default spawn point crashes when started cold (Wind Temple R04 and R08: the
  room's doors ask about Link before he is fully created). Pick another spawn point with R/L.
  `BLUEWAKE_LOOSE_REL_RETRACES=1` fixes both (#318): a REL the game reads from the disc's `/rels/` folder then
  takes a retrace to load, as on the console, so the doors are created after Link. With it on, a save state
  (F5) waits while such a REL is still loading, and loading one (F8) is refused until it is done (#350).
  It's off by default, and the two keep their mark until it's on.
- **(hangs)**: a developer test room that loads but never lets Link in.

## For tests and benchmarks

The same mechanism can skip the menu entirely. With these set, BlueWake starts a quest log by itself and
goes straight to a place:

| Variable | Meaning |
| --- | --- |
| `BLUEWAKE_WARP` | `stage:room:point[:layer]` (the game's own codes, for example `Siren:0:0`), a menu line and room (`6.0`), or a name from the names files (`Valoo roars`). A name that fits several rooms lists them and goes nowhere. |
| `BLUEWAKE_WARP_FILE` | `1`, `2` or `3`: press A for you until that quest log starts. An empty log stops it. |
| `BLUEWAKE_WARP_TIME` | `0`–`23` or `HH:MM` (to the game's nearest degree, four minutes), or `morning`, `noon`, `evening`, `night`. On islands where time stands still the clock still runs on at dawn and dusk to the next settled time, so capture soon after arriving |
| `BLUEWAKE_WARP_DATE` | The save's day, `0`–`65535`: the day of the week and the moon's phase (the day mod 7; until noon, the day before) |
| `BLUEWAKE_WARP_STOP_AFTER` | End the run that many play frames after Link appears (for sweeping many places) |

Every place in the menu was warped to this way, headless, on Windows, from a late-game file: 451 of 468
arrive. The rest are the places with notes above, plus a few that crash at `pc=0x8180FFF0` through a host
bug in how a finished disc read calls back into the game. A given build crashes the same way every time,
but which places it hits moves when the host code changes; it needs its own fix.

`scripts/stage_select_sweep.py APP CARD OUT` runs that sweep (one headless run a place, 120 frames after Link
arrives, several at once) and checks that the card is unchanged after each. On a Mac (M3 Max, `main` at `d73ac95`,
October 12) from an early file at Outset, 446 of 468 arrive in about 8 minutes, and the card is unchanged after all
468. The other 22: the four test rooms marked (hangs) and Wind Temple R04 and R08 marked (crash), as above; and 16
late-game cutscenes the early file has no story progress for (9 crash right after arriving, calling `0x80000000`
from `0x80109608`, a missing object; 7 wait), with the Ganon room cutscene and the Ending. None crashed at
`0x8180FFF0`.

## Names files

`config/stage_select/` holds three, loaded in order; a later file replaces the names of the rooms it
covers and keeps everything else:

1. `names.tsv`: our own names, each line's place (stage, room, spawn point, layer), the room's spawn
   points, and the notes.
2. `names-randomizer.tsv`: dungeon rooms named after their Wind Waker Randomizer item locations (MIT,
   see `NOTICE-randomizer`).
3. `names-decomp.tsv`: other rooms named after the actors in them, from the zeldaret/tww decompilation.

A name is at most 31 letters; the menu shortens long ones to fit the screen next to their line's name.
