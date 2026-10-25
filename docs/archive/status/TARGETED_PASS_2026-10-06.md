# Targeted follow-up — October 6, 2026

This pass followed the triage handoff. It prepared physical iPad validation and reviewed the
Linux shutdown evidence. No release or experimental-default change was made.

## Physical iPad: installed, gameplay still pending

The M2 iPad Pro (12.9-inch, sixth generation) now has the private host built from
`27dd34d7cd9cbea17a619079a04badb66af7cbf7`, including the Pictobox writeback candidate.
This supersedes the `1d54690` installed-host checkpoint in the earlier handoff.
The existing personal translated module/resources were retained; their translation revision
was not independently established. The app still displays version 0.5.0/build 4, so that label
alone does not identify this private candidate.

Host executable SHA-256:
`91fc4f9abcc4be042e2b09e17ccd647bbef4d1a53b2986ae9bcafe92b74a133a`.

Checked on the Mac and connected physical iPad:

- Incremental iOS host build completed successfully.
- Reassembled personal app was signed and passed strict/deep signature verification.
- Full Documents and Library backups were retained before the in-place installation.
- All 17 selected existing save, SRAM and configuration files matched their backup hashes
  on both pre-install readback and post-install readback.
- Device installation completed successfully. The app/data container identifiers changed;
  rediscover paths before reusing any previous diagnostic environment.

The device remained locked. No gameplay, speaker listening, Pictobox, dungeon-map or controller
acceptance test ran in this follow-up. Installation is not gameplay proof. The earlier measured
intro-audio results remain valid within their recorded scope; they were not repeated here.

Both `BLUEWAKE_DEFER_DVD_COMPLETION` and `BLUEWAKE_CACHE_FLUSH_FALLBACK` remain off by default.
Normal reopening does not enable these experiments. Private app, module, backups and captures
remain local and must not be uploaded.

## Linux evidence and log reporting

The contributor's full session log ends in `double free or corruption (!prev)` after its
performance summary. Previously the summary tool silently omitted this untagged allocator abort.
The reporting repair and detailed runtime/build review are tracked in
[PR #148](https://github.com/chrissotraidis/bluewake/pull/148).

The proposed shutdown repair is plausible, but its exported patch is not yet in the runtime
commit fetched by a fresh build. The supplied performance log does not prove a repaired exit.
A [follow-up was posted](https://github.com/chrissotraidis/bluewake/pull/107#issuecomment-6012081314)
requesting a short post-fix shutdown result and the outstanding current-main/build integration.
No Linux runtime change or port merge was made in this pass.

## Resume with these bounded checks

1. Unlock the iPad and rediscover app/container state. Use scratch copies of saves and settings.
2. Finish the audio candidate's speaker-listening acceptance with mirroring closed. Reuse the
   recorded full-track/next-event evidence; do not repeat a long run merely to produce it again.
3. Check two distinct Pictobox photos, saved-photo reload and the dungeon map. Enable only the
   candidate under test and confirm flags/paths in its startup log. Record visual results separately
   from scripted input or decoder counters. A physical-controller check remains separate.
4. Windows hardware remains deferred. Linux needs the contributor's post-fix evidence and build
   integration before a support decision. No background polling or publication is implied.
