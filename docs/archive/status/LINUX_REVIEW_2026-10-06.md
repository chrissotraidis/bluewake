# Linux shutdown and log review

October 6, 2026. Read-only review of [PR #107](https://github.com/chrissotraidis/bluewake/pull/107)
at `e0951c5e7be8025e52a9417be33f35c2e258756d`; no contributor branch or RecompCore source changed.

## Evidence received

The contributor supplied [the full session log](https://gist.github.com/jkoehler11/2b5aec8b533b7de4f6ea6b32f5e47a62)
and [a shutdown diagnosis](https://github.com/chrissotraidis/bluewake/pull/107#issuecomment-6008169596).
The log identifies a Ryzen 9 5900X, Radeon RX 6600 / RADV, Mesa 26.2.4-arch1.1,
EndeavourOS, Vulkan, 960x720 and Smooth Motion off at a 30 FPS target. Despite the
comment's word "headless", this is an Aurora/Vulkan run with a compatible surface and window;
it is not a test of BlueWake's no-render headless backend. Input was scripted, so controller
recognition in the log does not prove manual controls.

The exit summary records 11.1 minutes, 434 watched intervals, one below target, a lowest game
speed of 43% and no newly compiled pipelines in those watched intervals. The full log also has
nine hitches, with a worst frame of 387 ms. This supports good sustained performance on that
configuration, not an uninterrupted 30 FPS guarantee or complete game/platform acceptance.

Crucially, the final line after the performance summary is `double free or corruption (!prev)`.
The contributor reports a clean post-fix run, but the supplied log still contains the failure.
Its source/runtime provenance must be clarified before treating it as post-fix shutdown evidence.
A clean log tail plus exit status from the actual patched build is the small remaining request;
there is no need to repeat an eleven-minute performance run merely to prove shutdown.

## Patch assessment and integration gaps

The patch moves function-local device-owning texture-layout and empty-texmap caches to file scope
and releases them in `gxcore::shutdown()`. Its call site follows interpolation/render-worker and
pipeline-compiler shutdown, and precedes `webgpu::shutdown()` and `window::shutdown()`.
This matches the described late-destructor backtrace and is a plausible focused repair.
`git apply --check` succeeds against the current pinned RecompCore `d5b92fb` without source mutation.
No Linux build or hardware reproduction was performed on this Mac, so that check is not acceptance.

The PR currently only adds the exported shutdown patch. Its profile and lock still pin
`95a6a47`; the Linux builder and CI fetch that pin and do not apply this new patch file. Therefore
an untouched fresh build does not yet incorporate the repair. The proper next step is a
RecompCore `bluewake-next` PR retaining James Koehler-Killeen's authorship, followed by the BlueWake
pin/profile update and patch export. Current main already uses patch number 0157 for dungeon maps;
allocate the next available number when integrating rather than reusing that number.

The current Linux workflow still lacks `libxtst-dev`, and Linux CMake/CI do not yet include the
shared DVD-completion or cache-writeback fallback regression tests. Bring current main in, retain
both candidates' off-by-default behavior, and add those shared tests before expecting CI parity.
The outstanding manual save/reload, settings, controller, dungeon-map and clean-quit checklist
remains the official-support gate. Promotional footage is optional; reuse permission was supplied.

## Triage tool repair

The session summary previously printed `fatal lines: 0` for this log: it discarded untagged lines
before searching for failures, and its failure expression did not recognize glibc allocator aborts.
The repair checks failure text before structured-tag parsing and recognizes double-free and
invalid/corrupted allocator messages. Existing benign device-destruction exclusions remain intact.
The original supplied log now reports one fatal line and still retains its performance summary.
Synthetic regression tests cover the actual timestamped abort, untagged allocator/console failures,
ordinary tagged failures counted once and benign shutdown messages. No reporter paths or log files
are committed. This fixes the analysis tool, not the Linux runtime crash.
