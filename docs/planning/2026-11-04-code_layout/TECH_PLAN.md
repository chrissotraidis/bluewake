# TECH_PLAN — Code layout of the game module

Status: M1 implemented, opt-in; measurement next.
[Product brief](PRD.md) · [Tracker](PROGRESS.md)

## M1: split the cold blocks, keep the hot sections together

**The change:** `scripts/linux/build.py --split-functions` adds, only to the final module compile that uses the training profile:
- **`-fsplit-machine-functions`:** blocks the profile never saw run move to `.text.split.<function>`, as a `<function>.cold` part.
- **`-Wl,-z,keep-text-section-prefix`:** lld keeps `.text.hot`, `.text.unlikely` and `.text.split` as separate output sections, so the hot parts of every chunk pack together.

**Training is unchanged:** the instrumented compile doesn't get the flags, so the training fingerprint stays the same.

**Checked on one chunk:** the hottest J3D chunk (`func_802ED6E0`), compiled with the build's own command plus the flag:

| | Size |
|---|---:|
| Before | 844 KB in one piece |
| After, hot part | 553 KB |
| After, `func_802ED6E0.cold` | 296 KB |

The inline helpers' copies split the same way.

| File | Change |
|---|---|
| [`scripts/linux/build.py`](../../../scripts/linux/build.py) | `--split-functions` (off by default; needs a training profile); provenance |

**Verification:**
- **Correctness:** guest checkpoints identical to the control, since only placement changes.
- **Speed:** render_bench on the Steam Deck, five save states, at least six interleaved launches each.
- **The mechanism:** the desktop's instruction-TLB and op-cache counters, as a check that the effect is the one intended.

## M2 (after M1): the hot text on huge pages

At module load, copy the module's `.text.hot` range into anonymous memory with `MADV_HUGEPAGE` and `mremap` it over the original mapping. The range must be 2 MB-aligned. SteamOS runs transparent huge pages in `always` mode. This is planned once M1 has a result.
