# The benchmark tour

**Done:** jslay88 wrote it in #360, merged October 17. What's left is the first run on a build made from someone's
own disc (the optional check below), at the next release candidate. The rest of this page is kept as the task was set.

One task. It adds the command the speed work is waiting on. It does not make the game faster, and it does not need you to already know the runtime.

The behaviour is already decided. Read [PERFORMANCE.md](PERFORMANCE.md) from "Phase 1: the benchmark tour" through the check at the end of that section. This page is who does it, and when to stop. If the two disagree, PERFORMANCE.md wins.

This is a fit for a person opening the pull request, or for a mid-range or high-range coding model doing the edit while the person checks the result. A small model should not be given it. The usual failure is an edit to the game or the runtime that the spec does not ask for.

## Done when

- `scripts/bench_tour.py` does what Phase 1 describes, including `compare`.
- `python3 tests/test_bench_tour.py` passes. The test uses made-up log lines only. No disc, no card, and no built game.
- That test is listed in `.github/workflows/audit.yml`, next to the guest-checkpoint test.
- The pull request contains those changes and nothing about gameplay, timing, or rendering.

A run on a real build is optional. Do it only with a build you made from your own disc, and only to prove the script:

- Two runs of the same build.
- Identical checkpoints.
- Stop speeds within 3%.
- Every stop loads. The log's scene lines name each stage.

Paste the `compare` output in the pull request. Name the machine, the commit, and whether the run was headless or rendered. Do not attach the log, the card, the save, or the module.

If you cannot run it, say so, and leave the "Not written yet" line in PERFORMANCE.md. The script and the test are still the whole task.

## What to hand a model

Give it this page and the Phase 1 section. These rules are the whole job:

- Implement that section. Do not add stops, flags, or another way to measure.
- Do not change the game, the runtime, the builders, rendering, or timing.
- Do not guess stage codes. The default stops are the ones written in Phase 1. For `--untrained`, pick two places that are not in `TRAINING_TOUR` in `scripts/windows/build.py`, and take their codes from `config/stage_select/names.tsv`.
- Write the test in the style of `tests/test_guest_checkpoints.py`. It must pass with `python3 tests/test_bench_tour.py`.
- `compare` prints the change at each stop and whether the checkpoints match. If they do not, it names the first retrace where they differ.
- No disc image, extracted game file, card, save state, module, optimization profile, or session log in the commit.
- The commit and the pull request use your name. No model or bot as author, and no generated-with line.
- One pull request. If something in the spec cannot be done, stop and say what blocked you. Do not invent a replacement.

## How you check it

Before you open the pull request, or before you accept the model's edit:

1. The diff is the script, the test, and the audit workflow line. PERFORMANCE.md changes only after a real run, and only to replace "Not written yet" with the stops that actually ran.
2. `python3 tests/test_bench_tour.py` passes.
3. `git status` shows no card, save, log, module, or disc file.

## What this moves

After this command exists, a speed change is one pull request, measured the same way, with the numbers written in PERFORMANCE.md. That is the next speed work (priorities row 9). This task does not include that change.

Do not spend the machine on compiler-flag trials, a new training profile, or porting native functions. The comparison has to exist first. Native certification is a separate bug ([#179](https://github.com/chrissotraidis/bluewake/issues/179)).

## If it stalls

Two attempts that do not match Phase 1 are enough. Stop. Write down the command and the first error. Do not keep editing the game to make the script easier.
