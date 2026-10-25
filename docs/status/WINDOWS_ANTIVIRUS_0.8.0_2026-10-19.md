# Antivirus warnings on the 0.8.0 Windows download (October 19, 2026)

The day after 0.8.0 was published (October 18), players on Discord said the Windows zip failed to download
with "virus detected", while 0.7.0 downloaded fine. One player gave Microsoft Defender's detection name:
`Tecabans.STV!cl`. The rule this led to is in [AGENTS.md](../../AGENTS.md#when-antivirus-flags-the-windows-download).

## What was checked

On a Mac, from the zip downloaded from the release:

- `BlueWake-v0.8.0-windows-x64.zip` is
  `56cc845823ec7e6af19f5306ab7517a511d6bbcaa222cc7f84a437b38c588e2f`, the same as the release's `SHA256SUMS`.
- Its files are the ones 0.7.0 had, plus `d3dcompiler_47.dll` for the Direct3D 11 fallback (#377). That file
  and `dxil.dll` and the Visual C++ runtime DLLs are signed by Microsoft. `BlueWake.exe`, the game module and
  the open-source libraries are unsigned, as in every earlier release.
- `BlueWake.exe` holds no web addresses besides an imgui FAQ link, and no Discord, PowerShell or `cmd.exe`
  strings. Its only `%APPDATA%` use is the settings and saves folder.
- `BlueWake.exe` called itself version 0.1 with no publisher, because `windows/src/bluewake.rc` was never
  updated after the first Windows build. It now takes the version from `version.json`, and the Windows CI
  checks it. The fix reaches players with the next Windows build; 0.8.0 was not rebuilt for it.

Not checked: Defender itself (no Windows PC was used), and VirusTotal.

A player also reported malware named `brimine86.exe` and `denodino.exe` in `AppData\Roaming\com_int_cli_net40`.
No file in the 0.8.0 zip contains those names, and the player didn't say it came from BlueWake. It is treated as
unrelated unless someone shows otherwise.

## The report to Microsoft

- [x] Chris reported `BlueWake-v0.8.0-windows-x64.zip` at https://www.microsoft.com/en-us/wdsi/filesubmission as
  incorrectly detected, with `Tecabans.STV!cl` (done by October 20). Result: not known yet. Don't report 0.8.0
  again; add Microsoft's answer here when it comes.
- October 20: the zip downloaded from the release again still matches `SHA256SUMS`
  (`56cc845823ec7e6af19f5306ab7517a511d6bbcaa222cc7f84a437b38c588e2f`). The GitHub report is
  [#422](https://github.com/chrissotraidis/bluewake/issues/422).

## Signing: the decision still open

Reporting each flagged release works but doesn't scale to frequent releases, because an unsigned file earns
trust on its own and every build starts again. A signed build carries the publisher's trust from one release to
the next.

| Option | Cost | Fits BlueWake today? |
| --- | --- | --- |
| [SignPath Foundation](https://signpath.org/terms.html) | Free for open source | Not the Windows zip: it forbids proprietary code and wants builds anyone can reproduce from public source. The zip holds the game module built from a maintainer's disc |
| [Microsoft's Artifact Signing](https://learn.microsoft.com/en-gb/azure/artifact-signing/quickstart) | About US$10 a month | Individuals only in the USA and Canada; otherwise a registered organisation |
| A certificate from a certificate authority | A few hundred US dollars a year | Yes, with an identity check. It puts the signer's verified name on a file that holds translated game code |
| Players build the Windows game themselves ([DIRECTION.md](../DIRECTION.md)) | Development work | The download becomes open-source code only, which SignPath could sign |

Until one is chosen: report only releases that players say are flagged, and ship ready-made Windows builds
when there is something worth a release, not every week.
