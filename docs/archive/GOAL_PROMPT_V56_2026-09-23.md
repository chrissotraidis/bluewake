# Goal prompt v56 - the iPadOS loop (2026-09-23)

**USER-DIRECTED. Supersedes v55's ordering; v55's Route B campaign continues as a background track.**
The user asked for the game to actually work, on iPadOS, tested in the simulators one at a time,
with no hardware iPad yet. The dossier for this reorientation is
[status/IPADOS_REORIENTATION_2026-09-23.md](status/IPADOS_REORIENTATION_2026-09-23.md).

## Objective

A person can install BlueWake on an iPad, supply their own `GZLE01` data, and play Wind Waker
with touch or a controller: correct picture, correct sound, saves that persist, sensible behavior
when the app is backgrounded, at the game's authentic speed. Until hardware testing is authorized,
every claim is made on the iOS simulator and labeled as simulator evidence (PRD 14.6).

## The route

Route A hosted by `apple/ios` (see the dossier, section 1). The translated composite is unchanged
game code; the iOS host is the macOS host's sources plus an entry shim, touch controls and a DSP
shim. Route B work continues only when it does not block this loop.

## One iteration

1. Pick the top open item in the queue below.
2. Make the smallest change that moves it, in the owning layer: `runtime/host`, `apple/ios`, or
   `ref/recompcore` through a numbered patch registered in `config/dependencies.lock.json`.
3. Build: `cmake --build build/ios-sim --target BlueWake` (configure once as recorded in
   `apple/ios/README.md`).
4. Run exactly one game on the Mac: the simulator (`scripts/ios/sim_run.sh`), the macOS host, or
   the Dolphin reference, never two. `scripts/one_game_guard.sh` refuses to start a second one and
   is called by sim_run.sh and bench_instructions.sh; use it before any hand-run game too. Never
   time a run while a build is in progress, and never edit a script a running job is executing
   (bash reads scripts as it goes; an edit to sim_run.sh killed an acceptance run on 2026-09-23).
   Graphics work compares against a private Dolphin reference of the same moment, recorded on its
   own (PRD 14.4): `scripts/card_to_gci.py` turns a BlueWake save into a GCI for Dolphin's GCI
   folder.
5. Record the evidence (milestones, screenshots, counters) under `local-research/ipad/` and a dated
   entry at the top of [status/CURRENT.md](status/CURRENT.md). An iteration counts only if a
   screenshot, milestone or counter moved.
6. Every three iterations, report to the user.

## The queue, ordered by what stops a person from playing

**Re-ordered 2026-09-23 evening at the user's direction:** the picture comes first (it was visibly
wrong: palette textures, a 640x480 scene, flat shading), and the touch shell must be SunPad's, as
FR-015 requires. Items below keep their numbers for history; the working order is 7, 2, 6, then
the rest.

0. **DONE - saves and audio on the simulator.** `scripts/ios/sim_save_acceptance.sh` passes:
   save, the guest's quit, reload, and a 400 s capture of the game's own mix.
   DONE 2026-09-24 night: the mix was mono because SRAM was never emulated (every EXI access read
   0, so OSGetSoundMode said mono); runtime/host/src/ipl_sram.c serves Dolphin's default SRAM on
   the iOS app and the title audio now matches a Dolphin reference in stereo image and level.


1. **DONE - the play-scene picture freeze** (recompcore 0057). Still open inside it: a single
   rejected batch should be recoverable instead of sticky.
2. **Touch controls in play: the SunPad shell.** DONE (2026-09-23 evening): the ImGui overlay is
   replaced by `apple/ios/src/BWGameOverlay.mm`, adapted from SunPad (docs/archive/SUNPAD_TRANSFER.md):
   SunPad's controls and defaults, the three-dot menu, touch settings and the layout editor, with a
   pause-reason set that holds the guest (recompcore 0067). Done since: title to play and walking
   driven through the UIKit controls (BLUEWAKE_TOUCH_TAPS). DONE 2026-09-24: a player who takes the controls during a scripted
   run turns the script off (it had been pressing A under the stick: rolls). DONE 2026-09-24: a hardware
   keyboard walks Link on the simulator (recompcore 0083; the bound controller had discarded the
   keyboard's stick). DONE 2026-09-24 evening: the game runs on the iPhone 17 simulator and the phone
   defaults sit in the 4:3 letterbox bars instead of over the minimap and item HUD; controller
   defaults checked (face buttons by position, right shoulder Z, analog L/R, right stick C). Open:
   contextual Wind Waker mappings (sailing), the smallest and largest phones.
3. **Full-screen, correct aspect.** DONE as far as iPadOS 26 allows: aspect is fitted (recompcore
   0058); the app launches full screen in the default multitasking mode, and iPadOS 26 has no public
   API that forces it in windowed mode, where the 4:3 picture letterboxes (2026-09-24). Checked
   2026-09-24 evening: a floating window in portrait and a rotation to landscape full screen both
   resize the picture and the controls and keep running at 60.
4. **Data import on the device path.** DONE (2026-09-23 evening): the first-run screen imports the
   disc through the document picker, validates GZLE01 USA rev 0 and prepares main.dol and rels/ on
   the device, byte-identical to the Mac preparation. The composite still comes from a Mac build.
5. **Lifecycle.** DONE for home-screen round trips (recompcore 0059): the picture returns and the
   guest is suspended while away. DONE 2026-09-24: an audio interruption holds the game through the pause set
   (simulated on the simulator; a real call is a device check). Termination while backgrounded needs
   nothing: every card write replaces the file. DONE 2026-09-24: the touch controls survive a home-screen round
   trip (recompcore 0076; a new Metal view had been covering them).

6. **Speed.** WHERE IT STANDS 2026-09-24 night: rested on the simulator, Outset play, interiors and the
   village hold 58-60 retraces a second (the village 59.9 at 98 M cycles a retrace); the heaviest view
   (pier facing the island) holds 51-54 at 112 M cycles, work-bound. Refuted this night: user-interactive
   QoS for the GX workers (a large loss on the simulator), a lower EFB resolution (null), native leaf
   copies (priced out: their time is exact FP work, not boundaries). The remaining levers are the
   leader-guard fold in the emitter (priced at about 2 percent, not worth its rebuild) and the play-scene source
   port (the only one that reaches 60 in the heavy view; trades bit-exactness in ported functions).
   Awaiting the user's choice between them. Earlier history: 2026-09-24: the host profile retrained with a rendered route (scripts/pgo_host_train.sh):
   -5.6 percent instructions, -5.9 percent cycles for the simulator app. Retrain after hot host changes.
   On it: route Outset play 58.3, dialogue 59.4, heavy view 57.5/56.5 retraces a second.
   Heaviest stretch found (2026-09-24 evening): the walk into Outset's village, 24-32 retraces a
   second on the simulator, game thread 99 percent busy; the interior runs at 59.8.
   The unattended route on the simulator (2026-09-24 afternoon): prologue 60.0, play start
   59.9, Outset play 57.7, opening dialogue 58.9 retraces a second; audio starved 0.14 percent.
   Outset heavy view on the simulator: 57.4 retraces a second, median frame 16.8 ms
   (2026-09-24 afternoon: the actor search by id runs natively, -21.9 percent certified host
   instructions, digest exact); 54.0 (midday, 0082 build; the game thread busy 99 percent of the time), 52.7 / 51.8 (2026-09-24 evening before; 47.5
   the evening before, 18 the morning before that); macOS
   headless play-window median about 53. Landed: 0060 alias index, high-level DSP, 0062 staging,
   0063 worker wake, 0065 gather-pipe batching, PGO of the hot chunks, inline FP/paired-single
   helpers, PGO host, 0073 reused render packet (-4.6% rendered instructions), 0074 HUD texture
   re-uploads removed. The game thread is now 91% translated guest code; the GX worker waits on it.
   Audio stutters wherever the guest falls below 60 retraces a second (user report, 2026-09-24):
   measured (39% of pushes starved in the heavy view); recompcore 0077 stretches the output in time
   instead, pending a listen. Next: the guest-level profile
   (BLUEWAKE_PC_SAMPLE) names the actor-search family (cTgIt/fpcSch/cNdIt, about 10%) and the matrix
   helpers; judge candidates on the instruction bench, since the sampler overweights short dispatched
   functions (the __save_gpr/__restore_gpr native dispatch was a null).
   2026-09-24 night: host work cut 334.0 -> 280.0 M instructions per certified play retrace (FIFO
   writes without the device sync, a frameless edge front, the GroundCross observation off the turn
   loop), all digest-exact. What is left on the game thread is translated code (about 85%) and the
   per-edge service (7%, needed: interrupt delivery is gated to one cycle). The next step is an
   emitter project with a full rebuild and PGO retraining (hours): the leader guard folded into the
   precharge decision (priced at up to 6% of the body), or a block-local downcount.

7. **Graphics correctness. FIRST.** DONE: palette textures (recompcore 0066; title logo, minimap,
   HUD icons). DONE: EFB copies at the render scale (recompcore 0068; the 3D scene was 640x480 because
   Wind Waker redraws a full-screen copy every frame). Measure the heavy view after it. NOT REPRODUCED on the current build, user report
   2026-09-24 08:31 (before recompcore 0080-0082): on the simulator the title logo once drew as speckled outlines (THE LEGEND OF white on a
   dark box); not reproduced by captures, live screenshots, the START route or other FIFO batch sizes
   (recompcore 0075), the shell's pause, or a home-screen round trip; a candidate cause, palette
   loads with junk address bits (fixed in recompcore 0081), did not occur on a title run. DONE
   2026-09-24: pause-menu text matches Dolphin (stale A/B labels, recompcore 0080; the SAVE tab's
   rejected palette, recompcore 0081; the cursor's brackets, a palette loaded after its texture is
   bound, recompcore 0082). DONE 2026-09-24: Link's flat
   toon shading matches Dolphin (a reference from an input movie, scripts/make_dtm.py). Started 2026-09-24: title, file
   select, play HUD and pause menu match Dolphin. The prologue's story text, Outset's opening dialogue,
   the sea, swimming, shore and an interior (the steered route, working again) draw cleanly.
   DONE 2026-09-24 evening: moving 3D play matches Dolphin from shared inputs (scripts/ref_walk_corpus.sh:
   pier, swim, beach, grass, drowning and respawn; scripts/pad_trace_to_dtm.py: the village route
   replayed from BlueWake's own steering, where collision and step-up also agree). A normal container
   launch draws the title logo correctly in 40 of 40 screenshots.
   DONE 2026-09-24 night: EFB depth peeks answered from Aurora's snapshot (recompcore 0084); the
   sun's glare now draws, matching Dolphin with EFB access on (its movies had run with it off).
8. **Device readiness without a device.** NEXT (2026-09-24 night, user's choice): a physical iPad run
   before any change of route; state and install steps in status/IPAD_STATE_2026-09-24.md. DONE to the hardware gate (2026-09-24): the app builds
   for iphoneos, scripts/ios/build_device_composite.py builds the composite for A13 with PGO, and
   the embedded, ad-hoc-signed bundle passes codesign. Next needs a signing identity and a device.
9. **Compatibility routes on the simulator.** DONE 2026-09-24: the steered Outset route reaches the
   house interior; soaks of 54,000 and 30,000 retraces run to the end, memory flat at about 637 MB. The existing Outset, ladder, door, Omasao and save
   routes, then onward per the PRD's coverage catalog.

## Fences, carried over

No Nintendo data, original binaries, generated game code, saves, captures or signing material in
Git. `ref/` changes are numbered patches, registered in the lock. Do not weaken a clause to make a
log pass. One BlueWake process and one booted simulator at a time.
