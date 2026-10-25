#!/usr/bin/env python3
"""Write a Dolphin input movie (.dtm) from BlueWake pad-script entries.

    scripts/make_dtm.py OUT.dtm POLLS ENTRY[,ENTRY...]

ENTRY is poll:buttons:length[:stick_x:stick_y], the BLUEWAKE_PAD_SCRIPT
shape, with GameCube PAD bits for buttons (0x0100 A, 0x0200 B, 0x1000 Start,
...) and signed stick values (-128..127, +y up). Dolphin consumes one
controller state per pad poll, and it polls several times per game frame, so
a poll index is not a retrace: space presses generously and check the dump.

The movie starts at power-on (no save state), uses port 1 and memory card
slot A, and sets the header's tick count far ahead: Dolphin ends playback as
soon as the emulated tick count passes it, so a zero there ends the movie at
once. The header layout is Source/Core/Core/Movie.h in ref/recompcore.

Used by the Dolphin reference captures in docs/archive/status/CURRENT.md (2026-09-24):
run Dolphin with its own user folder (-u), a GCI-folder card holding the save
(scripts/card_to_gci.py), [Movie] DumpFrames = True and
[Settings] DumpFramesAsImages = True, then -e DISC -m OUT.dtm.
"""
import struct
import sys

# PAD bit -> (byte, bit) in Dolphin's 8-byte ControllerState.
BITS = {
    0x1000: (0, 0), 0x0100: (0, 1), 0x0200: (0, 2), 0x0400: (0, 3),
    0x0800: (0, 4), 0x0010: (0, 5), 0x0008: (0, 6), 0x0004: (0, 7),
    0x0001: (1, 0), 0x0002: (1, 1), 0x0040: (1, 2), 0x0020: (1, 3),
}


def apply_game_config(h):
    """Save this game's Dolphin settings in the movie (bSaveConfig).

    Without it, playback ran with EFB access off even though Dolphin enables it
    for Wind Waker (Sys/GameSettings/GZL.ini), so GXPeekZ read 0 and the sun's
    glare never drew in the movie references (CURRENT.md, 2026-09-24). Header
    offsets are Source/Core/Core/Movie.h's DTMHeader.
    """
    h[81:97] = b'Metal'.ljust(16, b'\0')   # videoBackend
    h[97:113] = b'HLE'.ljust(16, b'\0')    # audioEmulator (informational)
    h[137] = 1   # bSaveConfig
    h[138] = 1   # bSkipIdle
    h[139] = 1   # bDualCore
    h[140] = 0   # bProgressive
    h[141] = 1   # bDSPHLE
    h[142] = 0   # bFastDiscSpeed
    h[143] = 4   # CPUCore: JIT ARM64
    h[144] = 1   # bEFBAccessEnable (GZL.ini)
    h[145] = 1   # bEFBCopyEnable
    h[146] = 0   # bSkipEFBCopyToRam (GZL.ini EFBToTextureEnable = False)
    h[147] = 0   # bEFBCopyCacheEnable
    h[148] = 0   # bEFBEmulateFormatChanges
    h[149] = 0   # bImmediateXFB
    h[150] = 1   # bSkipXFBCopyToRam
    h[151] = 1   # memcards: slot A
    h[152] = 0   # bClearSave: use the user folder's card
    h[159] = 1   # bFollowBranch
    h[160] = 1   # bUseFMA


def main():
    if len(sys.argv) != 4:
        raise SystemExit(__doc__)
    out, polls = sys.argv[1], int(sys.argv[2], 0)
    entries = []
    for item in sys.argv[3].split(','):
        f = item.split(':')
        if len(f) not in (3, 5):
            raise SystemExit('bad entry %r' % item)
        start, buttons, length = int(f[0], 0), int(f[1], 0), int(f[2], 0)
        stick = (int(f[3], 0), int(f[4], 0)) if len(f) == 5 else None
        entries.append((start, buttons, length, stick))
    states = []
    for i in range(polls):
        b = [0, 1 << 6]  # is_connected
        sx = sy = 128
        for start, buttons, length, stick in entries:
            if start <= i < start + length:
                for bit, (byte, pos) in BITS.items():
                    if buttons & bit:
                        b[byte] |= 1 << pos
                if stick is not None:
                    sx, sy = 128 + stick[0], 128 + stick[1]
                break
        trig_l = 255 if b[1] & (1 << 2) else 0
        trig_r = 255 if b[1] & (1 << 3) else 0
        states.append(bytes([b[0], b[1], trig_l, trig_r, sx, sy, 128, 128]))
    h = bytearray(256)
    h[0:4] = b'DTM\x1a'
    h[4:10] = b'GZLE01'
    h[11] = 0x01                                    # GC controller, port 1
    struct.pack_into('<Q', h, 13, polls)            # frameCount (informational)
    struct.pack_into('<Q', h, 21, polls)            # inputCount
    h[151] = 0x01                                   # memory card in slot A
    apply_game_config(h)
    struct.pack_into('<Q', h, 237, 486000000 * 600)  # tickCount: 600 s
    open(out, 'wb').write(bytes(h) + b''.join(states))
    print('wrote %s: %d polls, %d entries' % (out, polls, len(entries)))


if __name__ == '__main__':
    main()
