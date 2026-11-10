# PRD — Why higher render settings drop the Deck below 30 fps

Status: In progress — goal: 3x + full HD look + Smooth Motion holds 30 on the Deck; step 1 (faster cloud shadows) building
Source base: BlueWake `main` at `ee991d3`
[Technical plan](TECH_PLAN.md) · [Tracker](PROGRESS.md)
Scope: find and remove avoidable GPU work so the Steam Deck holds 30 game frames a second at higher render settings, without changing the picture. Excluded: lowering picture quality, shipping a GPU clock setting, and enabling an unqualified experiment by default.

## Goal: full HD look at 3x holds 30

**3x render scale + the full HD look + Smooth Motion holds 30 game frames a second on the Steam Deck**, paced, from the owner's save state at Outset, with the Game Mode build and settings. Today: 24.5 fps, so about **+22 %** is needed. On the Deck's 1280×800 screen, 3x already supersamples about 3 samples a pixel; 4x adds smoothing, not detail, and is the docked/TV target after this one.

Four steps, in order, each opt-in in the Performance section first and qualified paced (six or more launches each way, pictures):

| Step | What | Expected share of the gap | Picture |
|---|---|---|---|
| 1 | Faster cloud shadows (baked noise texture; RecompCore#54, #586) | Cloud shadows cost 7.9 % at 3x + HD | Within half an 8-bit step |
| 2 | Cheaper ambient occlusion (four full-screen passes today) | AO costs 6.5 % at 3x + HD | Target: unchanged or within an 8-bit step |
| 3 | One pass for the per-pixel HD effects (edge light, torch light, cloud and AO apply), and a cheaper far blur | Far blur 5.2 %, edge light 1.4 %, bloom 1.1 % at 3x + HD | Bit-identical for the merge; far blur unchanged or within a step |
| 4 | In-between frame reuses the real frame's off-screen copies | Only if 1–3 fall short | Very small differences; opt-in |

The 3x shares (M4-T01, turned off, paced, 24.39 baseline): cloud shadows 7.9 %, ambient occlusion 6.5 %, far blur 5.2 %, edge light 1.4 %, bloom 1.1 %; Smooth Motion off reaches 29.92. The effects together are about the whole 22 % gap, so steps 1–3 must remove most of each effect's cost, or step 4 is needed.

## The problem

On the Steam Deck, raising the render scale from 3x to 4x, or turning on the HD look, drops the game below 30 frames a second and makes it stutter. Measured paced, the way the game is played, from the owner's save state at Outset with the Game Mode build and settings:

| Setting | Game frames a second | Game thread busy | GPU busy | GPU clock |
|---|---|---|---|---|
| 3x | 29.0 | 88 % | 85 % | 553 MHz |
| 4x | 25.8 | 82 % | 89 % | 913 MHz |
| 3x + HD look | 24.5 | 79 % | 89 % | 938 MHz |
| 4x + HD look | 18.2 | 69 % | 93 % | 1266 MHz |

As the settings rise the game thread gets *less* busy: it is waiting for the GPU. The owner's own play session showed the same (4x: GPU 97 % busy, 26–28 fps; 4x + HD: 19–20 fps with Smooth Motion skipping in-between frames).

## Retracted earlier conclusions

Earlier runs on this PR measured **unpaced**, with an empty settings file (direct-call gate, huge pages, efficient effects, mods and widescreen all off) and a different save state. Unpaced, the game thread runs flat out and holds the chip at its 15 W limit, which hides a full GPU. These conclusions came from those runs and are withdrawn:

| Withdrawn | Why |
|---|---|
| The cause is CPU/GPU power sharing lowering the game thread's clock | Unpaced artifact. Paced, the GPU is full and the game thread waits |
| A 1100 MHz GPU cap confirms it | The cap was slower than no cap in 9 of 10 cases, and slower in the owner's play |
| 4x costs −6.3 % at Outset; HD look −1.2 % to −7 % | Unpaced numbers. Paced: −11 % and −16 % (4x + HD: −37 %) |
| Render scale 2 is faster than 3 on the Deck (+1.3 %) | Unpaced, wrong settings. Not retested |
| RecompCore #53 (staging-copy batching) rejected | Measured unpaced without the owner's settings. Reopened for a paced retest |

Still valid: almost no draw is identical between the real and in-between frame (12 of 1.69 M at Outset), so skipping identical passes stays closed. That is a draw count, not a timing.

## What matters

- **It's what players hit.** 4x and the HD look are the settings people turn on, and they cost 11 % to 37 % of the frame rate on the Deck.
- **The GPU has headroom in clock.** It runs at 553–1266 MHz of its 1600 MHz while 85–93 % busy, so the governor is part of the picture.
- **Smooth Motion's in-between frame is the largest cost**, and its passes cost two to four times what the same passes cost on the real frame.

## Success and constraints

| Measure | Target |
|---|---|
| Measurement | Paced, from the owner's save state, Game Mode build and settings (render_bench `--paced` with every setting passed) |
| Repeatability | At least four interleaved launches per setting for diagnosis; six for a keep/reject decision |
| Performance gate | A source change is kept only if it is ahead in every launch |
| Picture | No change to what is drawn |
| Rejected experiments | Remove product integration and keep the measured result |
