# Importing textures from Wind Waker HD

BlueWake can use compatible model textures from your own USA Wii U
Wind Waker HD disc (`BCZE`, title `0005000010143500`). The importer converts
them into the same Dolphin-format replacement pack that Aurora loads in
Dusklight and BlueWake. Your GameCube `GZLE01` revision 0 disc is
still required to play.

## Where it is in the app

Mac, Linux, Windows, iPhone and iPad share the same section, first in their Mods tab or menu:
**Mods › Wind Waker HD textures**. It holds the **HD textures** switch, how
many textures the chosen pack has, and a way to add a pack:

- **Mac, Linux and Windows:** F1 › Mods › **Choose texture pack folder…**.
  Windows also has **Open the texture folder**.
- **iPhone and iPad:** ⋯ › Mods › **Install Texture Pack…**, which copies the
  pack into the app.

## Import in the app

The native importer is in `main` since October 17 ([#315](https://github.com/chrissotraidis/bluewake/pull/315)),
not yet in a download (0.7.0 doesn't have it). Its entry is **Mods › Wind Waker HD textures › Import
from Wind Waker HD…**: F1 on Mac, Linux and Windows, or ⋯ on iPhone and iPad.
Android integration is in [draft #347](https://github.com/chrissotraidis/bluewake/pull/347). The app needs no Python.

1. Choose **Folder…** for your decrypted Wind Waker HD dump: the folder that
   holds `code`, `content` and `meta` (as Cemu uses it), or its `content` folder.
   Alternatively, choose **WUD/WUX Image…**, then the disc key and either a
   common key or a decrypted title key. Each key can be 32 hexadecimal digits
   typed or pasted into the form, or a key file. A keys file for the emulators
   (`keys.txt`: one key per line, `#` comments, for every title) works as it is:
   BlueWake tries its keys and keeps the one that decrypts this disc, so there
   is no line to pick. Key selections are used only for this import and are not
   saved in settings. Cancelling a key picker returns to key selection. On iPhone/iPad,
   cancelling the initial folder/image picker ends the flow; reopen **Import
   from Wind Waker HD…** to start again.
2. BlueWake reads it with the GameCube disc it already plays and makes the pack
   in a new folder, preserving existing packs: `Load/Textures/WWHD` in the
   app's data folder (`WWHD-2` and so on if it exists).
3. On desktop, the menu shows progress; you can close it and play while the
   import works, or cancel the job. When it finishes, reopen Mods to see the
   result. The new pack is selected and **HD textures** is on. Resume the game
   to save the choice, then restart BlueWake to load it.
4. On iPhone/iPad, the game pauses behind an import progress sheet. **Cancel**
   keeps it paused while the current read and cleanup finish. Switching apps
   during import also requests cancellation; the result appears after returning.
   When the import completes, the app offers to turn on HD textures for the next
   launch. Restart to load the pack. This path still needs a full-dump test on
   hardware and an interactive check of cancellation/background transitions.

The native importer implements the script's matching and conversion: from
the supplied decrypted HD folder and GameCube disc it makes the same 3,546
textures, in about 30 seconds on an M3 Max. On Linux (Arch, X11, NVIDIA),
pdale-boop imported his own dump's `content` folder from the menu, from a path
with spaces and Japanese characters: 3,546 keys and 33,859 PNGs, the same counts
as the Mac, and the restart loaded all 3,546. Cancelling an import kept his
previous pack selected and left no partial pack. The image reader passes
generated encrypted-disc checks, including a Mac menu import. On the iPhone 17e
simulator, a test app running the production iOS menu and importer (without
the game) imported generated inputs through Files, as a folder and as a WUX
image with key files; cancelling a key picker kept the other choices. An
authentic WUD/WUX comparison, a Windows interactive import, and iPhone/iPad
hardware, memory and lifecycle checks remain open. These are distinct from
build and simulator fixture results below.

The output follows the app's data folder, including Linux portable mode and
an explicitly selected desktop settings location.

## The import script

The script makes the same pack from a computer, and also reads WUX and WUD
images with their key files.

## Set up the importer

From the repository root:

```sh
scripts/setup_wwhd_tools.sh
```

This creates a local Python environment in ignored `build/wwhd-tools`, installs
the versions pinned in `scripts/wwhd/requirements.txt`, and builds a small Yaz0
decoder if a C compiler is available. Without a compiler the Python decoder
works, but takes longer. No game data or keys are downloaded by this script.

On Windows, with Python 3.11 or newer installed, run this from PowerShell in the
repository instead (it builds the decoder with Visual Studio's C compiler when
one is installed):

```powershell
powershell -ExecutionPolicy Bypass -File scripts\setup_wwhd_tools.ps1
```

and use `build\wwhd-tools\Scripts\python.exe` wherever the commands below say
`build/wwhd-tools/bin/python`.

## Import your discs

For a WUX/WUD image, supply your disc key and a common key file. Keys are
16-byte binary files or text files containing 32 hexadecimal digits. The app
also takes a `keys.txt` for the emulators, with every title's key in it, and
picks the one that decrypts your disc; this script reads one key per file, so
give it that disc's own line. The importer decrypts the disc's own embedded
ticket to obtain its title key.

```sh
build/wwhd-tools/bin/python scripts/import_wwhd_textures.py WindWakerHD.wux \
  --gc-disc WindWakerGC.iso \
  --disc-key game.key --common-key common.key --install
```

The image is read directly, so a 25 GB expanded WUD is unnecessary. The
importer validates the disc identity, file table checksum and each hashed
content block it reads. It also detects incomplete WUX extraction before
reading any texture content.

`game.key` and `common.key` beside the HD image are used by default. If you
already decrypted that disc's title key, use `--title-key title.key` instead
of `--common-key`. A NUS title key may differ from the disc's title key even
when the title ID is the same. A `.tik` file contains an encrypted title key;
it does not substitute for the WUD disc key.

For a decrypted HD `content` folder, no key files are needed:

```sh
build/wwhd-tools/bin/python scripts/import_wwhd_textures.py /path/to/content \
  --gc-disc WindWakerGC.iso --install
```

On a Mac, the default GameCube disc is the one inside your BlueWake app
(`/Applications/BlueWake.app/Contents/Resources/Game/GZLE01.iso`, or the same
place under `~/Applications`), or else the ISO Wind Waker Recomp imported
(`~/Library/Application Support/Wind Waker Recomp/GZLE01.iso`). You may omit
`--gc-disc` when one of those exists. On Linux it is the disc BlueWake
remembers (`~/.local/share/BlueWake/disc.txt`). On Windows it is the `.iso` or `.gcm` that
BlueWake remembers (`%APPDATA%\BlueWake\disc.txt`), or the ISO it unpacked
from a Dolphin `.rvz` (`%APPDATA%\BlueWake\GZLE01.iso`).

## Keep an existing pack as fallback

Add `--fallback-pack /path/to/existing-pack` to the import command to keep
existing HD artwork for anything the disc importer does not replace:

```sh
build/wwhd-tools/bin/python scripts/import_wwhd_textures.py WindWakerHD.wux \
  --disc-key game.key --common-key common.key \
  --fallback-pack "/path/to/ZWW4K 1.0.0d (4K)" --install
```

The importer searches nested texture folders, so the containing ZWW4K folder
is acceptable. Wind Waker HD artwork takes priority by source texture key,
regardless of PNG/DDS extension or mipmap naming. Only absent keys and their
own mipmaps are copied into a `fallback` subfolder. The original pack is
unchanged. The title copyright texture is excluded from fallback imports so
the disc's Nintendo copyright remains instead of a texture-pack banner. For example, existing HUD replacements can remain active while
compatible character and environment textures come from the HD disc.

## Use the result

On a Mac the default output is
`~/Library/Application Support/BlueWake/Load/Textures/WWHD`.
`--install` selects that folder in the app's settings
(`~/Library/Application Support/BlueWake/settings.ini`) for the next launch
and backs up the previous settings. Restart the app to load it. You can also
omit `--install` and pick the output folder with **Mods › Wind Waker HD
textures › Choose texture pack folder…**. Turn **HD textures** off to go back
to the GameCube textures.

On Linux the default output is `~/.local/share/BlueWake/Load/Textures/WWHD`
(`$XDG_DATA_HOME` when set), and `--install` writes the same setting to
`~/.config/BlueWake/settings.ini` (`$XDG_CONFIG_HOME` when set).

On Windows the default output is `%APPDATA%\BlueWake\Load\Textures\WWHD`.
Close BlueWake first: `--install` turns on **HD textures** and points it at
that folder in `%APPDATA%\BlueWake\settings.ini` (`hd_textures=1`,
`texture_pack=...`), keeping a backup of the previous file, and BlueWake
rewrites that file when its settings change. The settings menu (F1, Mods)
then names the pack's folder, with a button to go back to
`Load\Textures\GZLE01`.

Use `--output /path/to/new-folder` to choose another output folder. Existing
folders are preserved; the importer requires a new destination. For an
iPhone/iPad personal build, choose the generated folder with **Mods › Install
Texture Pack…**, turn on **HD Textures**, and restart.

The pack contains PNG textures, `_mipN` mipmaps and `import-report.json` with
source checksums, tool/dependency records, matches and skipped conflicts.
When a fallback is supplied, its selected keys and file checksums are also
recorded in the report. Unsupported fallback filenames are listed there.
The detailed report is local game-derived metadata; keep it with your private
pack. Never include the resulting textures, keys, tickets or discs in source
commits or public app releases.

## Scope and matching

The importer reads GameCube RARC archives, BTI textures and J3D TEX1 model
chunks, and HD Yaz0/SARC archives, Wii U BFRES v3/v4 FTEX textures, BFLIM
images and FFNT v3 bitmap fonts. Both Picto Box inventory icons and the water bottle have verified
mappings from the HD item layout to the GameCube item archive. The transparent
English HD title logo and its HD badge are fitted to the original logo UVs.
Other redesigned UI images are
not matched by their numbered filenames. For model textures it
matches both the resource identity and texture name; stage rooms are scoped
by stage and room, including the HD `sea_Room0` naming convention. It then
checks that the replacement preserves the original aspect ratio and is at
least the original resolution. The replacement filename hashes the original
GameCube texture bytes, and for indexed textures only the palette entries
those bytes reference, exactly as the renderer does.

The decoded HD component selections and source mipmaps are preserved, with
GameCube I4/I8 replacements using intensity for alpha as the original renderer
expects. Missing levels down to 1x1 are generated. Conflicting artwork for the same GameCube
texture key is skipped, except for explicitly audited shared-artwork variants.
Those choices preserve the original UV composition instead of HD debug-room,
figurine or photographic-cloud variants. Selected alternatives are recorded
in the report. Tiny placeholders and shader ramp textures stay
original. Different UV layouts or changed names need separate, verified
mappings and are not guessed.

This imports compatible model artwork. Wii U models, shaders, lighting,
normal/bake maps, redesigned HUD layouts and gameplay are not imported.
The HD camera mode indicators, minimap frame/check textures and BFRES
Japanese title layers use different transparency or layout rules and are excluded;
the English title uses the transparent HD layout images instead. An existing
fallback pack supplies other compatible missing textures. Verified BFLIM mappings
cover inventory bottles, equipment, trading items, collection icons and charts.
Forest Water's animated sparkle remains supplied by the GameCube game. HD HUD
hearts combine their separate outline and fill into the original sprite's UV
area. The swimming HUD's matching masks and decorations are also imported;
layout masks combine luminance and alpha for GameCube I4/I8 sampling.
Verified English action labels, inventory/quest/save headings, quest-log labels,
item counters, rupee digits and dungeon-floor labels are rebuilt from the HD
disc's own font glyphs. Each sprite retains its original alpha bounds and
transparent padding. A/B/X/Y symbols use the corresponding HD pictograph
font; redesigned L/R/Z and stick prompts retain their existing artwork.
The small HUD key and menu cursor corners are fitted the same way. Lower
resolution HD item-get sprites are not substituted for sharper originals.
The six opening story pictures are cropped to the original composition,
including splitting HD's panorama into the two original sprite planes.
RGBA8 and BC1-5 layout surfaces preserve color, luminance and alpha;
unmapped redesigned HUD images and JPA particle textures remain
unsupported. Unsupported resources and textures are listed in the report.

## Source and verification

The bounded format readers use the layouts documented by
[BFRES-Tool](https://github.com/aboood40091/BFRES-Tool),
[BFLIM-Tool](https://github.com/aboood40091/BFLIM-Tool), and
[JNUSLib](https://github.com/Maschell/JNUSLib). FFNT section layouts are also
documented by [3dstools](https://github.com/ObsidianX/3dstools/blob/master/bffnt.py);
the importer contains its own bounded reader for the audited BC4-alpha and
RGBA8 font surfaces, including GX2 array-page bank rotation. The Wii U address library is
vendored from BFRES-Tool at the exact revision in
`scripts/wwhd/vendor/SOURCE.json`, with its GPL-3.0-or-later license and
copyright notice. The importer has no GUI toolkit dependency.

Generated fixture checks cover decoding, palette hashing, stage/room scope,
malformed inputs, encrypted content integrity, conflicting replacements and
settings preservation, intensity-mask transparency, fallback priority and matching mip chains,
font array pages and malformed section chains, sprite-mask padding and panorama crops:

```sh
build/wwhd-tools/bin/python tests/wwhd_import_test.py
```

Checked by hand on October 12, 2026, on a Mac (M3 Max): a decrypted USA dump's
`content` folder with a revision 0 GameCube ISO gave 3,546 replacements (124
conflicting keys skipped). BlueWake loaded all of them at Outset, with the
Wind Waker HD look on and off; an unpaced run took 9.3 s against 9.2 s without the pack.

### Native importer memory check (October 15, 2026)

The importer now feeds PNG scanlines to zlib one row at a time, caps queued
and active PNG image data at 32 MiB (one larger image may run alone), and
shares unchanged mip chains across GameCube formats. Archive traversal borrows
uncompressed containers. The English layout pack retains only the heart
outline and HD badge needed by other members, then processes members in their
original order instead of keeping every member alive.

On the same M3 Max, using the same decrypted USA HD tree and revision 0
GameCube disc, a standalone native import with four PNG workers measured:

| Version | Peak resident RAM | Wall time |
| --- | ---: | ---: |
| PR #315 at `6232826`, before this change | 993 MiB (1,041,235,968 bytes) | 26.49 s |
| With streaming and shorter buffer lifetimes | 538 MiB (563,806,208 bytes) | 28.94 s |

Measured with `/usr/bin/time -l`, compiled with Apple Clang at `-O2`.
Peak RAM fell 46%. Other work was running on the Mac, so these times are not
an isolated speed benchmark. These are importer process measurements, not
the combined memory of an app importing while a game runs or an iPhone/iPad test.

Both native runs produced the same 3,546 texture keys, 33,861 filenames,
124 skipped conflicts and 17 unsupported entries. All 33,859 PNGs, including
every mip level, were byte-identical; matching metadata and all 91 generated
HUD/title textures were unchanged.

A fresh Python import from those inputs also matched every PNG's decoded
pixels and every texture chain's SHA-256, with the same matching metadata,
conflicts and unsupported entries. The native exact-report run produced the
same PNG bytes as its four-worker run, using 394 MiB peak resident RAM
(413,024,256 bytes) in 143.87 seconds with serial writing and SHA-256 enabled.

`tests/wwhd_native_test.cpp` checks generated nested SARC/RARC/Yaz0 archives,
empty members, malformed inputs and callback errors, plus streamed PNGs
against whole-image zlib compression and decoded pixels. It passes with
AddressSanitizer and UndefinedBehaviorSanitizer on this Mac. Build the development
importer and disc checks through the configured host build, so they use
the same pinned disc-crypto dependencies as the app. For an existing Mac
builder tree (replace `BUILD` with its CMake build directory):

```sh
cmake -S scripts/builder/training -B BUILD \
  -DBLUEWAKE_WWHD_TEST_PYTHON="$PWD/build/wwhd-tools/bin/python"
cmake --build BUILD --target bluewake_wwhd_native_import \
  bluewake_wwhd_disc_image_test bluewake_wwhd_disc_crypto_test \
  bluewake_wwhd_wiiu_disc_test bluewake_wwhd_import_job_test
ctest --test-dir BUILD/host --output-on-failure -R '^bluewake_wwhd_'
```

The test Python needs only `pycryptodome==3.23.0`; the app needs no Python.
Windows and Linux CI compile and run these generated-input checks too.
For a development import, run `BUILD/host/bluewake_wwhd_native_import`
with the HD content folder (or disc image), GameCube ISO and a new output
folder, in that order. An image can take three additional paths: disc key,
common key, title-key override (use an empty argument to skip the common
key). `game.key` and `common.key` beside the image are the defaults. Key
values never go in the command line. `WWHD_EXACT_REPORT=1` selects the slower,
serial SHA-256 report.
To compare native or Python packs, including every PNG and all available
RGBA-chain SHA-256s:

```sh
build/wwhd-tools/bin/python tests/wwhd_compare_packs.py OLD-PACK NEW-PACK
```

Keep both packs and their reports private. Windows/Linux interaction and
imports on phone/tablet hardware still need their own checks.

### Mac in-app check (October 15, 2026)

On the M3 Max with Metal, the native import was started through F1 > Mods
from the supplied decrypted USA content folder while a copied GameCube save
was running at Outset. Closing the menu let gameplay continue during the
conversion. The menu reported 3,546 textures and selected the resulting pack
in the isolated session's data folder. After selecting that installed pack,
resuming to save the preference and restarting, the renderer registered all
3,546 replacements; the HD HUD and textures were visible and movement worked.
The copied card's SHA-256 was unchanged.

The complete process peaked at 1,400,389,632 bytes (1,336 MiB) resident RAM,
sampled every 250 ms across startup, gameplay and import. This includes the
game and renderer and is not directly comparable to the standalone importer
table. It does not establish memory acceptance on iPhone, iPad or Android.

Keyboard navigation was used for the menu and the native folder picker.
Mouse clicks did not consistently activate the ImGui controls through the
automation surface; this run does not establish desktop mouse acceptance.

### Stopping an import

The desktop background job is owned until its worker exits. Closing the app
cancels and joins the worker before destroying its progress/status state;
completion, cancellation, a second import, missing input and shutdown are
covered by `bluewake_wwhd_import_job_test` on the desktop build paths. This
regression fails against the earlier detached worker and passes with AddressSanitizer
and UndefinedBehaviorSanitizer on the Mac. It uses generated inputs and a
controlled worker; it is not a mobile lifecycle or full-disc cancellation test.

Cancellation now also reaches folder and GameCube scans, archive members,
disc blocks, texture/mipmap work and PNG workers. It clears queued PNG jobs,
joins workers before cleanup, and checks again immediately before publishing
the finished folder. Once that rename succeeds, a late Cancel still reports
success. Files are explicitly closed and checked before publication so a
buffered write failure cannot silently produce an incomplete pack.

Generated checks on the Mac cover 23 cancellation points, both real PNG-worker
wait paths, cleanup and existing-pack preservation, late cancellation after
publication, and two buffered-close failures. Seven CTests, six ASan/UBSan
checks and two TSan checks pass. A fresh full import after these changes still
produces all 3,546 keys and 33,859 byte-identical PNGs, including every mip and
the 91 generated HUD/title textures; matching metadata is unchanged. The
complete iOS Simulator app also links with the changes. The iOS operation's six Foundation cases cover
success/error, cancellation before starting, while waiting for a provider,
during conversion, and after a completed commit. They verify grant lifetime,
suppression of late progress and one completion; they also run in the Mac CI
audit under sanitizers. They do not establish UIKit or Files-provider acceptance.

Cancellation cannot interrupt an OS read that is already stalled. Abrupt process
termination bypasses exception cleanup; the output recovery below handles the
new marked staging folders on the next import into the same parent folder.

A further output check rejects the import if removing a conflicting texture or
replacing its old mipmaps fails. Previously those filesystem errors were ignored,
so rejected artwork could survive in a completed pack. A generated three-texture
fixture reproduces the old success-after-failure result. The fixed importer
passes conflict pruning and both synchronous/threaded removal-failure cases on
the Mac under ASan/UBSan, preserving existing packs and removing staging. The
same generated checks run in Windows/Linux CI. Those checks exercise handled
failures; forced-process recovery has separate coverage below.

### Recovering interrupted output (October 15, 2026)

The importer holds a nonblocking OS lock for its output parent until all work
and cleanup finish. Another import using that parent refuses promptly.
The lock is an empty hidden file, `.bluewake-wwhd-import.lock`, in
`Load/Textures`. It stays after BlueWake exits, by design: removing it after
each import would let a second import create a new file at the same name and
get past the lock. It can be deleted while BlueWake isn't running. New
temporary output has a versioned ownership marker written before any texture
files. A later import recovers only that recognized, flat output; it leaves
completed packs, older unmarked folders, unexpected contents and links alone.
Cleanup keeps the marker until the payload has gone, so interrupted recovery
can be retried. A completed pack remains successful if only wrapper cleanup
fails; a cleanup problem before completion is reported with the original error.

Nine Mac CTests pass under ASan/UBSan, including real killed-child tests for
partial output, interrupted recovery, a completed rename, live contention,
Unicode paths and preserving unrelated files and link targets. The helper
also compiles for iOS device/simulator and Android arm64 API 28. Windows/Linux
CI includes these tests. Android's external storage rejects `flock`, so Linux
uses an open-file-description lock when that operation is unsupported. The
same generated helper now passes on an API 36 ARM64 emulator's internal and
external storage, including same-process contention, publication and lock
lifetime. This shell-run check does not establish app storage permissions or
menu acceptance. Windows first passed 70 of 71 checks; its recovery fixture
tried to read the locked byte and now compares the empty lock's metadata
instead. The new Windows/Linux CI results remain pending. Mobile input-copy
recovery is separate, and app-level force-close/retry remains unverified.
This does not promise recovery after power loss or remove older unmarked
temporary folders automatically.

### Native iOS Simulator check (October 15, 2026)

An isolated test app linked all eight production importer source files from
`a640108` for arm64 iOS Simulator 27.0 (iPhone 17e), using the iOS path's two
PNG workers. Seven generated-input cases passed: empty/missing HD folders,
missing GameCube disc, malformed-resource cleanup, successful import, refusing
to overwrite an installed pack, and repeat import to a new folder. Both packs'
12 PNGs passed independent CRC, dimension, pixel and mip-chain checks. The
production `BWGameOverlay.mm` also compiled without diagnostics.

This checks the importer in an actual Simulator process. The production menu,
Files picker, security-scoped external folder access, alerts, saved preferences
and next-launch renderer loading were not part of that harness. No game data
or physical device was used; full-dump iPhone/iPad memory acceptance remains open.

A separate full production simulator build from `a640108` with RecompCore
`ee99bbc` also linked and reached Outset on the iPad Air 11-inch (M4) / iPadOS
27.0 simulator. The game and touch overlay were visible. Its distinct test
bundle used a copied card, whose hash stayed unchanged. Device Hub returned
`noWindowsAvailable` for automation input, so this run did not verify the Mods
menu or file picker. The test app was uninstalled and the simulator shut down;
the physical iPad was not touched.

### Combined runtime check (October 15, 2026)

The native-import branch at `6bc56e5` also builds on macOS and iOS Simulator
with the newly approved Picto recognition pin, RecompCore `57123c6`. Five Mac
host checks passed (import-worker lifecycle, save state, cache flush, frame
interpolation and FIFO worker), as did the alpha, destination-alpha and early-depth
Metal pixel fixtures. A Mac Outset run registered all 3,546 replacements with
the HD look enabled and exited normally; the copied card stayed unchanged.
The temporary cache folder was missing in this smoke run, so SQLite caching
was disabled. This was not a performance measurement or first-photo test.
The exact combined iOS revision was compiled, not played or imported on hardware.

The expanded importer was also executed in a fresh iPhone 17e / iOS 27.0
Simulator fixture app after the WUD/WUX core was added (`46846e2`, with source
hashes checked again at `b23787d`). All fourteen production importer/crypto
units linked. The same seven generated folder cases and twelve PNG checks
passed; the test app was removed and the simulator returned to Shutdown.
This fixture did not execute an encrypted import or the production picker.

The new iOS folder/image/key flow compiles for arm64 device and simulator,
and the complete production simulator app links with RecompCore `57123c6`.
It holds security-scoped access while selecting and importing, and coordinates
all external input reads together on a worker queue using the returned URLs.
A Foundation-only check covers three coordinated reads, a coordination error
that prevents reads, and completion order. It does not test UIKit, iCloud or
another Files provider; those interaction checks remain open.

### Native WUX/WUD progress (October 15, 2026)

The native reader now handles WUD/WUX sector mapping, partitions, the Wii U
file table and title metadata, locally supplied keys, AES-CBC decryption,
and file-table / hashed-content checksums. It reads resources directly into
the existing texture matcher without expanding an entire WUD. The CLI and
desktop/iOS menus can select either a decrypted folder or an image with key files.

The reader's generated fixtures cover reordered/duplicated sectors, reads
above 4 GiB, a 64 GiB logical image backed by less than 9 MiB of test data,
raw/CBC/hashed content across block boundaries, title/common/override keys,
wrong keys, malformed tables and damaged content. The encrypted fixtures are
checked independently by the existing Python reader. Mac CTest and the
parser's AddressSanitizer / UndefinedBehaviorSanitizer run pass. The updated
production Mac and iOS Simulator apps compile and link. Windows and Linux CI
passed for the image/key menu at `ea31b24`; the later cancellation changes need
their own results.

An end-to-end generated fixture also checks folder import against WUD and
mapped WUX in four key modes: nine successful imports have identical PNGs,
with every mip's dimensions, pixels and CRC independently checked. Twelve
failure cases verify wrong-key rejection, existing-pack preservation, and
staging cleanup after corrupt content. Five more imports put the folder/image,
GameCube disc, keys and output beneath accented, Japanese and emoji paths;
their PNGs match the ASCII-path baseline on the Mac. The Windows development
CLI and fixtures now use the app's existing UTF-8 manifest; Windows execution
of these cases remains a CI check. A fresh full import of the supplied
decrypted HD tree still matches all 3,546 keys and all 33,859 PNG bytes from
the earlier pack; this run peaked at 561,643,520 bytes RSS on the Mac.

The Mac menu also completed a WUX import using generated disc metadata and
synthetic keys around one private HD resource. Image/disc/common-key selection,
cancelling and reopening a key picker, completion, pack selection and resumed
gameplay worked. Its nine PNGs matched the CLI output byte for byte; settings
contained no key paths, the report contained no key values, and the copied
card's SHA-256 was unchanged. This used keyboard navigation and native file
pickers. The title-key menu route and a renderer restart with this small pack
were not part of that check.

Still required before advertising image support: an authentic image-to-pack
comparison, a Windows import, image selection on Linux and on iPhone/iPad
hardware (the iOS simulator check used generated files), and phone/tablet
memory and lifecycle validation. Android integration is in draft #347. The supplied reference is a
decrypted folder; generated encrypted fixtures do not establish acceptance
of an authentic disc dump. The committed automated fixtures contain no game
assets or private keys; the Mac menu check's HD resource and output stay private.

### Android core check (October 15, 2026)

At `ea31b24`, the native development importer and three reader/crypto test
executables built for arm64 with NDK r29 (`29.0.14206865`, minimum API 28).
They ran on a fresh Android API 36 ARM64 emulator. Five generated imports
(folder, WUD and WUX with common/title key modes) produced thirty PNGs whose
pixels, CRCs and mip sizes passed the independent fixture checks; each image
pack also matched the folder pack byte for byte. The three reader tests passed,
and a wrong key was rejected without an output folder. Only generated fixtures
were used. The temporary emulator was stopped and removed afterward.

This is core execution, not the Android app, a Files picker, real-phone memory
or gameplay performance. Android #93 at `42610f2` has an existing-pack installer
but no native HD import entry. Its settings also still use one fixed pack folder;
selecting a completed native import must be wired into both launch settings
and the displayed count.

The remaining input adapter must handle document URIs through Android's
[Storage Access Framework](https://developer.android.com/training/data-storage/shared/documents-files),
not treat them as filesystem paths. The
[ContentResolver contract](https://developer.android.com/reference/android/content/ContentResolver#openFileDescriptor(android.net.Uri,java.lang.String,android.os.CancellationSignal))
allows a read-only descriptor to be a pipe. Image imports need a checked,
seekable source or a cancellable local staging copy; they must not request
write access merely to obtain seeking. Folder traversal, key access, cancellation,
storage exhaustion and activity recreation remain integration checks.
