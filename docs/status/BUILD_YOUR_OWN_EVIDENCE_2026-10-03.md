# Build-your-own and PadMint evidence (October 3, 2026)

Moved on October 15 from [Build your own BlueWake](../BUILD_YOUR_OWN.md) and the
[PadMint handoff](../PADMINT_HANDOFF.md), so those guides describe how things work. These are dated records
at BlueWake `3392854`, not instructions; current status is in [GOAL_LOOP.md](../GOAL_LOOP.md).

## The combined Mac build (October 3)

**Acceptance is in progress.** At `3392854`, the fresh owned-disc combined
build completes translation, local training, compilation, packaging and local
signature checks. Its relocated app passes actual game save/reload while
preserving the original card and other slots, and fullscreen/preferences survive
a separate launch. Fresh start reaches player control; a current-module save
state also restores correctly after quitting and reopening the app. A fresh
ladder/outdoor/door/indoor route passes. These checks do not establish every
scene, audible output, physical
controllers or sustained performance. A local replacement from the recorded
earlier developer app also preserves copied saves/settings/SRAM and loads the
save afterward; physical iOS upgrades remain untested. See the
[reconciliation ledger](FORK_RECONCILIATION_2026-10-02.md) for remaining
checks before migration is recommended.

## PadMint reconciliation (October 3)

The tested PadMint CLI is 0.2.9 at
`de13bd769540642f89d6fb2e0492d466a58ae32b`. A new source-only workspace passes at
BlueWake `3392854c8daaf5d7900cd034694fabe18b699a24`, selecting maintained runtime
`18ba3b642588a33b9e8eac4aba7f713bb8d3d778` and translator
`b8b534591cba8ca7cd43943a655ee6e2591cf5de`. The expected base-source digest matches.
The full run is now complete using the matching locally built app-only shell
and fresh local training. Final package structure/provenance, module/profile
hashes, original-shell code/data and actual interrupted-build reuse pass.
All 662 objects and four profiles retained at the latest resume keep identical
contents; the finished module has 822 objects. The publication gate correctly
rejects this translated-game-code IPA as personal-only. It remains local.

Keep the retained workspace pinned to `3392854`. Later main contains the
host/runtime Pictobox repair. Its clean `27c02a1` / `0568fedd` app-only shell
has been combined locally with the completed module, preserving both payloads
and separately embedding their provenance. Module compile dependencies and
preparation sources are unchanged. This is a compatibility-update candidate,
not a new clean current-source build or a second completed PadMint run.
The update candidate passes deep/strict development-signature verification and
is installed in place on the physical M2 iPad. Existing save/settings bytes are
preserved; bounded game save and separate-process reload of an isolated card copy
pass. This accepts the updated shell with the completed PadMint module, not a
second clean current-source build. Sustained play and matched performance remain open.

| Route | Evidence / remaining gate |
| --- | --- |
| Apple Silicon Mac → iOS source-only | Actual CLI source generation passes at `3392854`; USA rev-0 disc and exact source/dependency identities verified |
| Apple Silicon Mac → iOS full personal IPA | Earlier `95adeed` / CLI 0.2.8 complete assembly passes. At `3392854` / CLI 0.2.9, fresh 23,000-retrace training, compilation, final assembly/provenance and retained-object/profile reuse pass. The separate current-shell update candidate passes signing, physical iPad install, save/reload and in-place data preservation. Sustained play and matched performance remain open |
| macOS player app | Direct BlueWake builder now produces a fresh personal app with save/reload evidence. PadMint Mac remains planned; the direct builder does not establish adapter support |
| Windows / tvOS / Linux | No complete PadMint adapter acceptance; use each platform's explicitly documented source route or preview boundary |
