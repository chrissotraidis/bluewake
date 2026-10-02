# Building BlueWake for Apple TV

The experimental tvOS target is a controller-first build of the same app, targeting tvOS 17 or newer
with GameController input. The iPhone/iPad touch shell is not included. On first launch, the TV prepares
the game files from the disc image copied into its app container during installation.

Ian MacFarlane (@iannotian) contributed this in PR #3 and reported reaching the title screen on a
physical Apple TV 4K. BlueWake integrates his credited work through PR #6. This is not yet full
controller gameplay, audio, save persistence or performance acceptance. The dependency uses a
retagged pinned Dawn iOS archive as a development bridge, not a native tvOS Dawn build. A complete
couch-friendly in-game settings shell is still pending; do not expect the iPad touch menu here.
PadMint's published target remains iOS only, and public releases remain paused.

If preparing a transferred image fails, BlueWake leaves that image intact and waits for a changed
file or **Check for disc**. It does not delete your ISO or keep retrying the same failed input.

## Requirements

- A Mac with Apple silicon, Xcode, CMake 3.25 or newer, and Ninja.
- Your own uncompressed GZLE01 USA revision 0 disc image.
- An Apple TV with Developer Mode enabled and paired with this Mac.
- An Apple Development signing identity and a tvOS development provisioning profile for
  `dev.bluewake.BlueWake` that includes the Apple TV's device ID.

## Build and install

```bash
scripts/tvos/build_device.sh "iso/Legend of Zelda, The - The Wind Waker (USA).iso" \
    --no-mods \
    --identity "Apple Development: Your Name (TEAMID)" \
    --profile "/path/to/tvOS.mobileprovision" \
    --install "<Apple TV device ID>"
```

The builder validates the disc, translates its game code locally, builds and signs a tvOS app, installs
it, copies the ISO into `Library/Caches/BlueWake/GZLE01.iso` in the app's private container, then opens
the app. This Apple TV rejects writes to
Documents and Application Support, so the tvOS app uses Caches. Apple may purge Caches while the app
isn't running ([file-system guidance](https://developer.apple.com/documentation/foundation/using-the-file-system-effectively)); keep a backup of the ISO and copy `GZLE01.card` off the Apple TV regularly. Keep the app and its translated game code private; do not upload or share either one.

If the app opens before the transfer completes, its first-run screen watches for the file and imports
it automatically. You can also copy a disc image later with:

```bash
xcrun devicectl device copy to --device "<Apple TV device ID>" \
    --domain-type appDataContainer --domain-identifier dev.bluewake.BlueWake \
    --source "/path/to/GZLE01.iso" --destination "Library/Caches/BlueWake/GZLE01.iso"
```

Saves are stored in `Library/Caches/BlueWake/GZLE01.card` in that same app container. Copy the card
off the Apple TV after a play session:

```bash
mkdir -p "$HOME/Library/Application Support/BlueWake"
xcrun devicectl device copy from --device "<Apple TV device ID>" \
    --domain-type appDataContainer --domain-identifier dev.bluewake.BlueWake \
    --source "Library/Caches/BlueWake/GZLE01.card" \
    --destination "$HOME/Library/Application Support/BlueWake/AppleTV-$(date +%Y%m%d-%H%M%S).card"
```
