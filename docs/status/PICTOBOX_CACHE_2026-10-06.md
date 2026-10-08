# Pictobox cache writeback investigation

October 6, 2026. Issue [#13](https://github.com/chrissotraidis/bluewake/issues/13).

## Result

The stale second-photo preview is reproduced on the M3 Max Mac with Metal. The
opt-in `BLUEWAKE_CACHE_FLUSH_FALLBACK=1` candidate repairs it in a matched run.
This is shared host code; RecompCore is unchanged. The option is off by default
until iPad and Windows gameplay checks. It is not in release 0.5.0.
The earlier shutter freeze and the reported inability to select Save with the
left gamepad stick are separate acceptance questions. Do not close #13.

## Cause and narrow change

C-generated guest `dcbst` and `dcbf` instructions call the host instruction fallback.
The host previously advanced the PC without delivering those operations to its
cache observer. LLVM-generated operations already use `ppc_cache_control`.
The new helper forwards only these two writeback operations through that existing
callback, with the PowerPC rA=0 addressing rule and existing exception handling.
The host observer marks the affected 32-byte line dirty. No per-draw full-texture
hashing or blanket cache flush is added; other fallback behavior is unchanged.

Local generated instructions at 0x80303180 (`dcbst 0,r3`) and 0x803030F4
(`dcbf 0,r3`) demonstrate the skipped fallback path. For corroboration, the
[matching decompilation's Pictobox implementation](https://github.com/zeldaret/tww/blob/main/src/d/d_picture_box.cpp)
copies captured texture bytes and calls `DCStoreRangeNoSync` for the US version.
The code change relies on instruction semantics, not on copying that implementation.

## Matched reproduction

- Apple M3 Max, native macOS/Metal, 960x720 requested window; frame capture 1920x1440.
- Shared host based on `a14f14d` plus this candidate; RecompCore `d5b92fbb7d1d2ccd05d72d448e512dc06120efc9`.
- Personal translated module and copied Outset card/SRAM. A private checkpoint
  grants and equips the regular Pictobox in guest RAM; original saves are untouched.
- Original textures, no mods, Smooth Motion off, live controller input disabled.
- Load checkpoint at retrace 4101. X at 4200; A at 4550; cancel at 5150;
  exit camera at 5350; turn at 5500; X at 5750; A at 6100. Stop at 6650.
- Capture previews at retraces 5000 and 6501; `DOL_GXCORE_TEX_VERIFY=1` in both runs.
- Same candidate executable in the off/on comparison: SHA-256
  `cf41a5422f67023597329c50d6e5c15e5cad036071c404c4766cadbffef2c44f`.

Both runs write two I8 EFB readbacks to 0x00AA1FC0, 15808 bytes each. With the option
off, the second preview still shows the first bridge/house view. The verifier reports
changed CMPR bytes at 0x00A8B6A0, 152x104, 7904 bytes, while the dirty epoch remains 1.
With the option on, the second preview shows the new ladder/sea view and no
`[tex-stale]` warning occurs. Both runs exit normally after about 43 seconds.

Cache totals: off 190 hashed lookups / 190 uploads; on 215 hashed lookups / 193 uploads,
with approximately 9.14 million cache hits in each. This supports that the candidate
avoids blanket rehashing. These short cold-cache runs are not a performance benchmark.

The initial test harness accidentally retained fixed title-button pulses that masked
later scripted buttons. Those pulses were removed before the matched comparison;
only the runs with two confirmed readbacks are acceptance evidence.
Private screenshots, checkpoints, card copies, binaries and logs stay local.

## Checks and remaining gates

- Mac host compiles; focused cache fallback contract checks cover opcode filtering,
  rA=0, indexed addressing, PC advancement, preserved registers and dirty-epoch aliasing.
- [PR #146](https://github.com/chrissotraidis/bluewake/pull/146) merged as `fd93225`.
  Repository audit and attribution passed; Windows host CI passed 60/60 tests, including
  the cache-writeback contract. This is not a Windows gameplay result.
- One follow-up Mac run supplied left-stick X=-80 for 30 retraces at 6508. The capture
  at 6801 selected Yes. This verifies scripted menu input, not a physical controller.
  The saved off/on captures also retain the question text and surrounding menu.
- iPad and Windows gameplay are untested for this candidate. The earlier iPad audio
  candidate remains separate and has not been replaced by this Pictobox build.
- Next: two distinct regular and Deluxe photos; save/reload one photo; separately
  test gamepad Yes/No selection. Check the diagnostic startup line says
  `[texture-cache] fallback writeback=on (experimental)`.
- Do not infer that flickering, HD shading, missing flags or other texture reports
  share this cause. Reuse this evidence only if their own capture proves stale data.
