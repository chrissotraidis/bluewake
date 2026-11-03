#!/usr/bin/env python3
"""Run one local build command with a log and periodic, truthful progress."""
import argparse
import json
import os
import re
import signal
from pathlib import Path
import subprocess
import sys
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--log", type=Path, required=True)
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    command = args.command
    if command and command[0] == "--":
        command = command[1:]
    if not command:
        parser.error("a command is required")
    args.log.parent.mkdir(parents=True, exist_ok=True)
    events = args.log.parent / "progress.jsonl"
    started = time.monotonic()
    stage = args.log.stem
    cancel_signal = signal.SIGINT

    def cancel(signum, _frame):
        nonlocal cancel_signal
        cancel_signal = signum
        raise KeyboardInterrupt

    signal.signal(signal.SIGTERM, cancel)

    def report(state, **fields):
        names = {"running": "stage_progress", "complete": "stage_completed",
                 "failed": "stage_failed", "interrupted": "build_cancelled",
                 "started": "stage_started"}
        event = dict(schema_version=1, event=names[state], stage=stage, state=state,
                     elapsed_seconds=round(time.monotonic() - started), **fields)
        with events.open("a") as stream:
            stream.write(json.dumps(event) + "\n")

    report("started")
    print(f"  {stage}: starting (log: {args.log})", flush=True)
    env = dict(os.environ, NINJA_STATUS="[%f/%t] ")
    with args.log.open("wb") as log:
        try:
            process = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT,
                                       env=env, start_new_session=True)
        except OSError as error:
            report("failed", error=str(error))
            print(str(error), file=sys.stderr)
            return 127
        try:
            while True:
                try:
                    code = process.wait(timeout=15)
                    break
                except subprocess.TimeoutExpired:
                    with args.log.open("rb") as reader:
                        reader.seek(max(0, args.log.stat().st_size - 4096))
                        lines = reader.read().decode(errors="replace").splitlines()
                    latest = lines[-1][-240:] if lines else "working; no new output yet"
                    elapsed = int(time.monotonic() - started)
                    print(f"  {stage} · {elapsed // 60}m {elapsed % 60:02}s · {latest}", flush=True)
                    counts = re.search(r"\[(\d+)/(\d+)\]", latest)
                    progress = dict(completed=int(counts[1]), total=int(counts[2]),
                                    unit="build steps") if counts else {}
                    report("running", detail=latest, **progress)
        except KeyboardInterrupt:
            try:
                os.killpg(process.pid, signal.SIGTERM)
            except ProcessLookupError:
                pass
            # The training helper needs up to 10 seconds to reap its own group.
            deadline = time.monotonic() + 15
            while True:
                process.poll()  # reap the direct child; descendants may remain
                try:
                    os.killpg(process.pid, 0)
                except ProcessLookupError:
                    break
                if time.monotonic() >= deadline:
                    try:
                        os.killpg(process.pid, signal.SIGKILL)
                    except ProcessLookupError:
                        pass
                    break
                time.sleep(0.1)
            process.wait()
            report("interrupted")
            return 128 + cancel_signal
    report("complete" if code == 0 else "failed", exit_code=code)
    elapsed = int(time.monotonic() - started)
    print(f"  {stage}: {'complete' if code == 0 else 'failed'} in {elapsed // 60}m {elapsed % 60:02}s", flush=True)
    if code:
        with args.log.open("rb") as reader:
            reader.seek(max(0, args.log.stat().st_size - 16384))
            print("\n".join(reader.read().decode(errors="replace").splitlines()[-40:]), file=sys.stderr)
    return code if code >= 0 else 128 - code


if __name__ == "__main__":
    sys.exit(main())
