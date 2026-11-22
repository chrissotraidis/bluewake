# Technical plan — Five save state slots

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md)

## Where save states are today

`main.c`'s host_state_*: F5 / F8 (`mouse_camera.c`) and the menu footer's buttons call `bluewake_save_state_hotkey(load)`, which sets a flag. At the next retrace `host_state_turn` loads, or arms a save that `host_state_try_save` writes at the next clean point (draw done, nothing unsafe in flight). Files go to `host_state_dir()` (`BLUEWAKE_STATE_DIR`, the Mac app's States folder, else the working directory).

## Change

- `save_state.h`: `BLUEWAKE_STATE_SLOTS` (5), `bluewake_save_state_slot(slot, load)` and `bluewake_state_slot_info(slot, when, size)`.
- `main.c`: `g_state_hotkey_slot` says which slot the pending save or load is for; 0 keeps today's behaviour, and `bluewake_save_state_hotkey` sets it to 0. A slot is `slot-N.bwstate` in `host_state_dir()`, so it sits beside the quick states and follows the same folder rules. Saving writes that path over; loading checks the file exists first, then uses the same safety checks and `host_state_load` as F8. `bluewake_state_slot_info` is a `stat` of the file, with its local modification time for the menu.
- `menu_shared.h`: a fifth footer button, **Save state slots...** (`BW_MENU_STATE_SLOTS`), and the footer height counts as many rows as the buttons wrap into (it assumed at most two).
- `desktop_menu.cpp`: the button opens a popup with a 3-column table: the slot, its time or "Empty", and **Save** / **Load**. Choosing one closes the popup and the menu. The game's B closes the popup first (`bw_menu_back`).

Loading a slot sets `g_state_last_path`, as any load does, so F8 afterwards reloads that slot. That matches F8's documented "the last state saved or loaded".

## Risks

- Thread safety is the same as today's flags: the menu (main thread) writes the slot and then a `volatile` flag; the game thread reads them once a retrace.
- `localtime_r` vs `localtime_s`: `_WIN32` picks the Windows one.

## Verification

- CI: Linux, Windows and Mac builds compile.
- By hand, on a build: save to slots 1 and 3, relaunch, check the times, load 3, then 1, and an empty slot (status "Slot N is empty"); F5 then F8 still use quick-<retrace> files.
