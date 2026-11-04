# PRD — Code layout of the game module

Status: In progress. M1 (split the cold blocks, keep the hot sections together) is implemented and opt-in; the measurement build is next.
Source: BlueWake `main` at `ee991d312`. Measured on the [#437](https://github.com/chrissotraidis/bluewake/pull/437) recipe (`perf/clang-runtime-matrix@14c558d`, Clang 22.1.8, libc++).
Scope: where the compiler and linker place the game module's code. Nothing the code does changes.
[Technical plan](TECH_PLAN.md) · [Tracker](PROGRESS.md)

## Problem

The game thread is short of instruction fetch, even on a Zen 3 desktop with 96 MB of L3. Outset, headless, the #437 build, per 1,000 instructions:

| Counter | Game module | Note |
|---|---:|---|
| L1 instruction-TLB misses that hit L2 | 2.0 | |
| Instruction-TLB misses that walk the page table | 0.38 | 48 M per 600 frames, against 1.5 M on the data side |
| Op-cache hit rate | 58% | |

The cause is how the hot code is spread out:
- **The chunk functions:** each chunk is one function of about 800 KB, holding the blocks' prepaid copies, the original blocks they fall back to, and every fallback's call. The hot instructions are spread through all of it.
- **The shares:**

  | Share of chunk time | Chunks | Machine code | Address span |
  |---|---:|---:|---:|
  | 80% | 34 | 29.5 MB | 102 MB |

- **The build:** PGO, but no `-fsplit-machine-functions`, and the link doesn't keep the compiler's `.text.hot` sections together.
- **Huge pages:** on the Steam Deck the module comes from the AppImage's FUSE mount, so it can't get file-backed huge pages.

## Goal

The same code with the hot parts packed together, then, if that's measurable, the hot part on huge pages.

## Steps

| Step | Change |
|---|---|
| M1 | ~~`-fsplit-machine-functions` with the training profile~~: no-go, 2.5% slower on the desktop, iTLB walks +12.6%; reverted |
| M2 | The module's executable segment copied into 2 MB pages at load, without splitting (SteamOS runs transparent huge pages in `always` mode); host-only, opt-in `BLUEWAKE_MODULE_HUGEPAGES=1` |

## Success and constraints

| Metric | Target | Validation |
|---|---|---|
| Correctness | Nothing semantic changes | Guest checkpoints identical to the control |
| Outset game FPS | Ahead of the control in every interleaved launch (at least six); no minimum size | render_bench, five save states, on the Steam Deck |
| Risk to players | None by default | Opt-in builder option `--split-functions` |
