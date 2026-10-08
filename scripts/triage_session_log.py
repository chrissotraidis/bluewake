#!/usr/bin/env python3
"""Summarize a BlueWake session log: setup, crashes, slowdowns and hitches.

Usage: python3 scripts/triage_session_log.py session-*.log ...

Reads the lines the runtime already writes ([fps-dip], [interp-pace], [perf],
[gx-slow], [render-slow], [music-stream], [demo], [audio-lost], [crash] ...) and prints a short
report, so an attached log shows where and why the game slowed down or lost its sound without
reading it line by line.
"""
import re
import sys
from collections import Counter
from statistics import median

# Session logs stamp each line; a raw console capture does not.
LINE = re.compile(r"^(?:(\d\d):(\d\d):(\d\d)\.\d+ )?\[([^\]]+)\] ?(.*)$")
SETUP = ("[windows]", "[simulation]", "[aspect]", "CPU model", "OS:", "Device:", "Using framebuffer",
         "present mode", "Device lock", "[host] module", "[device]", "[smooth-motion]")
FATAL = re.compile(
    r"\[crash\]|\[panic\]|Device lost|exception 0x|fatal|"
    r"\bdouble free or corruption|free\(\): double free detected|"
    r"\b(?:malloc|free|realloc)\(\): (?:invalid|corrupted)", re.IGNORECASE)
# Not failures: a capped REL call trace once labelled [panic], the device released at exit, and
# the Windows startup line naming where crash reports would go ([crash] reports=...).
BENIGN = re.compile(r"\[panic\] vcall-after|Device lost: Device was destroyed|\[crash\] reports=")
PACE = re.compile(r"in-between frames (\d+) -> (\d+)")
CAUSES = {"gx-worker": "GX worker (GPU command conversion on the CPU)", "game-thread": "game thread",
          "shader-compile": "shader compile", "gpu-present": "GPU or presentation",
          "render-worker": "render worker", "interp-helper": "Smooth Motion helper",
          "smooth-motion-paused": "Smooth Motion paused after a slowdown (counter still shows 60)",
          "unclear": "unclear"}


def field(text, name, cast=float):
    m = re.search(r"\b" + re.escape(name) + r"=(-?[\d.]+)", text)
    return cast(m.group(1)) if m else None


def bottleneck(text):
    """Which part was saturated in an [fps-dip] second (logged as cause= since October 3)."""
    logged = re.search(r"\bcause=(\S+)", text)
    if logged:
        return CAUSES.get(logged[1], logged[1])
    gx_worker = re.search(r"(?:threads|workers): gx=(\d+)%", text)
    gx_worker = int(gx_worker.group(1)) if gx_worker else 0
    game = field(text, "busy") or 0
    present = re.search(r"present=(\d+)ms", text)
    gpu = re.search(r"gpu=(\d+)ms", text)
    waits = (int(present.group(1)) if present else 0) + (int(gpu.group(1)) if gpu else 0)
    if gx_worker >= 85:
        return "GX worker (GPU command conversion on the CPU)"
    if game >= 85:
        return "game thread"
    if waits >= 300:
        return "GPU or presentation"
    return "unclear"


def music_playback_spans(changes):
    """Observed state-4 spans, not proof of audible music or why a track stopped.

    Works on existing releases as well as new diagnostic lines. Retraces are
    guest time; don't turn them into wall-clock milliseconds. A restart/load
    can rewind the counter, and an unfinished log cannot prove a stop.
    """
    spans, active, previous_retrace = [], None, None
    for text in changes:
        retrace = field(text, "retrace", int)
        state = field(text, "state", int)
        track = re.search(r'path="([^"]*)".*?\bid=(0x[0-9a-fA-F]+)', text)
        if retrace is None or state is None or track is None:
            continue
        if previous_retrace is not None and retrace < previous_retrace:
            active = None  # save-state restore/restart invalidates the span
        previous_retrace = retrace
        key = (track[1], track[2].lower())
        if active is not None:
            start, previous_key = active
            if state != 4 or key != previous_key:
                spans.append((previous_key[0], retrace - start,
                              "state=" + str(state) if state != 4 else "track changed"))
                active = None
        if state == 4 and active is None:
            active = (retrace, key)
    return spans


def report(path):
    setup, fatal, dips, paces = [], [], [], []
    reasons, places, causes = Counter(), Counter(), Counter()
    gx_slow = gx_compile = render_slow = 0
    worst_ms, hitches, first, last, previous, day = 0.0, 0, None, None, None, 0
    thermal, summary = 0, None
    music = []
    demos, lost, missing = [], [], []
    with open(path, errors="replace") as log:
        for raw in log:
            raw = raw.rstrip("\r\n")
            # Allocator aborts and raw console failures may have no [tag]. A
            # perf-summary before shutdown does not make that exit successful.
            if FATAL.search(raw) and not BENIGN.search(raw):
                fatal.append(raw[:200])
            m = LINE.match(raw)
            if not m:
                continue
            if m[1] is not None:
                seconds = int(m[1]) * 3600 + int(m[2]) * 60 + int(m[3])
                if previous is not None and seconds < previous - 3600:
                    day += 86400
                previous = seconds
                seconds += day
                first = seconds if first is None else first
                last = seconds
            tag, text = m[4], m[5]
            if any(s in raw for s in SETUP) and "settings menu" not in raw and len(setup) < 14:
                setup.append(raw[raw.index("["):].strip()[:160])
            if tag == "fps-dip":
                reason = text.split("reason=", 1)[-1] if "reason=" in text else "?"
                stage = re.search(r"stage=(\S+) room=(-?\d+)", text)
                place = f"{stage[1]} room {stage[2]}" if stage else "?"
                reasons[reason] += 1
                places[place] += 1
                causes[bottleneck(text)] += 1
                dips.append(field(text, "speed") or 0)
            elif tag == "interp-pace" and PACE.search(text):
                paces.append(text)
            elif tag == "music-stream":
                music.append(text)
            elif tag == "demo" and text.startswith("end "):
                demos.append(text[4:])
            elif tag == "audio-lost":
                lost.append(text)
            elif tag == "demo-sound" and text.endswith("no sound"):
                missing.append(text)
            elif tag == "perf-summary":
                summary = text
            elif tag == "fps" and "thermal=" in text:
                thermal = max(thermal, int(field(text, "thermal", int) or 0))
            elif tag == "perf":
                worst_ms = max(worst_ms, field(text, "worst_ms") or 0)
                hitches += int(field(text, "hitches") or 0)
            elif tag == "gx-slow":
                gx_slow += 1
                if (field(text, "pipelines", int) or 0) or (field(text, "textures", int) or 0):
                    gx_compile += 1
            elif tag in ("render-slow", "present-slow"):
                render_slow += 1

    minutes = (last - first) / 60 if first is not None else 0
    print(f"== {path}  ({minutes:.0f} min)")
    for line in setup:
        print("  setup:", line)
    print(f"  fatal lines: {len(fatal)}")
    for line in fatal[:5]:
        print("   ", line)
    print(f"  seconds below target FPS ([fps-dip]): {len(dips)}"
          + (f", median game speed {median(dips):.0f}%, lowest {min(dips):.0f}%" if dips else ""))
    for reason, n in reasons.most_common():
        print(f"    {n:5d}  {reason}")
    for cause, n in causes.most_common():
        print(f"    {n:5d}  saturated: {cause}")
    for place, n in places.most_common(6):
        print(f"    {n:5d}  at {place}")
    drops = [p for p in paces if int(PACE.search(p)[2]) < int(PACE.search(p)[1])]
    print(f"  Smooth Motion lowered: {len(drops)} times, raised {len(paces) - len(drops)} times")
    for text in Counter(re.sub(r"[\d.]+ game frames", "N game frames", p) for p in drops).most_common(3):
        print(f"    {text[1]:5d}  {text[0][:150]}")
    print(f"  hitches in [perf]: {hitches}, worst frame {worst_ms:.0f} ms")
    print(f"  slow GX batches: {gx_slow} ({gx_compile} while compiling pipelines or loading textures)")
    print(f"  slow render/present frames: {render_slow}")
    print(f"  streamed music changes ([music-stream]): {len(music)}")
    for text in music[:12]:
        print("   ", text[:350])
    spans = music_playback_spans(music)
    short = [span for span in spans if span[1] < 60]
    if music:
        print(f"  streamed playback ended within 60 retraces: {len(short)}")
    else:
        print("  streamed playback: unknown (no [music-stream] diagnostics in this log)")
    for track, duration, end in short[:12]:
        print(f"    {track}: playing for {duration} retraces, then {end}")
    if short:
        print("    Check for an intentional skip/stop or premature finish; this is not a diagnosed failure.")
    if music or demos:
        print("  Audio caveat: a sound handle or mixed-output activity does not prove the expected music was audible.")
    quiet = [d for d in demos if " missing=0 " not in d or float(field(d, "silent") or 0) >= 2]
    print(f"  cutscenes ([demo]): {len(demos)}, with a missing sound or 2 s of silence: {len(quiet)}")
    for text in quiet[:12]:
        print("   ", text[:150])
    print(f"  sound lost ([audio-lost]): {len(lost)}, cues with no sound ([demo-sound]): {len(missing)}")
    for text in (lost + missing)[:12]:
        print("   ", text[:150])
    if thermal:
        print(f"  highest iOS thermal state: {thermal} (0 nominal .. 3 critical)")
    if summary:
        print(f"  last [perf-summary]: {summary[:300]}")


if __name__ == "__main__":
    names = sys.argv[1:]
    if not names or any(name in ("-h", "--help") for name in names):
        sys.exit(__doc__)
    for name in names:
        try:
            report(name)
        except OSError as error:
            print(f"== {name}: cannot read ({error.strerror})")
