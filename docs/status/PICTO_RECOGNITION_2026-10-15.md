# Picto Box recognition, October 15, 2026

BlueWake #334 (jslay88, `bac7fe3`) and RecompCore #36 (`5423b26`, merged as
`57123c6`) answer the game's actor-id read from the current EFB. Previously every
`GXPeekARGB` read returned zero, crediting pixels to the first registered subject.
The host drains pending FIFO work before the first read; the runtime takes one
snapshot for that frame segment and serves the remaining reads from it.

## Local Mac check

M3 Max, macOS 26.6.2, Metal, 1920 × 1440 framebuffer, original 4:3 view,
Smooth Motion off. A copied card and isolated settings/cache were used; stage
select disabled in-game saving. The card was byte-identical after every run.

The game's photo judgment was observed with temporary local diagnostics. Those
hooks, private states, card, image buffers and module are not part of the PR.

| Shot | Result |
| --- | --- |
| Lenzo centered in his studio, normal rendering and warm shader cache | Subject 94, detail 0: Lenzo recognized |
| Turn to the empty wall and take another photo | Subject 0: no subject recognized |
| Repeat the pair with synchronous pipelines as a diagnostic | Same results |

This is recognition evidence from actual gameplay. It is not a local completion
of the three-photo Lenzo quest, nor a widescreen or mobile check.

## Contributor checks

- jslay88: Lenzo accepts the letter photo on Linux ([#333](https://github.com/chrissotraidis/bluewake/issues/333));
  also confirmed the fix on an M4 Pro Mac ([#334](https://github.com/chrissotraidis/bluewake/pull/334#issuecomment-6163798355)).
- pdale-boop: all three Lenzo quest photos accepted on Windows, with deliberate bad
  photos rejected ([#334](https://github.com/chrissotraidis/bluewake/pull/334#issuecomment-6159163583)).

## Remaining first-use follow-up

An isolated empty shader cache exposed a pre-existing renderer behavior: draws
can be omitted while their pipelines compile. That includes the alpha-only clear
and actor-id pass, so the first readback can contain ordinary scene alpha.

[RecompCore#39](https://github.com/chrissotraidis/RecompCore/pull/39) waits for
alpha-only pipelines, keeping ordinary color draws asynchronous. On the Mac it
produces the expected alpha values on the first cold-cache photo and passes the
Lenzo/empty-wall pair. This draft proposes its pin for validation only. Windows/Linux gameplay checks
remain before adopting the follow-up. The source-only integration CI does not
replace those checks. iPhone/iPad and Android recognition remain unchecked.

Issue #267 stays open for its reporter. The opt-in photo crop (#338, RecompCore#37,
merged October 17) is separate from actor recognition.

## First-use regression fixture

`bluewake_gxcore_picto_peek_pixels_test` uses generated rectangles and no game data.
With global synchronous compilation disabled and an empty cache, it checks alpha
IDs 252, 4 and 8 before the first present, snapshot reuse within a frame, and a
new frame returning ID 12. On M3 Max/Metal it fails against the original pipeline
code (reads alpha 255) and passes with RecompCore#39. Source CI builds it on
Windows and Linux; running it requires a graphics device and is manual.

After configuring the relevant host with its regression tests enabled:

```sh
cmake --build build/linux-host --target bluewake_gxcore_picto_peek_pixels_test
picto_dir=$(mktemp -d)
DOL_AURORA_USER_DIR="$picto_dir/user" DOL_AURORA_CACHE_DIR="$picto_dir/cache" \
AURORA_SYNC_PIPELINES=0 build/linux-host/bluewake_gxcore_picto_peek_pixels_test
```

On Windows PowerShell, using the Windows host build directory:

```powershell
cmake --build build/windows-host --target bluewake_gxcore_picto_peek_pixels_test
$pictoDir = Join-Path ([IO.Path]::GetTempPath()) ([Guid]::NewGuid().ToString())
$env:DOL_AURORA_USER_DIR = Join-Path $pictoDir 'user'
$env:DOL_AURORA_CACHE_DIR = Join-Path $pictoDir 'cache'
$env:AURORA_SYNC_PIPELINES = '0'
& build/windows-host/bluewake_gxcore_picto_peek_pixels_test.exe
```

Use a fresh directory for every run; the fixture rejects a nonempty cache. Record
the renderer and PASS/FAIL, then check a first good and bad game photo using a copy
of a card. Run in a separate shell so the fixture environment does not carry into
normal play.

## Gallery alignment

[#346](https://github.com/chrissotraidis/bluewake/pull/346) merged as `dc45b43` after
green Windows/Linux CI and contributor gameplay checks at 16:9, 16:10 and 21:9.
It has no runtime dependency on the crop.

The [M3 Max/Metal visual check](https://github.com/chrissotraidis/bluewake/pull/346#issuecomment-6179086666)
uses a freshly built module, host `14bc288` and
runtime `57123c6`. Real photo capture and the keep-photo prompt lead to a
three-photo gallery: center and side photos align with their frames at all three
aspects, with left/right navigation endpoints captured. All three sessions exit 0
and leave the copied cards unchanged. Stage select equips the camera in memory
and disables saving. These static captures establish alignment and navigation
endpoints, not frame-by-frame animation or mobile acceptance.
