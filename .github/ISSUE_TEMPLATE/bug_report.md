---
name: Bug report
about: Report a build failure or an issue while playing
---

## What happened?

Describe what you expected and what happened. Include steps to reproduce it.

## Your setup

- BlueWake source revision or app build:
- Device and operating system/version (iOS/iPadOS, macOS, Windows or experimental tvOS):
- For desktop performance/crashes: CPU, GPU and graphics driver version:
- For Apple build failures: Mac model, Xcode version and target SDK:
- Build: default (local training) or `--no-train`:

## If this happened while playing

- Area, quest log and approximate time of the problem:
- FPS and game speed shown in the overlay:
- Render resolution, aspect ratio and enabled mods:
- Touch controls or controller:
- Audio mode (HLE/default or LLE), Smooth Motion setting and enabled gameplay extras:
- Fresh installation/new game or updated app/existing save:

## If this happened after changing settings

- Which settings changed, and did you restart afterward?
- Does it happen on every launch or only in a specific scene?
- Last relevant log lines/error, if available:

Do not delete the app, memory card or settings to troubleshoot. Back up saves before an
update; include reproduction steps using a separate test configuration if you have one.

On iPhone/iPad, **Help & Feedback › Report a Problem on GitHub** fills in device
information. **Share Session Log**, in the same menu, exports the session log.
On Windows, session logs are in `%APPDATA%\BlueWake\logs`.

## If the build failed

Include the build command, failing stage and relevant error lines. Stage logs
are under your build directory's `logs`; training logs are in `pgo-local/logs`.

Review logs for personal paths before posting. Do not attach a disc image,
app/IPA, translated source/module, save, texture pack or generated game profile.
