# Provenance — source base imported from SwitchWaker

- Repository: https://github.com/centollOS/SwitchWaker
- Commit: `04372277d03f934e1e563ae302bba9f565c5cb5d` (`main`, 2026-10-13)
- Method: file copy (no Git history imported), exact pin, no modifications
- Import date: 2026-10-17
- Imported by: `scripts/source_port/import_switchwaker.sh` (this repository)
- Manifest: `source-port/IMPORT_MANIFEST.sha256` (sorted, `sha256sum -c` format,
  2 561 files, 30 585 827 bytes; manifest sha256
  `9cb738d8b71c51db2b9cfffe31e9f3929209a37f4d11aa4a609ec2cf8c8e781f`)

## What is here

| Path | Origin | License |
| --- | --- | --- |
| `game/` | the Wind Waker decompilation (`snrubrm/tww` fork plus the port's `TARGET_PC` hunks and the `zeldaret/tww` convergence) | CC0-1.0 (`game/LICENSE`) |
| `native/CMakeLists.txt`, `native/cmake/` | the port's build system | MIT (`LICENSE` in the repository root of the source) |
| `native/sdk/`, `native/include/sdk/`, `native/include/pc/`, `native/check/`, `native/tools/`, `native/src/pc/` | the game-specific SDK over Aurora and its run harness | MIT |
| `native/include/helpers/`, `native/src/helpers/` | big-endian/offset-pointer helpers, from Dusklight at `40457c6` | CC0-1.0 |
| `native/dsp_hle/` | Dolphin's DSP HLE behind `cos_sdk`, adapted from this repository's own `runtime/host/src/dsp_hle_backend.cpp` and `runtime/host/src/dsp_common_shim.cpp` lineage | GPL-2.0-or-later (`native/dsp_hle/LICENSE`, SPDX headers) |

The destination mirrors the upstream repository root for the imported trees (`game/` and
`native/` keep their names), so the port's own `CMakeLists.txt`, `cmake/` and `tools/` work
unchanged (`COS_ROOT` is `../game` from `native/`; `native/tools/gen_assets.sh` reads
`../../game/UPSTREAM`).

Two files beyond `PLAN_IMPORT.md`'s group list are imported because the build system requires
them: `native/CMakeLists.txt` (the project root the CMake modules hang from) and
`native/src/pc/` (the run harness `cos_pc`, globbed by `native/cmake/executable.cmake` and
linked by `cos_pc_tests`).

## What is not here

`switch/` (the Switch platform layer, deko3d renderer, uam), `native/data/`
(`initial_pipeline_cache.db`), `native/tools/dksh_cache/`, the Switch Aurora patches
(`native/patches/aurora/`, `switch/native/aurora/patches/`), their `docs/`, `scripts/`, root
documents and CI. None of it is needed for the host build or the planned Vita work.

No disc data, no game assets, no saves and no binaries: the source repository tracks none
(checked), and the import script refuses any file outside its list.

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
