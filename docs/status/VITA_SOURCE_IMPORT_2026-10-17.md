# Vita source-port import — 2026-10-17

Import of the SwitchWaker source base into `source-port/`, executed from `PLAN_IMPORT.md` (sealed
`build-f5/runtime-next/phase2-prep-3c882970`) with the evaluation `vita-source-port-eval-6db00b6b`
(criterion C1 satisfied: `TWW_BYTEMATCH_PARTIAL`, the byte-match difference localized to a
documented non-deterministic compiler bug; accepted by the coordinator with that environmental
caveat) and the user's go-ahead of 2026-10-17.

**Branch:** `source-port/import-base`, base `main` at `2b097a4e174b2bb7114a8f2bed12af1ea0f7692f`.
No push, no pull request; step 7 (independent review) and step 9 (merge) are separate.

## What was imported

- Base: [centollOS/SwitchWaker](https://github.com/centollOS/SwitchWaker) at
  `04372277d03f934e1e563ae302bba9f565c5cb5d` (`main`, 2026-10-13), read from the read-only clone
  `/home/fred/projects/ilp32-census-work/switchwaker` (clean at the pin).
- Method: file copy, no Git history (the upstream commit messages carry `Co-Authored-By: Claude`
  lines; our rules keep human authorship only). Reproduced by
  `scripts/source_port/import_switchwaker.sh`.
- Layout: `source-port/` mirrors the upstream repository root for the imported trees —
  `source-port/game/` and `source-port/native/**` — so the port's own `CMakeLists.txt`, `cmake/`
  and `tools/` work unchanged (`COS_ROOT` is `../game` from `native/`; `gen_assets.sh` reads
  `../../game/UPSTREAM`). `PROVENANCE.md` and `IMPORT_MANIFEST.sha256` sit at its root.
- Size: 2 584 files (2 263 in `game/`, 321 in `native/`), 36 MB; manifest sha256
  `33b22f660f833fd007f4c419e2182033d47ed580a15247c34faa1872f266b3b4`
  (`cd source-port && sha256sum -c IMPORT_MANIFEST.sha256`).
- Licenses: `game/` CC0-1.0; `native/` MIT; `native/include/helpers/` and `native/src/helpers/`
  CC0-1.0 (Dusklight at `40457c6`); `native/dsp_hle/` GPL-2.0-or-later (decision D2=A);
  `native/patches/aurora/` MIT, as Aurora. Details in `source-port/PROVENANCE.md`.
- Attribution: no AI attribution lines in the imported files (grep for `Co-Authored-By`,
  `Anthropic`, `Claude`, `Copilot`, `OpenAI`, `ChatGPT`: 0 occurrences in `game/` and `native/`);
  `scripts/check_attribution.py` passes on the branch.
- Hygiene: no disc data, no game assets, no saves, no binaries (extension check: 0); `switch/`,
  `native/data/`, `native/tools/dksh_cache/` and the Switch-only Aurora patches are not imported.

## Scope adjustments vs PLAN_IMPORT.md (all required by the host smoke)

- `native/CMakeLists.txt` and `native/src/pc/` (the run harness `cos_pc`, globbed by
  `native/cmake/executable.cmake` and linked by `cos_pc_tests`).
- `native/patches/aurora/` (0001-0021): `native/src/pc` calls Aurora APIs the patch set adds
  (`AuroraSetContentScale`, the texture-replacement runtime API). The import script now lists it.
- `.gitignore`: `!source-port/game/`, so the existing root `game/` rule does not hide the import
  (the first commit had silently missed the 2 263 `game/` files; fixed before review).
- Three GCC gates in `native/cmake/GameConfig.cmake` (the upstream build is written for clang; this
  machine has GCC 16.2.1 and no clang): `-ferror-limit` gated on Clang/AppleClang;
  `-Wno-changes-meaning` and `-Wno-narrowing` added for GNU. All three are recorded in
  `PROVENANCE.md` with the upstream and current file hashes.

## Host smoke (PLAN_IMPORT step 6)

Machine: aarch64 Linux, GCC 16.2.1, CMake 4.4.3, ninja 1.13, Python 3.14. The smoke ran with GCC
only: no clang is installed on this machine, and GCC prints "unrecognized command-line option
'-Wno-…'" notes for the clang-only flags in the port's list, which are not fatal.

```sh
# resource index headers (no disc; the decomp fork is fetched at b09eebc)
source-port/native/tools/gen_assets.sh --res-only \
    --decomp /home/fred/projects/phase2-prep-work/decomp-ci \
    --out /home/fred/projects/phase2-prep-work/assets/GZLE01        # 614 headers

# RecompCore checkout for native/dsp_hle (elliotttate/RecompCore, the pin fetch_recompcore.sh uses)
git init ref-recompcore && git fetch --depth 1 origin 8ab24daee9c641634fda5cac30389ad4b2cfda5e

cmake -S source-port/native -B <build> -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCOS_ASSETS_DIR=<res headers> -DCOS_RECOMPCORE_DIR=<ref-recompcore>
ninja -C <build> cos_sdk_smoke cos_pc_tests
<build>/cos_sdk_smoke      # prints "ok", exit 0
<build>/cos_pc_tests       # prints "ok", exit 0
```

Results: configure clean (Aurora `3227d76` fetched and patched with `native/patches/aurora`);
build 0 errors; `cos_sdk_smoke` → `ok` (rc 0, with the expected logged notes about SI/VI on the
host); `cos_pc_tests` → `ok` (rc 0). Not run: `switchwaker` (it needs the game modules and
disc-generated asset headers; no disc is involved in this import).

## What this proves, and what it does not

Proves: the imported tree configures, builds and passes its SDK/harness smokes on a GCC aarch64
host; provenance, licenses and attribution are clean and verifiable (`sha256sum -c`).

Does not prove: that the game builds or runs (asset headers from a disc are needed), the ILP32
corrections (census `ilp32-census-c415b56e`, still to apply), any Vita build, or anything about
performance.

## Next

- C2 corrections: `ilp32-census-c415b56e` (boutisme obligatoire `endian.h:24-27`, heaps,
  `CMemBlock` (a)/(b) decision, layout checks) — a separate ticket, ~5-10 days plus scenario
  verification.
- C3 bounded GXM proof: `PLAN_C3.md` in the same sealed directory.
- Independent review of this branch (PLAN_IMPORT step 7), then the merge decision (step 9).
- No line was added to `PRIORITIES.md` or `GOAL_LOOP.md`: both are the ranked, player-facing plan
  for the current releases, and this import is an internal step of the Vita track; it is recorded
  here and in `source-port/PROVENANCE.md` instead.

## Review corrections

The independent review of this branch (CHANGES REQUIRED: B1 plus hygiene O1-O5) is applied in a
follow-up commit: B1 (PROVENANCE header refreshed from the regenerated manifest, with the refresh
rule), O1 (`RIGHTS_AND_LICENSES.md` no longer says the files are unmodified), O2/O3
(`native/README.md` and `native/LICENSE` imported and manifested), O4 (`LC_ALL=C sort` in the
import script) and O5 (the GCC-only note above).

## Commits on this branch

| Commit | What |
| --- | --- |
| `d90d23b` | import the source base at `04372277d0` (script, provenance, manifest, RIGHTS entry) |
| `8d21bd3` | build glue: gate the clang-only `-ferror-limit` flag for GCC |
| `e114843` | import the Aurora patch set the harness needs; `-Wno-changes-meaning` for GCC |
| `bdd8b4b` | build glue: `-Wno-narrowing` for GCC |
| (this file) | the journal |
| `251c58c` | review corrections: `native/README.md` and `native/LICENSE` imported, manifest and PROVENANCE header refreshed (B1, O1-O5) |
