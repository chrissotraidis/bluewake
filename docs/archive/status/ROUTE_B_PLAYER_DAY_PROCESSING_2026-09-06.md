# Original PLAYER day-processing dependency — 2026-09-06

Base `b7526f0`; active blocker BW-P4-0126. No P4 promotion.

The original turn-restart path retains dKy_DayProc, whose original day routine
requires letter delivery and automatic stocking. Full original d_letter.cpp
now composes unchanged with PLAYER; no dependency patch or substitute behavior
is required.

The private PLAYER init probe executes all four letter states, both delivery
branches for each state, all three state-setting operations, and verifies that
adjacent event bits remain intact. It also calls original dKy_DayProc twice
with explicit synthetic story flags: eligible Aryll/Tingle letters are stocked,
a sent Baito letter is delivered, Grandma's read letter stays read, counters
increment/saturate, and representative daily, weekly and temporary flags reset.
Event bytes, temporary flags and date are restored before actual Link phase two.
This is not an exhaustive story/mail campaign or disk-save test.

The first test attempted dSv_event_c::init(), exposing an unresolved original
setInitEventBit owner. The fixture now value-initializes its synthetic event
state explicitly; normal new-game initialization remains unqualified. This
test change is not a product initialization workaround.

Actual Link phase two and identity, the new letter/day checks, and the existing
PLAYER model probe pass Debug, Release and strict ASan/UBSan. All 74 public
tests pass in all three modes. The unchanged accumulated private camera, sea,
attention, collision, Toripost heap, morph, BG, Stone2 and room-lifecycle probes
were replayed in all modes. No GUI/Simulator or live user save was used.
Production phase-two, phase-two/three and phase-two/execute censuses retain
four/56/69 unresolved symbols in every mode (intentional linker exit 2).
Counts measure static dependencies, not executed calls or completion percent.

TWW and Aurora preparation checks pass. Asset Python tests pass 10/10. Diff
whitespace checks pass; both protected source hashes and lock hash are
unchanged. Audit retains only the known 20 tracked research files. Evidence
logs are private under ignored evidence/route-b-player-day-processing-20260906.

Next measured frontier: original JStudio stb-data-parse.cpp, required by Link's
demo-data handling, fails native syntax compilation at three pointer-to-int
casts (lines 25, 30, 46 at this pin/patch state). Its sequence header and the
JGadget variable-integer decoder also read multi-byte disk values in host
order. Repair pointer arithmetic and endian/alignment ownership together,
using independently encoded parser cases, rather than merely casting through
a wider integer. The byte-oriented paragraph-data routine itself has source.

Audio remains substantive unfinished work: Link voice methods in the pinned
JAIZelBasic source have empty nonmatching bodies. Diagnostic audio still
aborts on use; those bodies must not be admitted as successful behavior.
Actual phase three, full PLAYER admission/delete, continuous updates, draw,
input, normal boot and P4 remain unqualified. No external blocker.
