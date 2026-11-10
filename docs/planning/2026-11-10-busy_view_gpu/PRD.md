# PRD — The GPU's share at busy views: HD effects, power and Smooth Motion

Status: Planned — follows #586
Source base: BlueWake `main` with #586 (cheaper HD look effects and the present switches)
[Technical plan](TECH_PLAN.md) · [Tracker](PROGRESS.md)
Related: #586 (where this was found), #GAMETHREAD (the game thread at busy views), #590 (render_bench in Game Mode)

## The problem

With #586's four switches, 3x with the full HD look and Smooth Motion holds 30 game frames a second in six of seven bench scenes on the Steam Deck, in Game Mode. The busiest view the owner found at Outset doesn't: about **1,950 draws a frame**, against about 500 at the bench's view, and **26.5** game frames a second.

Measured in Game Mode at that view (bench state `8-busiest`, three launches each, #586's switches on):

| Turned off | Game frames a second | GPU | Chip power |
|---|---|---|---|
| Nothing | 26.53 | 84 % busy, 1,062 MHz | 13.4 W |
| Smooth Motion | 30.02 (+13 %) | 62 %, 701 MHz | 13.1 W |
| HD look | 29.19 (+10 %) | 80 %, 805 MHz | 13.4 W |
| Distant islands | 26.57 (no change) | 83 %, 1,074 MHz | 13.5 W |

The game thread is about 98 % busy in every row. Turning off the HD look leaves its work unchanged (the same draws, the same GX worker load) and still gains 10 %, because the GPU then needs a lower clock and leaves the processor more of the chip's power. At this view the CPU and GPU share the chip's ~15 W, and GPU work costs game frames twice: as GPU time, and as power taken from the game thread.

## Goal

**The busiest Outset view holds 30 game frames a second at 3x with the full HD look and Smooth Motion**, in Game Mode, keeping the 60-fps feeling (every in-between frame shown, owner's play test).

## Two parts

| Part | What | Why |
|---|---|---|
| 1 | Measure what each HD effect costs at the busiest view, with the processor's clock logged beside the GPU's clock and the chip's power, then make the largest cheaper | The HD look costs 10 % here, mostly through power. Per-effect costs so far are from lighter views |
| 2 | A Smooth Motion in-between frame built by reprojection, opt-in | Smooth Motion costs 13 % here: a full replay of the scene on the GPU, and its worker's CPU time |

The game thread's own work at busy views is the other half of the problem, planned on #GAMETHREAD.

## Success and constraints

| Measure | Target |
|---|---|
| Measurement | Game Mode, the owner's settings and shortcut (#590's `--display gamemode`), paced, state `8-busiest` and #586's seven scenes |
| Repeatability | 3 launches per setup for diagnosis; 6 or more, every launch ahead, to keep a change |
| Picture | Exact changes: frame-exact pictures with Smooth Motion off. Reprojection: approximate by design, judged by pictures and the owner's play test |
| Opt-in | Each change behind a switch in Config → Performance, off by default |
| 60-fps feeling | Every in-between frame shown; owner's play test |
