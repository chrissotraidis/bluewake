# TECH_PLAN — Code layout of the game module

Status: **Done.** M1 (split functions) no-go, reverted; **M2 (huge pages) kept, opt-in: +4.47% on the Steam Deck.** The switch is Config → Performance → *Game code on large memory pages* (#531, `06715f0`).
[Product brief](PRD.md) · [Tracker](PROGRESS.md)

## M1: split the cold blocks, keep the hot sections together (no-go, reverted)

> **Outcome:** desktop −2.5%, and iTLB page walks rose 12.6%. The split follows the training profile, and blocks Outset runs that training didn't are now 140 MB away. The code stays in history as `170e394c4`.

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

## M2: the module's code on huge pages (host-only, opt-in)

`runtime/host/src/module_hugepages.h`, called right after `dlopen` and before any of the module's code runs, when `BLUEWAKE_MODULE_HUGEPAGES=1`:
1. **Find the mapping:** the module's executable mapping, in `/proc/self/maps` by the file's real path. That avoids the GNU `dl` extensions, which `main.c`'s include order rules out.
2. **Copy it:** its 2 MB-aligned middle, into a temporary anonymous buffer.
3. **Replace it:** map anonymous memory over the same range (`MAP_FIXED`), advise `MADV_HUGEPAGE`, copy the bytes back, and make the range read-execute again.
4. **Report:** `[module] huge pages: N MB of code copied, M MB on huge pages`. The second figure is `AnonHugePages` from `/proc/self/smaps`.

**Exactness:** the same bytes at the same addresses. Relocations are already applied (`RTLD_NOW`), and the code is position-independent.

**Without transparent huge pages** (this desktop's kernel), `madvise` fails, the range stays on 4 KB anonymous pages, and nothing else changes.

**Costs:**
- **Memory:** the copied range is resident anonymous memory, about 400 MB, instead of page cache.
- **Profiling:** `perf` no longer maps those addresses to the module file.

That's one reason it's off by default.

**Testing:** it's host-only, so a host-only rebuild over the control (`build_script --reuse`) gives a candidate with the control's exact module.
