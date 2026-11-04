# PROGRESS — Code layout of the game module

Status: M1 implemented, opt-in; the measurement build is next.
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

Acceptance: ahead of the control in every interleaved launch (at least six), checkpoints identical. No minimum gain.

## Milestones

### M1: Split the cold blocks, keep the hot sections together

- [x] M1-T01: Measure the instruction side (#437 build, Outset, desktop): 0.38 page walks and 2.0 L2 iTLB hits per 1,000 instructions, 58% op-cache hit rate; 80% of chunk time in 29.5 MB of code spread over 102 MB.
- [x] M1-T02: `--split-functions` in the Linux builder; checked on the hottest J3D chunk: 553 KB hot, 296 KB split out.
- [ ] M1-T03: Build on the #437 recipe with `--split-functions`; desktop counters and first read.
- [ ] M1-T04: Steam Deck render_bench, five save states.
- [ ] M1-T05: Decide.

### M2: The hot text on huge pages

Planned after M1.

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
| M1-T01 | #437 build `clang22` (`14c558d`) | `perf stat -e bp_l1_tlb_miss_l2_tlb_hit,bp_l1_tlb_miss_l2_tlb_miss,op_cache_hit_miss.*,ls_l1_d_tlb_miss.*`, headless, 1,200 retraces from `1-outset.bwstate`; `perf record` with chunk sizes and addresses from `nm` | Desktop PC (Ryzen 7 5800X3D) | 127.17 G instructions, 47.03 G cycles: 259.0 M L2 iTLB hits, 48.3 M iTLB page walks, op cache 9.78 G hits / 6.99 G misses; data TLB 88.3 M L2 hits, 1.47 M walks. 50% / 80% / 95% of chunk time in 12 / 34 / 71 chunks, 9.4 / 29.5 / 56.9 MB, spread over 47 / 102 / 167 MB |
| M1-T02 | This branch | The J3D chunk's command from the build's `build.ninja` plus `-fsplit-machine-functions`, in `bluewake-linux-ci:local`; `readelf -S` | Desktop PC | `.text.hot.func_802ED6E0` 0x87262 bytes, `.text.split.func_802ED6E0` 0x48575 bytes (844,418 before) |
