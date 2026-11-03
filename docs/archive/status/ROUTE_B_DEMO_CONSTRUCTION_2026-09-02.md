# Route B Demo Construction

**Status:** construction and play-owner composition qualified on 2026-09-02.

The bounded full-unit census for `d_demo.cpp` initially reported seven
constructor-required host portability errors plus one unrelated camera error.
Patch 0074 keeps the PowerPC branches unchanged while using native pointer
arithmetic, explicit iterator narrowing, and a local JGadget comparator on
`TARGET_PC`. Repeating the full compile with the authentic asset include then
left only the known camera error. The preserved log has SHA-256
`9fa2727d8e87f2d93062373386cc89e1acd4868323cec8de5365ec143b2e0a57`.

Patch 0075 partitions the unchanged constructors for `dDemo_object_c`,
`dDemo_system_c`, and `dDemo_manager_c` away from camera, playback, parsing,
and update behavior. The constructor-only unit compiles. Its first retained
link census normalized the manager's eight allocations into these owners:

1. `dMesg_tControl`
2. `dDemo_system_c`
3. `JStudio::TControl`
4. `JStudio_JStage::TCreateObject`
5. `JStudio_JAudio::TCreateObject`
6. `JStudio_JParticle::TCreateObject`
7. `JStudio_JMessage::TCreateObject`
8. `JStudio::TFactory`

Patch 0076 promotes the first owner with its real `JMessage::TControl` base
and the already-qualified retail message HIO. The public test proves null
font ownership, zero line state, both HIO-derived font sizes, code/header
state, width 486, and four zero line lengths. Original JStage base methods
also back the second allocation; the aggregate probe observes the real system
bound to the manager's embedded demo object.

Patches 0077-0078 retain original stb/fvb control construction, link-list
ownership, transform defaults, and factory append while parsing and create
callbacks remain fail-closed. The aggregate manager is now a default public
test. It proves all eight allocations, exact object wiring, 1/30 timing,
factory append order, disabled transforms, and mode zero.

Patch 0079 isolates unchanged `dComIfG_play_c::createDemo` behind a minimal
compile frontier analogous to the qualified particle owner. The focused test
proves the real method stores the real manager. Exact play phase 4 then calls
that owner between real collision and sea/snapshot initialization. Unentered
destruction, parsing, playback, camera, and create callbacks remain explicit
aborting fences.

All 26 public tests pass in Debug, optimized Release, and strict ASan/UBSan.
Patches 0074-0079 change 138 of 7,591 reached source/header lines (1.82%).
The successor is the distinct player/window/camera/draw-view state band in
`BW-P4-0103`, not broader demo playback.
