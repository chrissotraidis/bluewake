# Legal & Provenance Audit (engineering audit — NOT legal advice)

> Historical research, superseded for publication: use [Rights and licenses](../../RIGHTS_AND_LICENSES.md) and the current source-only release policy.

Scope: what code/data each candidate component contains, where it came from,
what may be publicly hosted vs. must be user-generated, and what BlueWake
must never commit or redistribute.

> **2026-08-13 correction:** GPL-family licensing is not itself a code-signing
> prohibition. A ModernGekko/RecompCore build would carry GPL compliance and
> source-distribution obligations and may complicate Apple distribution.
> User-signed/sideloaded builds are technically possible; public distribution
> needs separate legal and license review. This document is not legal advice.

## Hard rules adopted for BlueWake

- Never commit/redistribute: ISOs, extracted assets, original DOL/REL binaries,
  Dolphin memory dumps, statically-recompiled output (it encodes Nintendo
  machine code), proprietary Nintendo SDK material, leaked source.
- `.gitignore` already blocks `*.iso, *.rvz, *.gcm, *.dol, *.rel, game/,
  ref/, local-research/`.
- No leaked Nintendo source may be used as reference at any point. The
  zeldaret projects enforce a no-leak policy for contributors; BlueWake
  inherits that stance transitively but should keep its own statement.
- Project codename "BlueWake"; no Nintendo trademarks/artwork in branding.

## Per-component provenance

| Component | Contains | Public hosting posture | Notes |
|---|---|---|---|
| zeldaret/tww, zeldaret/tp | Reconstructed (matching) C/C++ of Nintendo game code; no assets; build verifies vs. user-supplied disc hashes | Hosted publicly by zeldaret for years; compiled output embodies Nintendo-copyrighted expression | Same posture as all matching decomps (SM64, OoT). Tolerated to date; not risk-free. TP repo README/licensing to verify. |
| TwilitRealm/dusk | Port layer + (to verify) vendored TP decomp source; ships prebuilt binaries incl. iOS IPA | Shipped publicly 2026-05; still up as of 2026-08-08 | Distribution of compiled game code is the aggressive part; no takedown observed so far (UNVERIFIED — monitor). Assets remain on user's disc. License in LICENSE.md to verify. |
| DolRecomp / ModernGekko / Template | Clean tooling + runtime; PROVENANCE.md present (to verify contents) | Public | Tool itself is clean; its *output* is derived from the input binary and must stay local. |
| Aurora | Clean-room SDK/GX layer (from Metroid Prime ecosystem) | Public | License to verify (MIT expected). |
| decomp-toolkit | Clean tooling | Public | MIT/Apache expected; verify. |
| sp00nznet/ww | Recompiler + runtime, BUT commits `dolphin_*.bin` memory captures of the running game (game-derived data) and its architecture depends on them | Public (small repo) | **Provenance-contaminated for reuse** — treat as informative prior art only. Do not vendor. |

## Distribution model implications (macOS/iOS)

- The only provenance-clean end-user distribution model observed working today
  is Dusk's: publish source + (optionally) prebuilt app; user supplies their
  own disc image; assets never redistributed. For statically-recompiled code
  the stricter N64Recomp model applies: user runs the recompiler locally, or
  the distributed binary contains translated Nintendo code (same exposure as
  shipping the decomp build — community precedent exists but is legally
  untested).
- iOS: AltStore/sideload with the user's own Apple ID (Dusk's model) avoids
  App Store review entirely (an App Store submission of Nintendo-derived code
  would be rejected/struck). Requires Developer Mode; 7-day resign cycle on
  free Apple IDs; 3-app limit. AltStore PAL/EU alternative marketplaces are
  an option in the EU. This is a *distribution* constraint, not a technical
  blocker.

## Resolved items (2026-08-09)

- [x] dusk LICENSE.md = **CC0 1.0** (byte-identical to zeldaret/tp's).
      Aurora = **MIT**. borealis = to confirm on clone (encounter repo, MIT
      expected). decomp-toolkit = MIT OR Apache-2.0.
- [x] ModernGekko PROVENANCE.md: honest; pins Dolphin lineage; confirms
      GPL-2.0+ → combined work GPLv3; no game data included. The stack is now
      admitted only for the bounded prototype; distribution remains subject
      to compliance and legal review (see DOLRECOMP_MODERNGEKKO.md).
- [x] zeldaret/tww LICENSE = **CC0 1.0**; README: "does not contain any game
      assets or assembly whatsoever" — verified true (assets/ holds only
      generated index-enum headers; binary blobs extracted from the USER's
      dol at build time via config extract entries).
- [x] No candidate repo in the chosen stack vendors original Nintendo SDK
      headers; tww's src/dolphin is reconstructed matching source; aurora's
      dolphin headers are clean-room API declarations.
- [x] Chosen-stack posture: every layer of the favored architecture
      (tww CC0 + dusk-pattern port code CC0 + aurora MIT + borealis + SDL3 +
      Dawn BSD) is permissively licensed and contains no leaked material.
      The compiled BlueWake binary still embodies Nintendo-copyrighted
      game code (as does Dusklight's) — distribution posture should mirror
      Dusklight's (source-first, user-supplied disc, no assets) and note
      that this community practice is legally untested.
