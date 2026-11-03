#!/usr/bin/env python3
"""Warp to every place in the stage select, headless, one run each, and say how each run ended.

  python3 scripts/stage_select_sweep.py APP CARD OUT_DIR [--jobs N] [--only 1.0,32.28,...]

APP is a BlueWake executable (on a Mac, BlueWake.app/Contents/MacOS/BlueWake). CARD is a memory card
whose quest log 1 the runs start (BLUEWAKE_WARP_FILE=1); each run gets its own copy, and the sweep checks
that the copy is unchanged afterwards (the stage select turns saving off, #266). Every place in
config/stage_select/names.tsv is tried with BLUEWAKE_WARP, stopping 120 play frames after Link arrives
(BLUEWAKE_WARP_STOP_AFTER) or after 6,000 retraces. OUT_DIR gets each run's log and results.tsv.

How a run ends: "warp-arrived" is a place that loads and runs; "normal" is one that reached the
retrace limit without arriving (a hang, or a cutscene waiting for story progress the file lacks);
anything else is a crash, with its pc. The notes column is the names file's: (hangs), (crash),
(before)/(after). Late-game cutscenes need a late-game file. Nothing here is timed; it is a crash check.
"""
import argparse
import concurrent.futures
import os
import re
import shutil
import subprocess
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
NAMES = ROOT / "config/stage_select/names.tsv"
COLUMNS = ("place", "name", "stage", "note", "ended", "pc", "exit", "arrived", "card_unchanged", "seconds")


def places():
    out = []
    for line in NAMES.read_text(encoding="utf-8").splitlines():
        fields = line.split("\t")
        if line.startswith("#") or len(fields) < 7 or fields[1] == "-":
            continue
        out.append((f"{fields[0]}.{fields[1]}", fields[2], fields[3], fields[8] if len(fields) > 8 else ""))
    return out


def run(app, card, out, home, place, extra):
    key, name, stage, note = place
    folder = out / key
    folder.mkdir(parents=True, exist_ok=True)
    copy = folder / "run.card"
    shutil.copy(card, copy)
    env = {k: v for k, v in os.environ.items() if not k.startswith(("BLUEWAKE_", "DOL_"))}
    env.update(HOME=str(home), BLUEWAKE_CARD_PATH=str(copy), BLUEWAKE_SRAM=str(folder / "sram.bin"),
               BLUEWAKE_SETTINGS=str(home / "settings.ini"), BLUEWAKE_RENDERER="headless",
               BLUEWAKE_WALL_PACE="0", DOL_AURORA_FRAME_INTERP="0", BLUEWAKE_WARP=key,
               BLUEWAKE_STAGE_SELECT_NAMES=str(NAMES), BLUEWAKE_WARP_FILE="1",
               BLUEWAKE_WARP_STOP_AFTER="120", BLUEWAKE_MAX_RETRACES="6000",
               # Windows keeps settings, logs and its launch marker in the data folder (a portable
               # build's user\ otherwise): one of its own a run, so runs neither share nor change it.
               BLUEWAKE_DATA_DIR=str(folder))
    env.update(extra)
    start = time.time()
    with open(folder / "log.txt", "w") as log:
        try:
            code = subprocess.run([app], env=env, stdout=log, stderr=subprocess.STDOUT, timeout=600).returncode
        except subprocess.TimeoutExpired:
            code = "timeout"
    text = (folder / "log.txt").read_text(errors="replace")
    stop = re.search(r"\[run\] stopped: (.+?) after \d+ blocks at pc=(\S+)", text)
    unchanged = copy.read_bytes() == Path(card).read_bytes()
    copy.unlink()
    return {"place": key, "name": name, "stage": stage, "note": note,
            "ended": stop.group(1) if stop else "no stop line", "pc": stop.group(2) if stop else "",
            "exit": code, "arrived": "[warp] arrived" in text, "card_unchanged": unchanged,
            "seconds": round(time.time() - start, 1)}


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("app", type=Path)
    parser.add_argument("card", type=Path)
    parser.add_argument("out", type=Path)
    parser.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 2) // 2))
    parser.add_argument("--only", default="", help="comma-separated places, for example 1.0,32.28")
    parser.add_argument("--env", action="append", default=[], metavar="NAME=VALUE",
                        help="set for every run, for example BLUEWAKE_LOOSE_REL_RETRACES=12 (repeatable)")
    args = parser.parse_args()
    for item in args.env:
        if "=" not in item or item.startswith("="):
            parser.error(f"--env takes NAME=VALUE, not {item!r}")
    extra = dict(item.split("=", 1) for item in args.env)
    todo = places()
    if args.only:
        wanted = set(args.only.split(","))
        todo = [p for p in todo if p[0] in wanted]
    args.out.mkdir(parents=True, exist_ok=True)
    home = Path(tempfile.mkdtemp(prefix="sweep-home-", dir=args.out))
    results = []
    with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
        for r in pool.map(lambda p: run(args.app, args.card, args.out, home, p, extra), todo):
            results.append(r)
            print(f"{r['place']}\t{r['ended']}\t{r['pc']}\t{r['seconds']}s\t{r['name']}", flush=True)
    with open(args.out / "results.tsv", "w") as table:
        table.write("\t".join(COLUMNS) + "\n")
        for r in results:
            table.write("\t".join(str(r[c]) for c in COLUMNS) + "\n")
    arrived = sum(r["ended"] == "warp-arrived" for r in results)
    unchanged = sum(r["card_unchanged"] for r in results)
    print(f"arrived {arrived} of {len(results)}; card unchanged after {unchanged} of {len(results)}")
    return 0 if unchanged == len(results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
