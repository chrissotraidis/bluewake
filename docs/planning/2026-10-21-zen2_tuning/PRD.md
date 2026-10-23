# PRD — Steam Deck Zen 2 tuning decision

Status: Rejected / parked
Parent milestone: [PR #421, M6](https://github.com/chrissotraidis/bluewake/pull/421)
Decision source: [PR #438 measurement report](https://github.com/chrissotraidis/bluewake/pull/438#issuecomment-6357949110)

## Question

Does adding module-only `-mtune=znver2` improve the Steam Deck build while
retaining the portable `-march=x86-64-v3` ISA target?

## Measured result

Two builds from source `56a9537` used identical options except for module-only
`-mtune=znver2`. Four interleaved headless, unpaced rounds covered five fixed
scenes on Steam Deck. The median change was **+0.0%** overall.

| Scene | Untuned median FPS | Tuned median FPS | Change |
|---|---:|---:|---:|
| Outset | 42.3 | 42.0 | -0.7% |
| Windfall | 42.9 | 43.0 | +0.3% |
| Dragon Roost | 56.1 | 56.3 | +0.4% |
| Forest Haven | 38.6 | 38.6 | -0.2% |
| Sea by Fortress | 49.4 | 49.5 | +0.1% |

## Decision

Do not implement a `--mtune` option or change a portable default for this
result. This decision is limited to this compiler flag and workload; it does
not rule out other compiler, scheduling, or host-side experiments. Private
discs, modules, AppImages, states, and raw captures remain off-repository.
