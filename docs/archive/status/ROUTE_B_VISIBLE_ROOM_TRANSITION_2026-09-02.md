# Route B Visible Room Transition - 2026-09-02

## Boundary

`objectSetCheck` owns the state transition around the qualified Room44
reloader. A visible room queues BG and reloads once; a hidden room removes its
layer children, clears room switch state, and permits one later reload.

## Result

Patch 0121 promotes the original state decision and calls the exact native
reloader directly. Against the 10,880-byte title DZR, first visibility queues
BG before 172 ACTR, five TGDR, and one SCOB request, for 179 total. A repeated
visible check queues nothing. Hiding invokes child deletion and zone-9 switch
cleanup once and clears `mbReLoaded`; a repeated hidden check is a no-op. One
later unhide produces one further 179-request graph with BG first.

Child deletion and saved room-switch mutation remain narrow adapters to their
existing process and save owners. The room transition itself reads the native
room status, preserves the retail branch structure, and invokes the real
reloader. No requested profile executes.

## Evidence

- Exact Room44 DZR SHA-256:
  `0c89307c89183f65fd8ea72595bd15855ca24f2441b91b1b4eb06a353a33ace9`.
- Patch SHA-256:
  `828bd1cb4f1fcac1d2561c0455d1935ecd6a75502bde6912e0e90b0821567559`.
- Public contract SHA-256:
  `4d05e1de140fe5bccdc3865addfc5c1067d899db0dfaec7b068d747e63fa439b`.
- Debug focused executable SHA-256:
  `120ec6276308e013f2797aa74a3fa62005f197697f034a088e02c137194650b4`.
- Release focused executable SHA-256:
  `84c5f0ccda43fc13d7c8b441b2be943764068fed6f1c8656aedaccc8770d60fd`.
- ASan/UBSan focused executable SHA-256:
  `3abbe0cef170b648f2014aaf67d0dfb0f12ac39b78ac44a5e3e47aa15505989e`.
- Patches 0001 through 0121 replay cleanly from pinned TWW commit
  `03d27aa14389e648f51df2bc055ee3d53a3b67f2`.
- All 60 registered tests pass in Debug, optimized Release, and strict
  ASan/UBSan. The focused contract also passes separately against the exact
  private DZR in all three modes.

No BlueWake process or Simulator was launched.

## Next Boundary

Census BG and the 15 Room44 actor profile families before executing any of
them. Record source owner, profile size/methods, current target-PC compile
status, immediate link frontier, resource dependencies, and likely child
fanout. Use the result to select the smallest representative construction
cohort. Keep aggregate profile execution, room phase 4, BlueWake, and
Simulator closed.
