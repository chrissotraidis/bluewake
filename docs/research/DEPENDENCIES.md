# External Dependencies & Research Repositories

All third-party repositories live in `ref/` (gitignored). Pinned SHAs recorded
at examination time. "Required" means required by the currently favored
architecture; "informative" means examined for research only.

Examined: 2026-08-09.

| Name | URL | Pinned SHA | Last commit | License | Purpose | Status |
|---|---|---|---|---|---|---|
| zeldaret/tww | https://github.com/zeldaret/tww | `2289b54d5fd4ad22f7f9ee52ea4e2e81d79b5008` | 2026-08-08 | CC0-1.0 (no game assets in repo) | Matching decompilation of The Wind Waker (GameCube). Candidate source foundation. | ACTIVE, incomplete — analysis below |
| zeldaret/tp | https://github.com/zeldaret/tp | `c8fa8c9e2aab72cf4e5db0e5d1c84a9ea6ee6eb0` | 2026-06-23 | CC0-1.0 | Completed TP decomp; foundation of Dusk. Reference for what "done" looks like. | COMPLETE (shipped in Dusk) |
| TwilitRealm/dusk | https://github.com/TwilitRealm/dusk | `7434a0f834866294e7691bd86ab9527011265585` | 2026-08-08 | CC0-1.0 | Native TP port (Win/mac/Linux/Android/iOS) from tp decomp. Primary architectural precedent + candidate reusable runtime. | SHIPPED v1.0.x, active |
| ExpansionPak/DolRecomp | https://github.com/ExpansionPak/DolRecomp | `48c4ef11dd59c7367a3479a433e39a35bda80695` | 2026-08-03 | GPL-3.0 | GameCube/Wii PowerPC→C static recompiler. Candidate for recompiling not-yet-decompiled TWW code. | ACTIVE, maturity under review |
| ExpansionPak/ModernGekko | https://github.com/ExpansionPak/ModernGekko | `048c426ba3db0369e40826d22ad3adcce7fe7c58` | 2026-08-03 | GPL-3.0-or-later (Dolphin-derived) | Runtime for GameCube recompilations. | ACTIVE, maturity under review |
| ExpansionPak/ModernGekko-Template | https://github.com/ExpansionPak/ModernGekko-Template | `1ee85bb5e09c38f493a09f5fa6e9dc8228b23e42` | 2026-08-03 | none stated (defers to submodules) | Project template wiring DolRecomp output into ModernGekko. | ACTIVE |
| encounter/aurora | https://github.com/encounter/aurora | `1d10fa1bc502910a6336fdac32f31cd0ac39710d` | 2026-08-06 | MIT | GameCube SDK/GX compatibility layer (used by Metroid Prime decomp ecosystem). Candidate GX layer. | ACTIVE |
| encounter/decomp-toolkit | https://github.com/encounter/decomp-toolkit | `e4219e7644fb7b96d920d5bc3d1d950f5569dcaf` | 2026-03-01 | MIT OR Apache-2.0 | dtk: GameCube/Wii decomp build tooling (DOL/REL analysis, splitting, hashing). Used by tww build. | STABLE |
| encounter/borealis | https://github.com/encounter/borealis | (submodule of dusk; not separately cloned) | — | see repo | Game-agnostic app shell used by Dusk: disc verify (xxh3), data dirs, native file picker (iOS document picker), crash/Sentry, updater. | REQUIRED for Approach D |
| sp00nznet/ww | https://github.com/sp00nznet/ww | `146ecde38b683a718cbafffe1555880e872537ad` | 2026-03-29 | none stated | Solo Wind Waker pure static-recomp attempt (Windows/D3D11). Informative prior art ONLY — provenance-contaminated (commits Dolphin memory dumps); stalled pre-gameplay. | INFORMATIVE, do not vendor |
| KaiserGranatapfel/GameCubeRecompiled | https://github.com/KaiserGranatapfel/GameCubeRecompiled | (cloned, cursory look) | — | see repo | Experimental GC→Rust recompiler inspired by N64Recomp. Early toy. | INFORMATIVE |

Notes:
- No Nintendo game data has been downloaded or committed. The tww/tp decomps
  contain reconstructed source only and require a user-supplied original disc
  image to build/verify.

## Reassessment snapshot reported 2026-08-13

The second-opinion report inspected newer revisions below. These observations
were not independently reproduced in this repository and do not replace the
2026-08-09 pins above; refresh before relying on them.

| Name | Reported SHA | Reported observation |
|---|---|---|
| zeldaret/tww | `93a2a6522a0e8a05093194452aa99d176bbfcb32` | 73.422485% matched code; 31,459/39,324 matched functions |
| ExpansionPak/DolRecomp | `fa0cf619e8d7eb8cba7eaf55267a12caaebb46aa` | Apple Silicon build succeeded; 16 tests passed; REL generation/relocation tests present |
| ExpansionPak/ModernGekko | `884c20505d7179160f8bb01d9db0f723c53b09cb` | Newer runtime snapshot examined by reassessment |
| ExpansionPak/RecompCore | `e13ab348f13cd67879f6db6e9d7185410f8f62c6` | REL metadata and runtime address-remapping implementation reported |

See `../archive/FEASIBILITY_REASSESSMENT_2026-08-13.md` for scope and evidence limits.
