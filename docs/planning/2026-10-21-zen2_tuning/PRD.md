# PRD — Steam Deck Zen 2 tuning

Status: Draft  
Source: upstream `main` at `2ab8118fc09fa08569100201a871616d18a1b771`  
Parent milestone: [PR #421, M6](https://github.com/chrissotraidis/bluewake/pull/421)

## Motivation

Steam Deck uses a Zen 2 CPU while BlueWake's Linux build targets the portable
`x86-64-v3` ISA without a microarchitecture scheduling hint. This plan tests
whether `-mtune=znver2` improves the game-thread-limited workload while keeping
the same instruction-set requirement and a portable default build.

## Scope

- Add an explicit, recorded Linux builder option for `-mtune=znver2`.
- Apply the option consistently to the host and translated game module while
  retaining `-march=x86-64-v3`.
- Compare tuned and untuned AppImages with the compiler retained by M5.
- Keep the option disabled by default unless evidence extends beyond Deck.

Excluded: `-march=znver2`, Steam Deck-only binaries as the general release,
gameplay changes, private artifact publication, or combining other candidates
before this option is measured independently.

## Outcomes

- Provenance and PGO fingerprints distinguish tuned and untuned artifacts.
- Effective app/module compiler commands prove `-march=x86-64-v3` is unchanged
  and `-mtune=znver2` is the only experimental flag.
- Both variants pass correctness, Linux, AppImage ABI and launch checks.
- Steam Deck A/A and interleaved A/B results support a retain, reject, or
  inconclusive decision based on measured noise rather than a 30 FPS threshold.

## Success and constraints

| Metric | Baseline | Decision rule | Evidence |
|---|---|---|---|
| Game-thread throughput | Retained M5 compiler, no `-mtune` | Tuned median improves beyond A/A noise across target scenes | `bench_tour.py compare` summaries |
| Game Mode pacing | Same untuned artifact/configuration | Direction agrees with controlled runs; report FPS, busy cause and waits | Session-log summary |
| Correctness | Untuned checkpoints | All compared checkpoints match | Benchmark output |
| Build identity | One frozen M5 control | Only `-mtune=znver2` and derived hashes differ | Provenance/effective commands |
| Portability | `-march=x86-64-v3` default | ISA target and generic default remain unchanged | Tests and documentation |

M6 starts after M5 selects or retains a compiler. It also depends on the Linux
container blocker [#433](https://github.com/chrissotraidis/bluewake/issues/433).
Private discs, modules, AppImages, states, and raw performance captures remain
off the repository.
