# Windows source port: CI and integration notes (October 1 to 4, 2026)

> Dated record, moved from the "Status" section of [WINDOWS.md](../WINDOWS.md) on October 11, 2026.
> Windows releases have been built from `main` since 0.5.0; see [WINDOWS_TASKS.md](../WINDOWS_TASKS.md).


Experimental in BlueWake. The source port and settings overlay come from elliotttate's
`windows-release` commits `4b01c6b` and `4fbcc7f`; the original Git author is retained for the port.
The fork's initial PC measurements are recorded in its
[Windows port documentation](https://github.com/elliotttate/Wind-Waker-Recomp/blob/4b01c6bfd7e80e6c44880052d808c6e6b0ab7b39/docs/WINDOWS.md).
They are the contributor's results, not independent Windows gameplay evidence for this integration.

On October 1, the Win32 compatibility layer, entry shim, settings overlay and shared C host sources
were checked with a native macOS LLVM-MinGW cross-compiler. The source-only runtime/app also compiled
and linked on Windows with clang, the MSVC toolchain and Dawn in
[CI run 36799881615](https://github.com/chrissotraidis/bluewake/actions/runs/36799881615), including the
BlueWake settings theme and save-state sources. Neither check is Direct3D gameplay, a complete
personal-module build or controller/audio acceptance. Parallels is not needed for these checks.

The October 1 stability follow-up also passes native app linking and the 16-reader environment-cache
regression at `60be199` in [CI run 36805880425](https://github.com/chrissotraidis/bluewake/actions/runs/36805880425).
The source-only workflow opts into `BLUEWAKE_WINDOWS_REGRESSION_TESTS`; native app linking and both
the cache and shared FPS classifier/worker-counter checks pass at `830bf7d` in
[CI run 36808815002](https://github.com/chrissotraidis/bluewake/actions/runs/36808815002).
The pinned runtime's existing interpolation matching/blending/pacing suite also passes natively,
with the full app link and the other two tests, at `ec6e797` in
[CI run 36810248709](https://github.com/chrissotraidis/bluewake/actions/runs/36810248709).
The subsequent monotonic-clock initialization fix, its concurrent-clock regression, the other
three tests and the full app link also pass natively at `74da8ee` in
[CI run 36810799124](https://github.com/chrissotraidis/bluewake/actions/runs/36810799124).
These checks do not imply Windows gameplay or audio acceptance.

The current source qualification (`6b64f45`, runtime `0568fedd`, merged in #38)
builds the native Windows host and passes all 58 runtime regressions plus 24
builder training/cache checks in
[CI run 37071417006](https://github.com/chrissotraidis/bluewake/actions/runs/37071417006).
This includes the new PE/runtime and I8/RGB565 texture-encoding checks, and the
actual host save-failure fixture. The earlier `3392854` baseline passed 56 tests;
the newer host adds the Pictobox fix qualified on Mac in the reconciliation ledger.
This covers app linking and the registered regression suites, not a complete
player-owned-disc build, Direct3D gameplay, audio or physical controllers.

Remaining native acceptance needs an x86-64 Windows PC with a Direct3D 12 GPU.
A Windows ARM64 VM running x64 applications can provide separately labelled
compatibility evidence, but cannot establish native x64 performance parity.
Use the [Windows testing handoff](../archive/status/WINDOWS_ACCEPTANCE_2026-10-04.md) for the current build,
gameplay, save-preservation and donor-comparison checklist and results template.

The candidate includes prepaid blocks, fixed CPU/RAM storage, inline floating
point, gather helpers, direct calls and certified native replacements as explicit
builder options. Their bounded Mac evidence is in the
[reconciliation ledger](FORK_RECONCILIATION_2026-10-02.md); it does not
establish Windows performance. The Windows builder now carries Elliott’s local PGO training route: it builds
an instrumented module, plays the opening to control in isolated plain/modded
runs, validates executed translated functions, and uses the local profile for O2.
Source contract checks cover the wiring; a complete Windows disc-to-playback run
remains unverified. Mac training receipts do not validate that Windows path. Full clean Windows builds, matched performance and gameplay remain open.

