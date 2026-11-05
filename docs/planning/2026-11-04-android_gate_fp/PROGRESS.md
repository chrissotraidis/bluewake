# Progress — The direct-call gate and FP checks on Android

[PRD](PRD.md) · [Technical plan](TECH_PLAN.md)

## M1: the builder

- [x] M1-T01: Branch from #553 with #552 and #555 merged in, all in their opt-in state (2026-11-04).
- [x] M1-T02: fp_checks comes with the Windows builder's defaults. `--no-fp-checks` added; `--conservative` drops it.
- [x] M1-T03: `--direct-gate`: the module's training `launch.env` opens the gate. It's part of the module record and the provenance, and it requires direct calls.
- [x] M1-T04: `tests/test_android_perf_options.py`: 6 tests, passing locally (`python3 tests/test_android_perf_options.py`), in `audit.yml`. The existing Android and Windows builder tests still pass.
- [x] M1-T05: Huge pages on Android: compiled in by `__linux__`, but a no-op with the APK's uncompressed libraries. A `!__ANDROID__` guard is recommended for #554 (TECH_PLAN).

## M2: measure on a phone

- [ ] M2-T01: Build with and without `--direct-gate` (and `--no-fp-checks`), train on the device, and A/B with `scripts/android/measure.py`: interleaved runs, checkpoints identical.
- [ ] M2-T02: Record the speed per scene here. Keep it if it's ahead in every run.

## Evidence

| Task | Build | Command | Result |
|---|---|---|---|
| M1-T04 | This branch | `python3 tests/test_android_perf_options.py`; `test_android_app_only.py`, `test_android_training.py`, `test_windows_perf_options.py`, `test_fp_checks.py` | All OK |
