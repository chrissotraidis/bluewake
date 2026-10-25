# GZLE01 Cold-Boot Reference Oracle - 2026-08-25

This oracle was recorded with Dolphin 5.0-17995 from the user's private
`GZLE01-disc-image` alias. No disc data, generated game data, or captured
emulator state is tracked. The frame/audio artifacts remain ignored under
`local-research/evidence/dolphin-oracle-20260825-run5/`.

## Visual milestones

Dolphin reported 60 VPS and 30 rendered FPS during the cold boot. The retained
files are indexed by Dolphin's dump sequence; conversion from the audio time
to a 60 Hz VI index is an inference, not a direct read of the guest retrace
counter. Observed dumped-frame indexes are:

| Milestone | First dumped frame |
|---|---:|
| Black startup | 1 |
| Nintendo mark visible | 66 |
| Nintendo red pixels | 72 |
| Dolby mark visible | 176 |
| Title scene first nonblack fade | 475 |
| Title sky fully visible | 483 |
| Title logo red begins | about 611 |
| `Press Start` first readable | about 750 |

Contact sheets are private evidence. Their SHA-256 values are recorded with
the blocker ledger rather than the files themselves.

## Audio milestone

Dolphin's 32,028 Hz stereo DSP dump contains its first nonzero sample-frame at
index 305,765 and its first sample with absolute magnitude greater than 16 at
305,948. These correspond to 9.5468 and 9.5525 guest audio seconds, or about
video retraces 572.8 and 573.2 at 60 Hz. The 48,042 Hz DTK dump remains zero
through the retained run. This places the first audible DSP payload after the
title fade begins and before `Press Start` becomes readable.

## BlueWake comparison

The corrected-clock headless route stops normally at exactly 500 retraces
without executing the linked title scene (`title_ready=0`). A second bounded
route stops normally at 600 retraces after 24,481 donor DSP DMA callbacks;
none contains a nonzero byte and no retail `BankMgr::noteOn` is observed.

The earliest named divergence is therefore scene progression before the title
scene, not PCM production. Retail source shows `dScnLogo::dvdWaitDraw` gating
the opening-scene request on completion of the boot resource synchronization
chain. The next experiment must identify the incomplete command in that
predicate. It must not bypass the predicate, fabricate completion, inject
audio, or reopen heap/ARAM tracing.

## Artifact identities

- BlueWake 500-retrace log: SHA-256
  `29cc8ae3ec335f5570f6701dea4d4a818aba126a5abe2186d4e685b0430b26ee`.
- BlueWake 600-retrace log: SHA-256
  `c44c028ed281585004c58043bba8c951f162ec5172bcfd5b116125c6ac50e11c`.
- Dolphin DSP WAV: SHA-256
  `fda467f491e9f0fb082e109d7fd0d6ad5b8a9e2390bbd5b264e39c668f3d3e89`.
- Dolphin DTK WAV: SHA-256
  `4ebceb867ffd3beb620a626616c6aa3961b12a8b9c4ef8cfdd56e789e5e7ae30`.

No simulator was used.
