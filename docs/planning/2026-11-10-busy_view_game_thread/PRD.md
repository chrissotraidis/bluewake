# PRD — The game thread at busy views

Status: Planned — follows #586
Source base: BlueWake `main` with #586
[Technical plan](TECH_PLAN.md) · [Tracker](PROGRESS.md)
Related: #586 (where this was found), #591 (the GPU's share at busy views), #590 (render_bench in Game Mode), #503 (the earlier Outset game-thread work, merged)

## The problem

At the busiest Outset view the owner found (about **1,950 draws a frame**, bench state `8-busiest`), the Steam Deck runs **26.5** game frames a second at 3x with the HD look and Smooth Motion, in Game Mode, with #586's switches on. The presentation stall #586 removed isn't the limit here (present waits 13 ms a second). The game thread is.

A CPU profile at that view (Linux perf, build `9dde6aa9d`, #586's settings):

| | |
|---|---|
| Game thread busy | 98.6 % |
| Cycles per game frame | 124 M (about 3.1 GHz) |
| Instructions per cycle | 1.52 |
| Time in the game's translated code | about 89 % |
| Time in BlueWake's host code | about 10 % |
| Largest single function | 1.5 % |

Most of the translated code runs from the copy on 2 MB pages (`BLUEWAKE_MODULE_HUGEPAGES`), so perf shows it as `[JIT]` with no names. The time is spread thin: the game simply runs a lot of its own logic per frame at this view, for the actors, their models and their draw lists.

The GPU makes it worse: CPU and GPU share the chip's ~15 W, and at this view the chip draws ~13.4 W. That side is planned on #591.

## Goal

**Cut the game thread's time per frame at busy views by 10 % or more**, exactly (the same game state at the same retraces), so that with #591 the busiest Outset view holds 30 game frames a second at 3x with the full HD look and Smooth Motion.

## Success and constraints

| Measure | Target |
|---|---|
| Exactness | Guest checkpoints identical to the switch off, in every scene |
| Measurement | Game Mode (#590), paced, the owner's settings, `8-busiest` and #586's seven scenes; perf counters headless for attribution |
| Keep a change | 6 or more launches each way, every launch ahead |
| Opt-in | Each change behind a switch in Config → Performance, off by default, until qualified |
| Rejected experiments | Removed, with the measured result kept in the tracker |
