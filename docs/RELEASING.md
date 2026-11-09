# Releasing BlueWake: the checklist

Every release follows this list, in order, and its record (`docs/status/RELEASE_X.Y.Z.md`, started from the previous
one) gives each line a status and its evidence. A line that doesn't apply says why. The release rules themselves
(what may contain game code, who publishes) are in [AGENTS.md](../AGENTS.md#releases); only Chris publishes.

Most lines below were added after something was missed. When a release misses something, add the check here in the
same pull request as the fix, with the release it came from.

## Before freezing

1. **Scope.** The record lists what ships, on which platforms, how each change was checked and what wasn't checked by
   hand. Anything that changes gameplay, timing or rendering is off by default unless it was tested on that platform.
2. **Windows.** Everything under "In `main`, for the next Windows build" in [WINDOWS_TASKS.md](WINDOWS_TASKS.md) is
   either checked on the release build or named in the record as not checked.
3. **What changed for PadMint** (missed in 0.11.0). Compare `padmint.json` with the previous release's
   (`git diff vPREVIOUS -- padmint.json`). If a host, target, tool, input or option changed, PadMint's catalog entry
   (`catalog/bluewake.json` in chrissotraidis/padmint) may have to change with it. Run
   `python3 scripts/release/check_padmint_catalog.py`: it compares the recipe's iPhone hosts with the computers
   PadMint's menu offers BlueWake on. If it fails, open the PadMint pull request now, so it can merge and ship in a
   PadMint release on the day BlueWake publishes. A new or changed host is labelled experimental until a run is
   recorded.

## Before tagging

4. **Artifacts.** Every file passes `scripts/release/check_public_assets.sh` and PadMint's `audit`. Windows and Linux
   builds have only the accepted `containsTranslatedGameCode: true` finding; Mac, iPhone and iPad files have none.
5. **PadMint player check, with the checkout.** `scripts/player-check.sh` from PadMint's current `main`, with Apple's
   Python 3.9 and an empty PadMint folder, on the clean release checkout ([AGENTS.md](../AGENTS.md)). A failure blocks the
   release. This runs on a Mac only, so it says nothing about the menu on Windows or Linux: line 3 covers that.
6. **Player-facing text.** README and the guides name the new version's downloads, and nothing still promises "the next
   release" for something this release ships:
   `rg -n -i 'next release|from the next|v?PREVIOUS_VERSION' README.md docs --glob '!docs/status/**' --glob '!docs/archive/**'`.
7. **`SHA256SUMS`** made from the files downloaded back from the draft, and `shasum -c` passes.

## After publishing

8. **The tag and the downloads.** The tag points at the release commit, `releases/latest` is the new release, and every
   asset downloads without signing in and matches `SHA256SUMS`.
9. **PadMint player check, without a checkout**, from `releases/latest` (same setup as line 5).
10. **PadMint offers what the release builds.** `python3 scripts/release/check_padmint_catalog.py` passes against
    PadMint's `main`, and a PadMint release that contains the catalog change is out. For a host added in this release,
    someone opens PadMint on that kind of computer and sees BlueWake offered, or the record says nobody could.
11. **Reports since publishing.** On the next day and again after about three days, read every issue and comment since
    the release (`gh issue list --state all --search "updated:>=YYYY-MM-DD"`), PadMint's issues, and what Chris passes on
    from Discord. For each report, the record says whether it's a problem with the release and what follows: a fix, a
    question for the reporter, or nothing. Reply the way [AGENTS.md](../AGENTS.md#talking-to-people-on-github) describes.
12. **Replies to issues the release fixes**, asking the reporter to confirm; close only as AGENTS.md allows.

