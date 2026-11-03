# Proposals

> Dated record, moved from `docs/PROPOSALS.md` on October 11, 2026. The goal it serves is [DIRECTION.md](../DIRECTION.md) section 1.

Ideas for BlueWake that need agreement between the maintainers (Elliott and Chris) before any work
starts. A proposal here is not a decision or a plan.

## Easier Windows builds without publishing game code

**Status:** proposal, not agreed. Since October 4, BlueWake publishes a ready-made Windows build
([AGENTS.md](../../AGENTS.md)), so this is now about a possible later replacement for it, not a blocker.

**Problem.** Wind Waker Recomp's ready-made Windows apps are what most players use. BlueWake does not
publish builds that contain code translated from the game, so a Windows player builds their own from their
disc. Today that means installing Visual Studio with its clang compiler, Python, CMake and Ninja, then a
build of about 15 to 40 minutes on a fast PC ([BlueWake on Windows](../WINDOWS.md)). Installing Visual Studio is the
biggest hurdle.

**Idea.** Publish only the parts that contain no game code, and let the player's PC compile the rest:

1. A Windows app without the game module (like the iPhone and iPad app-only package).
2. A "build my game" step, in that app or in PadMint, that checks the player's disc, translates it,
   compiles the game module locally and puts it beside the app.
3. The compiler is fetched automatically on first use, so no Visual Studio install is needed.

**Ways to get a compiler without Visual Studio** (desk research, nothing tried on Windows yet):

| Option | What it is | For | Against |
| --- | --- | --- | --- |
| [xwin](https://github.com/Jake-Shadle/xwin) with LLVM's clang | Downloads Microsoft's C runtime and Windows SDK on the player's PC after they accept Microsoft's license | Same toolchain the Windows build uses today; we never redistribute Microsoft files | Another tool to fetch; Microsoft's download terms need reading |
| [llvm-mingw](https://github.com/mstorsjo/llvm-mingw) | A self-contained clang toolchain for Windows (about 180 MB, unzip and run) | Redistributable; BlueWake's Windows compatibility code already compiles with it | Different C runtime from today's build and from the prebuilt graphics library; real porting work |

**Questions to settle first.** Whether Elliott is comfortable with source-only Windows; how long the
compile takes on ordinary PCs; whether a reviewed optimization profile can be shipped so players can skip
the training run; and whether this belongs in PadMint ([PadMint #75](https://github.com/chrissotraidis/padmint/issues/75)
tracks Windows and Linux build hosts) or in BlueWake's own app.

