# PLAYER closure subsystem review — 2026-09-06

Base `8fb2f50`. BW-P4-0126 remains active. This is the mandatory anti-stall
review, not a gameplay milestone or new green implementation checkpoint.

Follow-on: `ROUTE_B_ANIMATION_AUDIO_STATE_2026-09-06.md` records the later
verified shared-service composition and real BAS/state integration. The WIP
labels below describe this review's original experimental state.

## Findings that change the implementation plan

The 55-symbol phase-three closure at the base consists of 30 audio and 25
other actor/runtime symbols. Treating each unresolved name as a missing whole
actor was wrong. The original game already separates shared actor services
into main-DOL static units. Bomb parameters, item state/visibility, ship
positioning and weapon data live there, independently of the corresponding
REL actor implementations. Preserve that original boundary.

Twenty-three full source units were syntax-probed with the native prelude,
GZLE01 asset includes, native iterator shim, MSL fallback, and errors enabled
for pointer truncation, integer narrowing and missing returns. Fourteen
compile and nine fail. Compilation is not behavioral completeness.

| Owner group | Measured result |
| --- | --- |
| JAIZelInst | Full unit compiles; conducting methods have bodies, but audio backend behavior is not qualified. |
| JAIZelAnime | Compiles despite empty startAnimSound, setSpeedModifySound and setPlayPosition bodies. Cannot be admitted as working audio. |
| JAIZelBasic | Missing returns in seStart/getLinkVoiceVowel and missing WaveBankMgr declaration; Link voice start is also empty. Requires reconstruction plus audio-owner work. |
| m_Do_audio | Five heap API compile failures; original global audio objects also impose constructor/lifetime requirements. |
| Gameover, itembase, salvage actor, boko actor, arrow, JPA manager, counter | Full units compile. This alone does not justify actor/profile or audio/particle admission. |
| Timer | Three OSTime-to-s32 narrowing failures. |
| Rope/himo2 | Three opaque-pointer actor-name API failures; require canonical-pointer review. |
| Boomerang | Missing private generated material display-list header. |
| Bomb REL | Missing generated d_a_bomb3.inc; its needed parameter helper is in the separate static unit. |
| Ship REL | Seven narrowing/pointer failures; its needed initial-position helper is in the separate static unit. |
| Sarace REL | Two declaration/narrowing failures; the required shared result field lives in d_com_static instead. |
| Boko/bomb/itembase/ship static units and d_salvage | All five compile unchanged. |
| d_com_static | One actor-pointer API failure in the stand-item model callback; the model user-area producer still requires native pointer qualification. |

## Grouped implementation experiment — work in progress

The five compiling shared actor-service units were added together to PLAYER's
original service composition. Strict phase-two/three linkage now retains 42
symbols: 30 audio and 12 other actor/runtime. Strict actual Link phase two and
event identity still pass. No phase-three execution occurred. This grouped
change is uncommitted and has not run the complete three-mode regression
matrix. Do not describe it as a qualified new checkpoint or infer that all
functions in the units were exercised.

Remaining non-audio owners include d_com_static shared fields, gameover/timer,
rope/boomerang, arrow, JPA emitter creation and the original counter global.
Some already have admitted fragments; avoid duplicate globals and callbacks
when replacing them with full source owners.

## First missing audio behavior: independent binary evidence

The pinned source's JAIZelAnime::setPlayPosition is empty. Its symbol range is
0x802ACFA0, length 0x68, in the verified private main DOL (SHA-1
8d28bab68bb5078c38e43f29206f0bd01f7e7a67).

A private Capstone 5.0.6 disassembly identifies an early return for null
animation data, an entry-count-bounded scan of start-frame values, and writes
to the data counter/current-time fields. Capstone leaves one word undecoded;
the pinned DolRecomp decoder identifies opcode 63/XO 32 as ordered floating
compare, with CR field 0 and operands f0/f1. The following condition combines
greater/equal; unordered values therefore must not be treated as an ordinary
less-than inverse. Both direction branches in this range write the same
counter and time. This is reconstruction evidence, not a completed method.

The native JAIAnimeSoundData header contains a host pointer and therefore
cannot alias the serialized BAS header. Any reconstruction must be paired
with a clear native BAS ownership/decoding contract. Do not cast raw BAS bytes
to the host header or certify sound emission based on frame-state tests.
The existing JAI animation construction-only composition is not full animation
audio initialization or playback.

## Selected integrated plan and acceptance endpoint

1. Continue the original shared actor-service group, resolving actual native
   pointer/asset owners and replacing existing fragments without duplicates.
   Do not port full actor RELs merely to get a shared main-DOL helper.
2. Establish the animation-audio state/data owner: reconstruct setPlayPosition
   against the verified DOL, qualify BAS endian/layout/lifetime and original
   initialization interactions, and explicitly identify remaining emission
   methods. Empty upstream bodies cannot be linked as successful behavior.
3. Address remaining Link voice/BGM/conducting dependencies as an audio
   subsystem, distinguishing complete original source from reconstruction and
   preserving explicit fail-on-use diagnostic boundaries. Do not add dozens
   of silent or generic successful stubs to force linkage.
4. Execute actual phase_3/makeBgWait with real Room44 collision, observable
   initial procedure/model state and unchanged parameters, then original
   PLAYER execute. Do not skip phase three or inject a procedure. Continue to
   camera/input/draw integration once that initialization is proved.

The next implementation action is the animation-audio data/state owner, while
preserving the grouped shared-service work already in the worktree. Further
supporting tests are evidence for this endpoint, not substitutes for it. A
newly measured architectural failure must trigger another review, not repeated
one-symbol checkpoint loops. The full PRD remains authoritative and unchanged.

Evidence: ignored evidence/route-b-player-owner-review-20260906, including
per-unit compile logs, grouped link/init logs and private disassembly. No GUI,
Simulator or user save was touched. No protected source/dependency pin changed.
No full regression/push claim for the experimental group; no external blocker.
