# Route B Toripost Audio Construction - 2026-09-06

Exact retail evidence corrected the planned BAS-metadata boundary: Toripost's
25-frame wait and 14-frame get BCKS resources both carry the no-BAS sentinel.
Patch 0140 therefore retains only original `JAIAnimeSound` construction and
its inert empty-slot `stop`, while BAS parsing, live audio, and position-driven
playback remain abort-fenced.

The focused probe now uses Toripost's unchanged McaMorf `param_8=TRUE`,
observes a real `mDoExt_zelAnime` with null BAS across both animations, and
passes in Debug, optimized Release, and strict ASan/UBSan:

`mca-morf archive=9852 model=joints3-materials1 bck=wait25-get14 bas=absent morph=8+4 frame=1 calc=1 packets=1 audio=constructed pass`

Executable SHA-256 values are Debug
`fd0b7d9372f5233d4328c3bb3d0932dac4082c4d1f8eb7934d3d09a79def5bec`,
Release `410781ec5b9759816a4c4775a07f01da47ec135df1d73f7de30cdae1a4b9f6e6`,
and sanitizer
`12ab9bc68dad10bc964a1df10b4610d57342d742f3326d6c6f0a097e319cca0b`.
Exact Room44, Stone2, and BG regressions and all 65 public tests pass in all
three configurations. Patches 0001-0140 replay cleanly; protected recompcore
hashes are unchanged; no BlueWake process or Simulator was launched.

Next: partition original Toripost around resource/solid-heap creation,
fresh-save WAIT, stable idle execute/draw, and delayed teardown. Keep mail,
event, item, particle, crash, collision-response, audio, non-idle modes, and
exact-parent composition closed until the independent lifecycle passes.
