# Provenance — source base imported from SwitchWaker

- Repository: https://github.com/centollOS/SwitchWaker
- Commit: `04372277d03f934e1e563ae302bba9f565c5cb5d` (`main`, 2026-10-13)
- Method: file copy (no Git history imported), exact pin, no modifications except the three
  documented GCC flag gates in `native/cmake/GameConfig.cmake` (see "Local changes vs upstream")
- Import date: 2026-10-17
- Imported by: `scripts/source_port/import_switchwaker.sh` (this repository)
- Manifest: `source-port/IMPORT_MANIFEST.sha256` (LC_ALL=C sorted, `sha256sum -c` format,
  2 584 files, 30 874 913 bytes as committed; manifest sha256
  `33b22f660f833fd007f4c419e2182033d47ed580a15247c34faa1872f266b3b4`)
- Rule: refresh this header (file count, bytes, manifest sha256) whenever the manifest is
  regenerated; the manifest records the current hashes, so it must be regenerated after any
  change to an imported file.

## What is here

| Path | Origin | License |
| --- | --- | --- |
| `game/` | the Wind Waker decompilation (`snrubrm/tww` fork plus the port's `TARGET_PC` hunks and the `zeldaret/tww` convergence) | CC0-1.0 (`game/LICENSE`) |
| `native/CMakeLists.txt`, `native/cmake/` | the port's build system | MIT (`LICENSE` in the repository root of the source) |
| `native/README.md`, `native/LICENSE` | the port's native-build documentation and the upstream repository's MIT license text, copied to `native/LICENSE` | MIT |
| `native/sdk/`, `native/include/sdk/`, `native/include/pc/`, `native/check/`, `native/tools/`, `native/src/pc/` | the game-specific SDK over Aurora and its run harness | MIT |
| `native/include/helpers/`, `native/src/helpers/` | big-endian/offset-pointer helpers, from Dusklight at `40457c6` | CC0-1.0 |
| `native/patches/aurora/` | the Aurora patch set (0001-0021) the port's harness and renderer need | MIT, as Aurora |
| `native/dsp_hle/` | Dolphin's DSP HLE behind `cos_sdk`, adapted from this repository's own `runtime/host/src/dsp_hle_backend.cpp` and `runtime/host/src/dsp_common_shim.cpp` lineage | GPL-2.0-or-later (`native/dsp_hle/LICENSE`, SPDX headers) |

The destination mirrors the upstream repository root for the imported trees (`game/` and
`native/` keep their names), so the port's own `CMakeLists.txt`, `cmake/` and `tools/` work
unchanged (`COS_ROOT` is `../game` from `native/`; `native/tools/gen_assets.sh` reads
`../../game/UPSTREAM`).

Beyond `PLAN_IMPORT.md`'s group list, the build system, the harness and the review require:
`native/CMakeLists.txt` (the project root the CMake modules hang from), `native/src/pc/` (the run
harness `cos_pc`, globbed by `native/cmake/executable.cmake` and linked by `cos_pc_tests`),
`native/patches/aurora/` (the harness calls Aurora APIs the patch set adds, e.g.
`AuroraSetContentScale`), `native/README.md` (referenced by six imported files) and
`native/LICENSE` (the upstream MIT text, copied from the repository root).

`native/README.md` mentions that the decompilation fork completes its remaining functions "with AI
assistance" in its prose; that is upstream provenance, not an attribution line, and the file is
kept verbatim.

## What is not here

`switch/` (the Switch platform layer, deko3d renderer, uam), `native/data/`
(`initial_pipeline_cache.db`), `native/tools/dksh_cache/`, the Switch-only Aurora patches
(`switch/native/aurora/patches/`), their `docs/`, `scripts/`, root documents and CI. None of it
is needed for the host build or the planned Vita work.

No disc data, no game assets, no saves and no binaries: the source repository tracks none
(checked), and the import script refuses any file outside its list.

## Local changes vs upstream

Three changes so far, on top of the exact copy (all in `native/cmake/GameConfig.cmake`, all for
the GCC host the smoke build runs on; the upstream build is written for clang):

- `-ferror-limit=50` is gated on Clang/AppleClang, because GCC rejects it (its equivalent is
  `-fmax-errors=N`).
- `-Wno-changes-meaning` is added for GNU compilers, because the decompiled code names union
  members after their types (`JAISequenceMgr.h`'s `PortArg`) and GCC 14+ diagnoses that as an
  error where clang accepts it.
- `-Wno-narrowing` is added for GNU compilers, because `-sizeof(...)` in a template argument
  (`JUTConsole.h`'s `ConsoleList`) narrows `unsigned long` to `int`; clang's
  `-Wno-c++11-narrowing` suppresses it and GCC's flag is `-Wno-narrowing` (a `-Woverflow` warning
  remains, which is not fatal).

Upstream hash as imported:
`d43c94d1b9acf40edcf2a15280123b1cfed1c62b7459eda2bbbd211eea3975f6`; current:
`12888b67f766fa649dfe2c38b172335f7bfe7cded0bff0eba3a2d5a49be4371a`. The manifest entry records
the current hash.

## Attribution

Human authorship only. The imported files contain no AI attribution lines (grep for
`Co-Authored-By`, `Anthropic`, `Claude`, `Copilot`, `OpenAI`, `ChatGPT`: 0 occurrences in
`game/` and `native/`), and no Git history was imported — the upstream commit messages carry
`Co-Authored-By: Claude` lines, which is why the copy method was chosen over `git subtree`.

Credits:

- `game/`: the `zeldaret/tww` contributors and `snrubrm` (the fork that completes the
  remaining functions); upstream pins in `game/UPSTREAM` (`snrubrm/tww` `b09eebc`,
  convergence `zeldaret/tww` `a1854d47`).
- `native/`: Pulpparty, depende3000 and the SwitchWaker contributors (MIT).
- `native/include/helpers/`, `native/src/helpers/`: TwilitRealm/Dusklight (CC0).
- `native/dsp_hle/`: adapted from the recompilation project's `runtime/host/src/`
  (this repository's lineage) over Dolphin's DSP HLE (GPL-2.0-or-later).

The imported `LICENSE` files are kept verbatim; `RIGHTS_AND_LICENSES.md` records the addition.
