# PROGRESS — Code layout of the game module

Status: **M2, the module's code on huge pages: +4.47% on the Steam Deck**, every scene faster, every launch ahead, checkpoints identical (host-only, opt-in `BLUEWAKE_MODULE_HUGEPAGES=1`). M1 (split functions) was a no-go and is reverted.
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

Acceptance: ahead of the control in every interleaved launch (at least six), checkpoints identical. No minimum gain.

## Milestones

### M1: Split the cold blocks, keep the hot sections together

- [x] M1-T01: Measure the instruction side (#437 build, Outset, desktop): 0.38 page walks and 2.0 L2 iTLB hits per 1,000 instructions, 58% op-cache hit rate; 80% of chunk time in 29.5 MB of code spread over 102 MB.
- [x] M1-T02: `--split-functions` in the Linux builder; checked on the hottest J3D chunk: 553 KB hot, 296 KB split out.
- [x] M1-T03: Build on the #437 recipe with `--split-functions` (`.text.hot` 61.8 MB, `.text.split` 139.5 MB, `.text.unlikely` 168.9 MB): desktop **−2.5%** (cycles per frame +2.7%), iTLB page walks **+12.6%**, checkpoints identical. The split follows the training profile, and blocks Outset runs that training didn't are now 140 MB away.
- [x] M1-T05: **No-go:** reverted (`170e394c4` keeps the code). The Deck run was skipped: a 2.5% loss with a worse mechanism won't turn into a gain there.

### M2: The module's code on huge pages

- [x] M2-T01: `module_hugepages.h`, called after `dlopen` behind `BLUEWAKE_MODULE_HUGEPAGES=1`; the host compiles cleanly (`-Wall -Wextra`) in the build container.
- [x] M2-T02: Host-only build over the control (`bench/huge` `31c47882ae1a`, `host_only: true`, module SHA-256 `3d3919e9a8f9` = the control's). On the desktop: `[module] huge pages: 372 MB of code copied, 0 MB on huge pages` (no THP in this kernel), checkpoints identical to the control's.
- [x] M2-T03: Steam Deck render_bench (THP `always`), five save states: **+4.47% overall**, every huge-page run (39.79–40.27) ahead of every control run (38.12–38.59); the Deck logs `372 MB of code copied, 372 MB on huge pages`; checkpoints identical.
- [ ] M2-T04: Decide: propose as the default on Linux, with a Config → Performance switch (#531's section).

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
| M1-T01 | #437 build `clang22` (`14c558d`) | `perf stat -e bp_l1_tlb_miss_l2_tlb_hit,bp_l1_tlb_miss_l2_tlb_miss,op_cache_hit_miss.*,ls_l1_d_tlb_miss.*`, headless, 1,200 retraces from `1-outset.bwstate`; `perf record` with chunk sizes and addresses from `nm` | Desktop PC (Ryzen 7 5800X3D) | 127.17 G instructions, 47.03 G cycles: 259.0 M L2 iTLB hits, 48.3 M iTLB page walks, op cache 9.78 G hits / 6.99 G misses; data TLB 88.3 M L2 hits, 1.47 M walks. 50% / 80% / 95% of chunk time in 12 / 34 / 71 chunks, 9.4 / 29.5 / 56.9 MB, spread over 47 / 102 / 167 MB |
| M1-T02 | This branch | The J3D chunk's command from the build's `build.ninja` plus `-fsplit-machine-functions`, in `bluewake-linux-ci:local`; `readelf -S` | Desktop PC | `.text.hot.func_802ED6E0` 0x87262 bytes, `.text.split.func_802ED6E0` 0x48575 bytes (844,418 before) |
| M1-T03 | Control `clang22` (A) against `layout554` (B, `bench/layout` `3e3a963cac36`, `split_functions: true`); local `measure-554/` (private) | `perf stat`, headless, 2,400 retraces from `1-outset.bwstate`, A B B A A B; then 1,200 retraces with iTLB and op-cache counters | Desktop PC | Per game frame: A 76.19 / 76.03 / 75.99 M cycles, B 78.16 / 78.05 / 78.05 M (**+2.7%**); instructions 210.35 → 210.82 M; rate 116.9–117.2 → 114.0–114.4 (**−2.5%**). Over 1,200 retraces: iTLB page walks 48.04 M → 54.11 M (**+12.6%**), L2 iTLB hits 257.6 M → 280.4 M (+8.9%), op cache 9.79 / 6.99 G → 10.33 / 7.07 G hits / misses. Checkpoints identical |
| M2-T02 | `clang22` `31c47882ae1a` (host-only over the control's build; same module `3d3919e9a8f9`) | `BLUEWAKE_MODULE_HUGEPAGES=1`, headless, 2,400 retraces from `1-outset.bwstate`, checkpoints every 300 | Desktop PC (no THP) | Loads and plays; 372 MB of code copied, 0 MB on huge pages; checkpoint hash `8d79fc3cda043b4e`, the control's |
| M2-T03 | Control `clang22` `14c558d62d1c` (A) against `clang22` `31c47882ae1a` (B, host-only; the same module); local `huge-554-deck/` (private) | `render_bench deck@… A B --states test_render/states --display desktop --pairs 3`, `--set BLUEWAKE_MODULE_HUGEPAGES=1` (both; A has no remap), render scale 3, Smooth Motion 1 step, widescreen, efficient soft effects | Steam Deck, Game Mode | **B +4.47% overall**: Outset +4.1%, Lava Room +5.1%, Hyrule Castle +3.3%, Butterfly Room +6.5%, Shipwreck +4.5%. Per run A 38.33 / 38.50 / 38.33 / 38.59 / 38.49 / 38.12, B 40.10 / 39.83 / 40.18 / 40.27 / 39.92 / 39.79. B's logs: `[module] huge pages: 372 MB of code copied, 372 MB on huge pages`. Guest checkpoints identical across every run |
