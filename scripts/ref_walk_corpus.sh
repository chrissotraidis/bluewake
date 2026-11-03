#!/usr/bin/env bash
# The 3D walk reference: the same inputs through Dolphin and through BlueWake's
# macOS host, one after the other (never both at once), then paired frames.
#
# Inputs: A presses from power-on reach Quest Log 1 on the Outset pier (the save
# in local-research/ipad/acceptance-20260923-095148/save.card, which Dolphin's
# user folder local-research/dolphin-ref/user holds as a GCI), then the main
# stick held forward. Link walks off the pier, swims, crosses the beach and the
# grass, swims past the cliffs until his air runs out, respawns on the dock and
# swims out again: water, foam, shadows, grass, the air meter, a fade and a
# respawn. Both sides also record audio (Dolphin's [DSP] DumpAudio, BlueWake's
# capture with the emulated SRAM, so both are stereo). Dolphin reads the presses in pad polls (several a frame), BlueWake in
# retraces, so the two schedules differ; frames are paired by picture.
#
# usage: scripts/ref_walk_corpus.sh [--dolphin-only|--bluewake-only|--pairs-only]
# Output: local-research/dolphin-ref/walk-pairs-*.png, walk-*-sheet.png
set -euo pipefail
cd "$(dirname "$0")/.."
r=$PWD
ref=$r/local-research/dolphin-ref
user=$ref/user
disc="$r/ref/The Legend Of Zelda The Wind Waker.iso"
mode=${1:-all}

if [ "$mode" = all ] || [ "$mode" = --dolphin-only ]; then
    scripts/one_game_guard.sh
    entries=$(python3 -c "print(','.join(['%d:0x0100:4' % p for p in range(340, 2401, 60)] + ['3000:0:17000:0:100']))")
    python3 scripts/make_dtm.py "$ref/walk.dtm" 20000 "$entries"
    mkdir -p /tmp/bw_old
    [ -d "$user/Dump/Frames" ] && mv "$user/Dump/Frames" "/tmp/bw_old/frames-$(date +%s)"
    mkdir -p "$user/Dump/Frames"
    [ -d "$user/Dump/Audio" ] && mv "$user/Dump/Audio" "/tmp/bw_old/audio-$(date +%s)"
    mkdir -p "$user/Dump/Audio"
    /Applications/Dolphin.app/Contents/MacOS/Dolphin -u "$user" -e "$disc" -m "$ref/walk.dtm" \
        > "$ref/walk-run.log" 2>&1 &
    pid=$!
    sleep "${BW_DOLPHIN_SECONDS:-170}"
    kill "$pid" 2>/dev/null || true; sleep 2; kill -9 "$pid" 2>/dev/null || true
    echo "dolphin: $(ls "$user/Dump/Frames" | wc -l | tr -d ' ') frames"
fi

if [ "$mode" = all ] || [ "$mode" = --bluewake-only ]; then
    scripts/one_game_guard.sh
    out=$ref/walk-bluewake
    mkdir -p /tmp/bw_old
    [ -d "$out" ] && mv "$out" "/tmp/bw_old/walk-bluewake-$(date +%s)"
    mkdir -p "$out"
    cp "$r/local-research/ipad/acceptance-20260923-095148/save.card" "$out/c.card"
    script=$(python3 -c "print(','.join(['%d:0x0100:2' % p for p in range(340, 1601, 60)] + ['1700:0:7300:0:100']))")
    env BLUEWAKE_SRAM=default BLUEWAKE_CAPTURE_AUDIO_WAV="$out/audio.wav" BLUEWAKE_RENDERER=aurora BLUEWAKE_DSP_MODE=hle BLUEWAKE_CYCLE_CAP=16384 \
        BLUEWAKE_DOL="$r/generated/full/main.dol" BLUEWAKE_RELS_DIR="$r/generated/full/rels" \
        BLUEWAKE_DISC="$disc" BLUEWAKE_CARD_PATH="$out/c.card" \
        BLUEWAKE_PAD_SCRIPT="$script" BLUEWAKE_PAD_CONFIRM_EVENT=any BLUEWAKE_MAX_RETRACES=9000 \
        BLUEWAKE_CAPTURE_OPENING_FRAME="$out/f.ppm" BLUEWAKE_CAPTURE_RETRACE=1600 BLUEWAKE_CAPTURE_INTERVAL=60 \
        "$r/build/runtime-host-dsp/BlueWake.app/Contents/MacOS/BlueWake" \
        "${BW_COMPOSITE:-$r/build/composite-cycle-hybrid-o2-v2/gGZLE01_recomp.dylib}" > "$out/run.log" 2>&1
    echo "bluewake: $(ls "$out"/f-*.ppm | wc -l | tr -d ' ') frames"
fi

python3 - "$ref" <<'EOF'
import glob, os, re, sys
from PIL import Image, ImageChops, ImageStat
ref = sys.argv[1]
frames = ref + '/user/Dump/Frames'
num = lambda p: int(re.findall(r'(\d+)', os.path.basename(p))[-1])
bw = sorted(glob.glob(ref + '/walk-bluewake/f-*.ppm'), key=num)
small = {p: Image.open(p).convert('RGB').resize((96, 72)) for p in bw}
moments = (1080, 1260, 1350, 1440, 1980, 2250, 2610, 2790)
pairs = []
for n in moments:
    path = '%s/framedump_%d.png' % (frames, n)
    if not os.path.exists(path):
        continue
    d = Image.open(path).convert('RGB').resize((96, 72))
    best = min(bw, key=lambda p: sum(ImageStat.Stat(ImageChops.difference(d, small[p])).mean))
    pairs.append((path, best))
    print('dolphin %d ~ bluewake retrace %d' % (n, num(best)))
pw, ph = 640, 480
for k in range(0, len(pairs), 4):
    sheet = Image.new('RGB', (pw * 2, ph * 4))
    for i, (a, b) in enumerate(pairs[k:k + 4]):
        sheet.paste(Image.open(a).convert('RGB').resize((pw, ph)), (0, i * ph))
        sheet.paste(Image.open(b).convert('RGB').resize((pw, ph)), (pw, i * ph))
    sheet.save('%s/walk-pairs-%d.png' % (ref, k // 4))
print('pairs: %s/walk-pairs-*.png (Dolphin left, BlueWake right)' % ref)
EOF
