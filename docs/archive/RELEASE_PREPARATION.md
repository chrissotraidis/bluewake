# Source release preparation

The release goal is a public BlueWake source repository and a reproducible,
interactive Mac builder that produces a personal IPA from the player's disc.
A source preview can be published after its source/history audit and accurate
build instructions pass. Claiming full-speed player-build equivalence additionally
requires matched device performance checks against the developer build; a
successful compile alone does not establish that result.

## Goal loop

1. Preserve the private development repository and local data. Start the new
   repository from reviewed source with fresh history; retain the README,
   badges, community links, license and repository presentation.
2. Audit every tracked file and the public source archive. Keep disc contents,
   translated modules, saves, signing material and private training profiles
   out of Git. Review binary fixtures and patches manually as well as running
   the automated checks.
3. Make the builder generate its optimization profile on the player's Mac.
   Train using their disc, verify that training reached gameplay, then compile
   the optimized iOS module. Generate supported mod variants locally too.
4. Show stage, elapsed time, compiler progress and actionable failures. Cache
   only results whose input identities match. Preserve logs for diagnosis.
5. Build from a fresh source checkout without development-only dependencies.
   Validate the personal package and install in place after backing up saves.
6. Compare the player and developer builds on the same iPad, settings, scenes
   and thermal conditions. Check Outset and a second demanding area. Preserve
   save data and record actual gameplay evidence.
7. Transfer the proven stages and their progress/cache contract to PadMint;
   keep one shared builder with game-specific adapters.
8. Recheck the exact source archive and Git history, review player instructions,
   update the private audit record, then publish only cleared source/tooling.

After a failure, record the cause, make the smallest justified fix, and repeat
the affected check. Do not substitute an easier scene or a developer-only
profile for the player build. Do not call the release ready while a required
check remains unverified.

## Current status

- The private repository was renamed and its local checkout preserved.
- The new checkout began with the same source tree and one initial commit.
- Fresh-card Mac training passed with the shipping HLE audio backend: 18m24s
  of playback, player control at retrace 20,405, and a normal stop at 23,000.
  It recorded 262 translated functions, including all 100 covered by the
  retained developer profile. A sampled function's profile hash matched across
  Mac O0 training and iOS O2 compilation. Hardware performance remains unverified.
- The complete builder verified and reused those local profiles for a freshly
  regenerated source tree. The optimized iOS compilation completed in 78m42s
  on an M3 Max with 16 jobs. The personal IPA passed package integrity,
  provenance, module-identity and private-data exclusion checks; the local
  app's signature also verified. These checks do not establish device performance.
- A four-run Mac comparison (developer/player/player/developer) loaded the same
  copied save, accepted scripted movement and produced identical route digests
  and player-state records. Over retraces 1,500–3,900, the locally trained module
  averaged 13.26 ms per retrace versus 13.62 ms for the developer module.
  This short headless test uses an instrumented host; it does not measure iPad
  FPS, rendering, audible output or mod behavior.
- PadMint's real source-only BlueWake integration passed: expected source
  digest, seven stage start/completion pairs, and a clean unchanged checkout.
- Fresh extraction, translation and all mod variants reproduced the same
  805-file source digest as the training input. The iOS host app also compiled.
- The public-assets wrapper now normalizes ZIP/TAR contents safely before
  scanning; the old compressed-TAR result was insufficient content evidence.
  Malformed, unsafe and nested archives fail closed. Source references were
  reviewed separately from actual translated function implementations.
- Stage cancellation (including a TERM-resistant descendant), profile changes
  forcing recompilation, paths with spaces and mod-cache invalidation passed
  focused checks. The source archive passed the publication scan and manual
  metadata/patch review.
- GitHub Actions could not start because of an account billing/spending-limit
  error; its repository audit and dependency checks passed locally.
- Earlier matched Outset tests measured about 26 FPS without PGO, 27.5 FPS
  with runtime/host profiles, and 29.9 FPS with the developer's full profile.
  These measurements do not yet prove the new local-training path.
- Corrected ZIP/TAR source scans, manual history/provenance review and player
  documentation checks passed for the source preview. The maintainer controls
  repository visibility. Hardware acceptance separately gates full-speed
  player-build claims. Personal IPAs contain translated game code and remain
  on the builder's machine.
- The iPad is unavailable during this pass. All ongoing work is Mac-only;
  hardware acceptance is deferred until the owner makes it available.

See [PadMint handoff](../PADMINT_HANDOFF.md) for the shared-builder contract.
