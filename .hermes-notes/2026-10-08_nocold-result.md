# --no-cold A/B result (PERFORMANCE.md step 3) — DONE, negative result

## Build
- --no-cold build at build/linux-nocold (no_cold=true, composite_digest 54f54434
  same as default; only optimization changed). 440 cold chunks in default
  (103 main-DOL + 337 actor RELs), confirming cold code = combat/actor/cutscene.

## Measurement (5900X, headless, unpaced, wall time over 2000-retrace window)
Bird scene (sea/44 event=2) via Deck state quick-2082.bwstate (retrace 2082,
mods 0x2). Loads clean (fields=151 missing=0 mismatched=0). Deterministic:
byte-identical guest checkpoints across all runs (unlike title demo).

- default run1: 26.27s, 1,458,633 blocks
- default run2: 26.31s (noise ~0.15%)
- --no-cold:    26.86s, 1,458,633 blocks (same blocks, byte-identical ckpts)
=> --no-cold 2.2% SLOWER on the bird scene, well outside noise. Identical
   behavior. Cold-code hypothesis: NEGATIVE for the bird scene — its hot code
   is trained or -O1/size-opt is not the limiter here.

## Title demo (trained scene, non-deterministic) — also neutral/negative
- default 18.63-18.99s vs --no-cold 19.52s (~3% slower), as Chris predicted.

## Verdict (written to docs/PERFORMANCE.md Results)
--no-cold does not help the bird scene on Linux. The 30% instruction gap is
from lean block copies (lever 1), not cold code (lever 2). --no-cold should
not be made default. Combat/boss scenes still untested (no savestate), but
the bird scene — the headline cold candidate — shows no benefit.

## Key measurement gotchas learned
1. BLUEWAKE_MAX_RETRACES must EXCEED the loaded state's retrace count or the
   run stops after 0 blocks (loop is `g_host_retrace_count < max_retraces`).
2. Title demo is NON-deterministic (cpu/mem1 hashes vary run-to-run); bird
   scene from the state IS deterministic (byte-identical checkpoints). Use the
   bird scene for A/B gating.
3. BLUEWAKE_MODS env didn't override the settings.ini mod mask (stayed 0x3);
   the state's mods-0x2 warning is cosmetic, the run still matched.
