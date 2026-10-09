#!/usr/bin/env python3
"""Measure a BlueWake Android build's speed at one spot, like for like.

    python scripts/android/measure.py --card-on-device PATH [--apk APK] [--cool] [--headless] [--label NAME]

Loads a save from a copy of a memory card, lets the game run to retrace --from (3000 by
default: the save loaded and settled), then counts the game thread's cycles and
instructions for --seconds with simpleperf and reads the session log's [perf] lines over the
same window. The result is the game's speed (retraces a second over 59.94), how busy the
game thread was, and its instructions and cycles per retrace. Instructions per retrace is
the number to compare between builds: it does not change with the clock, so it holds while
the phone heats and throttles, which a phone does within a minute.

The card is copied, never used in place: the copy goes to the app's folder as
measure.card (and measure-sram.bin), and the player's own card is only read. launch.env is
written for the run and put back as it was afterwards.

--cool sleeps the phone first and waits (up to 40 minutes) until its skin temperature is
under 34 C and every CPU cluster may run at its full clock again, so runs start alike.
--headless measures without drawing or pacing (BLUEWAKE_RENDERER=headless,
BLUEWAKE_WALL_PACE=0): how fast the build can run the game at all.

The default --pad-script presses A three times after the title, which loads Quest Log 1 from
the file select; a card whose first quest log is somewhere else loads there instead.
"""
import argparse
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time

sys.path.insert(0, str(Path(__file__).resolve().parent))
import install  # noqa: E402  (find_adb, pick_serial)

ROOT = Path(__file__).resolve().parents[2]
ACTIVITY = "dev.bluewake.android.BlueWakeActivity"
RETRACE_HZ = 59.94
PAD_SCRIPT = "540:0x0100:2,640:0x0100:2,740:0x0100:2"


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--serial", help="adb serial of the device (default: the one phone connected)")
    parser.add_argument("--adb", type=Path, help="adb executable")
    parser.add_argument("--package", default=install.DEFAULT_PACKAGE, help="the app to measure")
    parser.add_argument("--apk", type=Path, help="install this APK first (adb install -r: the app's data is kept)")
    card = parser.add_mutually_exclusive_group(required=True)
    card.add_argument("--card", type=Path, help="a memory card file on this PC to load from (pushed as a copy)")
    card.add_argument("--card-on-device", metavar="PATH",
                      help="a memory card on the device to load from, such as another app's files/GZLE01.card "
                           "(copied on the device)")
    parser.add_argument("--sram", help="the console settings (sram.bin) beside the card, on the device or this PC "
                                       "(default: sram.bin beside --card-on-device, if there is one)")
    parser.add_argument("--from", dest="start", type=int, default=3000, help="start counting at this retrace")
    parser.add_argument("--seconds", type=int, default=60, help="how long to count")
    parser.add_argument("--cool", action="store_true", help="wait for a cool phone at full clocks first")
    parser.add_argument("--headless", action="store_true", help="no drawing and no pacing")
    parser.add_argument("--pad-script", default=PAD_SCRIPT, help="BLUEWAKE_PAD_SCRIPT that loads the save")
    parser.add_argument("--env", action="append", default=[], metavar="NAME=VALUE",
                        help="more launch settings for the run, such as DOL_AURORA_RENDER_SCALE=3 (repeat)")
    parser.add_argument("--label", default="build", help="a name for this build in the results")
    parser.add_argument("--out", type=Path, default=ROOT / "build/android/measure",
                        help="where the session log and the results go")
    args = parser.parse_args()

    adb = install.find_adb(args.adb)
    serial = install.pick_serial(adb, args.serial)
    files = f"/sdcard/Android/data/{args.package}/files"
    mode = "headless" if args.headless else "rendered"
    name = f"{args.label}-{mode}"
    args.out.mkdir(parents=True, exist_ok=True)

    def adb_run(*command, check=True):
        return subprocess.run([adb, "-s", serial, *[str(c) for c in command]], check=check,
                              capture_output=True, text=True, encoding="utf-8", errors="replace").stdout

    def shell(command, check=True):
        return adb_run("shell", command, check=check)

    def skin_temperature():
        """The hottest SKIN reading Android's thermal service reports, in C."""
        temps = [float(m) for m in re.findall(r"mValue=([\d.]+), mType=3,", shell("dumpsys thermalservice", False))]
        return max(temps) if temps else None

    def clocks_at_full():
        """Every CPU cluster may run at its own top clock (no thermal cap)."""
        out = shell("for p in /sys/devices/system/cpu/cpufreq/policy*; do "
                    "echo $(cat $p/scaling_max_freq) $(cat $p/cpuinfo_max_freq); done", False)
        pairs = [line.split() for line in out.splitlines() if len(line.split()) == 2]
        return bool(pairs) and all(a == b for a, b in pairs)

    def caps():
        return shell("cat /sys/devices/system/cpu/cpufreq/policy*/scaling_max_freq", False).split()

    if not shell(f"ls {files}/game/main.dol", False).strip().endswith("main.dol"):
        sys.exit(f"{args.package} has no game files on the device: run scripts/android/install.py first")

    shell(f"am force-stop {args.package}")
    if args.cool:
        shell("input keyevent KEYCODE_SLEEP")
        print("waiting for a cool phone (skin under 34 C, full clocks)", flush=True)
        deadline = time.time() + 40 * 60
        while time.time() < deadline:
            skin = skin_temperature()
            if skin is not None and skin < 34.0 and clocks_at_full():
                break
            time.sleep(10)
        else:
            print("  still warm after 40 minutes: measuring anyway (see the start temperature below)")
    start_skin, start_caps = skin_temperature(), caps()
    print(f"start: skin {start_skin} C, clock caps {' '.join(start_caps)}", flush=True)

    if args.apk:
        print(f"installing {args.apk.name}", flush=True)
        adb_run("install", "-r", "-g", args.apk)

    # The card and the console settings, as copies the run may write to.
    if args.card:
        adb_run("push", args.card, f"{files}/measure.card")
    else:
        shell(f"cat '{args.card_on_device}' > {files}/measure.card")
    sram = args.sram
    if sram is None and args.card_on_device:
        beside = str(Path(args.card_on_device).parent.as_posix()) + "/sram.bin"
        sram = beside if shell(f"ls '{beside}'", False).strip() == beside else None
    if sram and Path(sram).exists():
        adb_run("push", sram, f"{files}/measure-sram.bin")
    elif sram:
        shell(f"cat '{sram}' > {files}/measure-sram.bin")

    env = [f"BLUEWAKE_CARD_PATH={files}/measure.card", "BLUEWAKE_PERF_LOG=1", "BLUEWAKE_PLAYER_PROBE=1",
           "BLUEWAKE_PAD_BUTTONS=0x0100", "BLUEWAKE_PAD_PULSE_ON_TITLE_READY=1", "BLUEWAKE_PAD_PULSE_LENGTH=2",
           "BLUEWAKE_PAD_CONFIRM_EVENT=any", f"BLUEWAKE_PAD_SCRIPT={args.pad_script}",
           "BLUEWAKE_MAX_RETRACES=200000"]
    if sram:
        env.append(f"BLUEWAKE_SRAM={files}/measure-sram.bin")
    if args.headless:
        env += ["BLUEWAKE_RENDERER=headless", "BLUEWAKE_WALL_PACE=0"]
    env += args.env
    previous_env = shell(f"cat {files}/launch.env 2>/dev/null", False)
    local_env = args.out / f"{name}.env"
    local_env.write_text("".join(f"{e}\n" for e in env), encoding="utf-8", newline="\n")

    before = set(shell(f"ls {files}/logs", False).split())
    try:
        adb_run("push", local_env, f"{files}/launch.env")
        shell("input keyevent KEYCODE_WAKEUP")
        shell(f"am start -n {args.package}/{ACTIVITY} --ez showWhenLocked true")
        log = None
        for _ in range(30):
            new = sorted(set(shell(f"ls {files}/logs", False).split()) - before)
            if new:
                log = new[-1]
                break
            time.sleep(1)
        if log is None:
            sys.exit("the game did not start a session log")

        def retrace():
            found = re.findall(r"retrace=(\d+)", shell(f"tail -c 20000 {files}/logs/{log}", False))
            return int(found[-1]) if found else 0

        last, same = -1, 0
        while (now := retrace()) < args.start:
            if not shell(f"pidof {args.package}", False).strip():
                sys.exit(f"the game stopped before retrace {args.start} (log {log})")
            same = same + 1 if now == last else 0
            if same >= 30:
                sys.exit(f"the game stalled at retrace {now} (log {log})")
            last = now
            time.sleep(2)

        r0, t0 = retrace(), time.time()
        print(f"counting {args.seconds} s from retrace {r0}", flush=True)
        stat = shell(f"simpleperf stat --app {args.package} --per-thread -e cpu-cycles,instructions "
                     f"--duration {args.seconds} 2>/dev/null", False)
        r1, t1 = retrace(), time.time()
        end_skin, end_caps = skin_temperature(), caps()
    finally:
        shell(f"am force-stop {args.package}", False)
        if previous_env.strip():
            restore = args.out / "launch.env.previous"
            restore.write_text(previous_env, encoding="utf-8", newline="\n")
            adb_run("push", restore, f"{files}/launch.env", check=False)
        else:
            shell(f"rm -f {files}/launch.env", False)

    session = args.out / f"{name}-session.log"
    session.write_text(shell(f"cat {files}/logs/{log}", False), encoding="utf-8")
    text = session.read_text(encoding="utf-8")

    # The game thread is the busiest SDLThread (all the app's native threads carry that name).
    counts = {}
    for line in stat.splitlines():
        m = re.match(r"\s*(\S+)\s+\d+\s+(\d+)\s+([\d,]+)\s+(cpu-cycles|instructions)\b", line)
        if m and m.group(1) == "SDLThread":
            counts.setdefault(m.group(2), {})[m.group(4)] = int(m.group(3).replace(",", ""))
    retraces = r1 - r0
    game = max(counts.values(), key=lambda c: c.get("cpu-cycles", 0)) if counts else {}

    perf = [(float(m.group(2)), float(m.group(3))) for m in
            re.finditer(r"\[perf\] retrace=(\d+) rate=([\d.]+) .*?busy=([\d.]+)%", text)
            if r0 <= int(m.group(1)) <= r1]
    rate = sum(p[0] for p in perf) / len(perf) if perf else retraces / (t1 - t0)
    busy = sum(p[1] for p in perf) / len(perf) if perf else None
    chassis = re.findall(r"\[chassis\] (\S+)", text)
    spot = re.findall(r"\[player-scene-state\].*?(pos=\S+)", text)
    stage = re.findall(r"stage=(\S+) room=(\d+)", text)

    result = {
        "label": args.label, "mode": mode, "log": log,
        "retraces": retraces, "seconds": round(t1 - t0, 1),
        "speed_percent": round(rate / RETRACE_HZ * 100, 1),
        "game_thread_busy_percent": round(busy, 1) if busy is not None else None,
        "game_thread_instructions_per_retrace_M": round(game.get("instructions", 0) / retraces / 1e6, 1)
        if retraces and game else None,
        "game_thread_cycles_per_retrace_M": round(game.get("cpu-cycles", 0) / retraces / 1e6, 1)
        if retraces and game else None,
        "chassis": chassis,
        "spot": {"stage": stage[-1][0] if stage else None, "room": stage[-1][1] if stage else None,
                 "pos": spot[-1] if spot else None},
        "start": {"skin_c": start_skin, "clock_caps": start_caps},
        "end": {"skin_c": end_skin, "clock_caps": end_caps},
    }
    (args.out / f"{name}.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2))
    if not game:
        print("simpleperf counted nothing: is the APK profileable (every build.py APK is)?")


if __name__ == "__main__":
    main()
