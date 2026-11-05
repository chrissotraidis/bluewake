# Tracker — The #552 gate and #553 fp_checks on the Apple builder

[PRD](PRD.md) · [Technical plan](TECH_PLAN.md)

## What was done

| Step | State |
|---|---|
| fp_checks in the Apple preparation steps, on by default, `--no-fp-checks` | Done, unit-tested |
| `--direct-gate` for training, with its fingerprint key | Done, unit-tested |
| Gate host code on Apple clang/arm64 | Checked by reading the code only |
| Mac build and launch | **Not done**: no Mac |
| Speed on Apple hardware | **Not measured** |
| Save states identical to a build without fp_checks | **Not checked on arm64** |

## Open risks

- #553 M2's `dolrecomp_f32_from_bits` change applies to every platform. It was tested with DAZ off and on, on x86 only. arm64 flushes denormals through FPCR.FZ, not MXCSR, so it needs its own check.
- On Apple the module grows with fp_checks and lean_memory. That hasn't been checked against the iOS link and size limits.
- The Mac settings menu has no row for `BLUEWAKE_DIRECT_GATE` until [#531](https://github.com/chrissotraidis/bluewake/pull/531)'s rows land. Until then, it's an environment variable.

## Decision

The switches mirror Linux exactly. Whether to keep fp_checks in the Apple default is decided after a Mac build passes the save-state equality check and at least 6 interleaved launches.
