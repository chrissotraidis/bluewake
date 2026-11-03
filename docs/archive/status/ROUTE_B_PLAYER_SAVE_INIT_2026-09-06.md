# Original save initialization and two PLAYER updates — 2026-09-06

Base `a7d00f1`; patches 0193–0194 and original collision composition. BW-P4-0126 remains active.

## Integrated result

Strict ASan/UBSan completes original `dSv_info_c::init`, Link phase two,
real-ground phase three and two original Link updates, each followed by
original `dCcS::Move`. No registration counts are reset by the diagnostic.
The pending Room0 request stays singular, but is not progressed. Ground is
550 in room 44; Link starts unequipped with life/max-life 12 rather than the
prior raw-zero save. This is in-memory initialization, not CARD loading,
file selection, normal spawning, normal boot or continuous gameplay.

## Evidence changed the implementation

The previous second update entered `changeDeadProc`, then death voice 22,
because raw-zero save state had no health. The voice fences remain intact.
Original complete save initialization now runs before phase two, with the
actual JAIBasic/JAIZelBasic constructors moved earlier in the diagnostic
lifetime. It includes player status/config, saved memory, ocean/event,
temporary memory, dungeon/zone and temporary-event initialization.

0193 shares original game output-mode and main/sub-BGM identity methods,
driver/stream output-mode storage and original race/high-score data. Full
JAIGlobalParameter compiles. Full driver/stream compilation first exposed
unrelated DSP integer-pointer and stream pointer-width errors; only the
unchanged output-mode owners are composed from those two units. DSP and
streamed playback remain unqualified. macOS has no console SRAM sound-mode
setting; the host reports a uniquely logged stereo default. All three output
modes and rejected game-interface mode 3 are checked against actual stored
driver/stream/interface state. No backend observer substitutes for setters.

Original save init exposed native array UB. GZLE01 DOL 0x800589A8–0x80058B54
contains a five-iteration selected-item loop, writing game-info+0x5BD3+i.
The fifth play-state byte is the first equipment byte (play+0x4937), beyond
the declared four-element selected-item array. 0194 preserves that retail
alias explicitly for native index 4 in both accessors. It does not enlarge
the layout or shorten original initialization. All 256 byte values, reverse
equipment-to-item aliasing and adjacent-byte preservation are checked.
Control-test item/equipment bytes are restored before original initialization;
the affected private targets were rebuilt and replayed in all three modes
after this final isolation fix.
Original initial event registers are checked at high score 20 and value 14.

Healthy initial state then reached `checkPlayingMainBgmFlag` during normal
face selection. The unchanged original query replaces its diagnostic fence;
public controls check live main/sub sound IDs independently and null results.

Second update next reached the original ground-path query. Full `d_path.cpp`
now composes unchanged. LLDB at that actual call reports room 44/path 255;
the real room archive has 40 RPAT paths and 271 points. An independent ground
query checks the same no-path sentinel and height. Room path tables are not
decoded/published yet; valid-path traversal is unqualified, not proved by
the sentinel rejection. This distinction must remain for later movement.

## Verification state

- Focused replay passes two updates/Move and the new state controls in
  Debug, Release and strict ASan/UBSan (`detect_leaks=0:halt_on_error=1`,
  `UBSAN_OPTIONS=halt_on_error=1`).
- Public CTest passes 82/82 in all modes. Rebuilt private regressions pass:
  phase three, SE registration, animation playback, player init/event identity/
  model, camera run/event/constructor/matrix/mass, sea, attention, DZB,
  toripost heap, morph audio, BG profile, stone2 and room lifecycle.
- Parameter/emission oracles pass 11,076/12,880 cases per mode. Production
  creation/phase-three/runtime censuses remain four/41/52 missing symbols,
  with expected build exit 2 in each mode. No full-profile admission implied.
- Both preparers pass; protected recompcore and lock hashes are unchanged.
  Audit output is byte-identical to the known 20 tracked evidence-file
  failure. Python player-asset tests pass 10/10.
- Archived logs: ignored `local-research/evidence/route-b-player-save-init-20260906`.
  Working logs are ignored `/tmp/bluewake-player-save-*`; initial collision
  history remains `/tmp/bluewake-player-collision-*`. Actual path breakpoint
  evidence is `/tmp/bluewake-player-path-lldb.log`.
- No app/Simulator launched or save file touched. No external blocker.

## Next action

Connect actual input via the original JUTGamePad
and mDoCPd path, progress scene requests and compose PLAYER camera/draw toward
a sustained controllable session. Aurora already exposes virtual PAD state
for deterministic input replay; that can test the real poll path without
injecting Link stick fields. GBA/reset/rumble owners must be qualified or
explicitly fenced as appropriate, not silently skipped. Normal boot and all
remaining PRD requirements remain required.
