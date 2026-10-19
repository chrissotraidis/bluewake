#!/usr/bin/env python3
"""Measure a BlueWake build at fixed stops, and compare two of those runs.

    scripts/bench_tour.py CARD APP [--stop-retraces N] [--untrained]
        [--mode rendered|headless|both] [--state FILE]
    scripts/bench_tour.py compare A.json B.json

The card is copied with scripts/card_set_restart.py into a scratch folder,
and quest log 1 is placed on Outset (sea 44 0). A card continue is not the
measurement: two continues of one build already diverge before the save
loads. One headless setup plays that card and saves a state at retrace
2390. Each measured mode loads the state and warps.

Default stops, from retrace 2400, one every --stop-retraces (default 1800).
The run ends at 11400:

    2400   sea:44:0      Outset
    4200   sea:11:1      Windfall
    6000   M_NewD2:0:0   Dragon Roost Cavern
    7800   sea:41:0      Forest Haven
    9600   sea:1:100     sea by the Fortress

--untrained replaces those five with M_Dai:0:0 and kaze:0:0, which the
builders' training tour does not visit. --state FILE loads that file and
does not warp. --mode rendered is the default (the game plus the graphics
thread). headless sets BLUEWAKE_RENDERER=headless. both runs each mode
from the same state.

For each stop the first 300 retraces are skipped. Game frames a second are
the [perf] rate divided by 2, averaged over the rest of the stop. Busy is
the busy= field, averaged the same way. Dip causes are the cause= values of
[fps-dip] lines in that window. A stop counts as loaded only when a [load]
line names its stage, not when the test warp is requested.

compare prints the speed change and the busy change at each stop. A speed
change outside 3% is noted so a person can judge two runs of one build. It
is not a failure. The command exits non-zero only when checkpoints differ,
and it names the first retrace.

Windows fills BLUEWAKE_RENDERER and BLUEWAKE_WALL_PACE only when they are
unset (windows/src/win_entry.c). This script sets both. Headless has not
been confirmed on a Windows PC. If it does not run there, use --mode rendered.

The scratch card, the state and the logs stay in a temporary directory.
summary.json is written in the current directory. Do not commit it.
"""

from __future__ import annotations

import argparse
import gzip
import json
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import NamedTuple


START = 2400
SETTLE = 300
SAVE_AT = 2390
# Two retraces past the request, the margin the training tour uses once
# gameplay is already running. Control on this route is near retrace 833.
SETUP_CAP = SAVE_AT + 2

PERF = re.compile(
    r"\[perf\] retrace=(?P<retrace>\d+) rate=(?P<rate>\d+(?:\.\d+)?) "
    r"worst_ms=\d+(?:\.\d+)? hitches=\d+ busy=(?P<busy>\d+(?:\.\d+)?)%"
)
DIP = re.compile(
    r"\[fps-dip\] retrace=(?P<retrace>\d+)\b.*\bcause=(?P<cause>\S+) reason="
)
LOAD = re.compile(r"\[load\] retrace=(?P<retrace>\d+)\b.*\bstage=(?P<stage>\S+)")
CHECKPOINT = re.compile(
    r"\[guest-checkpoint\] version=1 retrace=(?P<retrace>\d+) cycle=(?P<cycle>\d+) "
    r"cpu=(?P<cpu>[0-9A-Fa-f]{32}) mem1=(?P<mem1>[0-9A-Fa-f]{32}) "
    r"mem2=(?P<mem2>[0-9A-Fa-f]{32}) aliases=(?P<aliases>[0-9A-Fa-f]{32}) "
    r"alias_spans=(?P<alias_spans>\d+) alias_bytes=(?P<alias_bytes>\d+)"
)
FAILED_CHECKPOINT = re.compile(r"\[guest-checkpoint\] failed retrace=(?P<retrace>\d+)")
STATE_LOADED = re.compile(r"\[state\] loaded \S+ retrace=(?P<retrace>\d+)")


class Stop(NamedTuple):
    stage: str
    room: int
    point: int
    name: str

    @property
    def code(self) -> str:
        return f"{self.stage}:{self.room}:{self.point}"


DEFAULT_STOPS = (
    Stop("sea", 44, 0, "Outset"),
    Stop("sea", 11, 1, "Windfall"),
    Stop("M_NewD2", 0, 0, "Dragon Roost Cavern"),
    Stop("sea", 41, 0, "Forest Haven"),
    Stop("sea", 1, 100, "sea by the Fortress"),
)
UNTRAINED_STOPS = (
    Stop("M_Dai", 0, 0, "Earth Temple"),
    Stop("kaze", 0, 0, "Wind Temple"),
)


def continue_pad_script() -> str:
    """An A press every 60 retraces from 340 to 1600, the continue route."""
    return ",".join(f"{retrace}:0x0100:2" for retrace in range(340, 1601, 60))


def warp_string(stops, stop_retraces: int = 1800, start: int = START) -> str:
    parts = []
    for index, stop in enumerate(stops):
        at = start + index * stop_retraces
        parts.append(f"{at}:{stop.code}")
    return ",".join(parts)


def tour_end(stops, stop_retraces: int = 1800, start: int = START) -> int:
    return start + len(stops) * stop_retraces


def card_restart_argv(source, dest) -> list[str]:
    script = Path(__file__).resolve().parent / "card_set_restart.py"
    return [sys.executable, str(script), str(source), str(dest), "sea", "44", "0"]


def _child_base(base: dict[str, str]) -> dict[str, str]:
    return {key: value for key, value in base.items() if not key.startswith(("BLUEWAKE_", "DOL_"))}


def _isolate(env: dict[str, str], scratch, card) -> None:
    scratch = Path(scratch)
    card = Path(card)
    if sys.platform == "win32":
        env["BLUEWAKE_DATA_DIR"] = str(scratch)
        return
    env["BLUEWAKE_CARD_PATH"] = str(card)
    env["BLUEWAKE_SRAM"] = str(scratch / "sram.bin")
    env["BLUEWAKE_SETTINGS"] = str(scratch / "settings.ini")


def _phase1(env: dict[str, str], max_retraces: int) -> None:
    env.update({
        "BLUEWAKE_MAX_RETRACES": str(max_retraces),
        "BLUEWAKE_WALL_PACE": "0",
        "BLUEWAKE_PERF_LOG": "1",
        "DOL_AURORA_FRAME_INTERP": "0",
        "BLUEWAKE_PAD_BUTTONS": "0x0100",
        "BLUEWAKE_PAD_PULSE_ON_TITLE_READY": "1",
        "BLUEWAKE_PAD_PULSE_LENGTH": "2",
        "BLUEWAKE_PAD_SCRIPT": continue_pad_script(),
        "BLUEWAKE_GUEST_CHECKPOINT_INTERVAL": "300",
    })


def measurement_environment(base, *, mode, load_state, scratch, card, warp,
                            max_retraces, overrides=None) -> dict[str, str]:
    """The child environment for one measured mode.

    `warp` is the BLUEWAKE_TEST_WARP string. None leaves that variable unset,
    which is what --state does. Rendered does not set BLUEWAKE_RENDERER.
    """
    env = _child_base(base)
    _phase1(env, max_retraces)
    env["BLUEWAKE_LOAD_STATE"] = str(load_state)
    if warp is not None:
        env["BLUEWAKE_TEST_WARP"] = warp
    if mode == "headless":
        env["BLUEWAKE_RENDERER"] = "headless"
    _isolate(env, scratch, card)
    allowed = {"BLUEWAKE_NATIVE_WORKERS", "BLUEWAKE_GAMETHREAD_DIAGNOSTICS"}
    for key, value in (overrides or {}).items():
        if key not in allowed or not str(value).isdigit():
            raise ValueError(f"invalid benchmark override: {key}={value}")
        env[key] = str(value)
    return env


def setup_environment(base, *, scratch, card, state_path) -> dict[str, str]:
    """Headless continue that only writes the state the measurements load."""
    env = _child_base(base)
    env.update({
        "BLUEWAKE_MAX_RETRACES": str(SETUP_CAP),
        "BLUEWAKE_WALL_PACE": "0",
        "BLUEWAKE_PERF_LOG": "1",
        "DOL_AURORA_FRAME_INTERP": "0",
        "BLUEWAKE_PAD_BUTTONS": "0x0100",
        "BLUEWAKE_PAD_PULSE_ON_TITLE_READY": "1",
        "BLUEWAKE_PAD_PULSE_LENGTH": "2",
        "BLUEWAKE_PAD_SCRIPT": continue_pad_script(),
        "BLUEWAKE_RENDERER": "headless",
        "BLUEWAKE_SAVE_STATE": f"{state_path}@{SAVE_AT}",
    })
    _isolate(env, scratch, card)
    return env


def _mean(values: list[float]):
    if not values:
        return None
    return round(sum(values) / len(values), 4)


def _checkpoint(match: re.Match) -> dict:
    fields = match.groupdict()
    return {
        "retrace": int(fields["retrace"]),
        "cycle": int(fields["cycle"]),
        "cpu": fields["cpu"].upper(),
        "mem1": fields["mem1"].upper(),
        "mem2": fields["mem2"].upper(),
        "aliases": fields["aliases"].upper(),
        "alias_spans": int(fields["alias_spans"]),
        "alias_bytes": int(fields["alias_bytes"]),
    }


def scan_log(text: str):
    perfs = []
    dips = []
    loads = []
    checkpoints = []
    for line in text.splitlines():
        if line.startswith("[perf] "):
            match = PERF.fullmatch(line)
            if match:
                perfs.append((int(match["retrace"]), float(match["rate"]), float(match["busy"])))
        elif line.startswith("[fps-dip] "):
            match = DIP.search(line)
            if match:
                dips.append((int(match["retrace"]), match["cause"]))
        elif line.startswith("[load] "):
            match = LOAD.search(line)
            if match:
                loads.append((int(match["retrace"]), match["stage"]))
        elif line.startswith("[guest-checkpoint] "):
            failed = FAILED_CHECKPOINT.fullmatch(line)
            if failed:
                checkpoints.append({"retrace": int(failed["retrace"]), "failed": True})
                continue
            match = CHECKPOINT.fullmatch(line)
            if match:
                checkpoints.append(_checkpoint(match))
    return perfs, dips, loads, checkpoints


def _window(perfs, dips, loads, begin: int, end: int, expected_stage, code: str, name: str) -> dict:
    measure_from = begin + SETTLE
    rates = [rate / 2.0 for retrace, rate, _busy in perfs if measure_from <= retrace < end]
    busies = [busy for retrace, _rate, busy in perfs if measure_from <= retrace < end]
    causes = []
    for retrace, cause in dips:
        if measure_from <= retrace < end and cause not in causes:
            causes.append(cause)
    seen = [stage for retrace, stage in loads if begin <= retrace < end]
    if expected_stage is None:
        loaded = bool(seen)
    else:
        loaded = expected_stage in seen
    return {
        "retrace": begin,
        "code": code,
        "name": name,
        "loaded": loaded,
        "game_frames_per_second": _mean(rates),
        "busy_percent": _mean(busies),
        "dip_causes": causes,
    }


def summarize_log(text: str, stops, stop_retraces: int = 1800, start: int = START) -> dict:
    perfs, dips, loads, checkpoints = scan_log(text)
    reports = []
    for index, stop in enumerate(stops):
        begin = start + index * stop_retraces
        reports.append(_window(
            perfs, dips, loads, begin, begin + stop_retraces, stop.stage, stop.code, stop.name,
        ))
    return {"stops": reports, "checkpoints": checkpoints}


def summarize_state_log(text: str, stop_retraces: int) -> dict:
    """One window after a loaded state: skip 300 retraces, then one stop."""
    perfs, dips, loads, checkpoints = scan_log(text)
    match = STATE_LOADED.search(text)
    if match is None:
        return {"stops": [], "checkpoints": checkpoints}
    begin = int(match["retrace"])
    seen = [stage for retrace, stage in loads if begin <= retrace < begin + stop_retraces]
    code = seen[-1] if seen else "state"
    report = _window(perfs, dips, loads, begin, begin + stop_retraces, None, code, "saved state")
    return {"stops": [report], "checkpoints": checkpoints}


def _norm_checkpoint(record: dict) -> dict:
    retrace = int(record["retrace"])
    if record.get("failed"):
        return {"retrace": retrace, "failed": True}
    return {
        "retrace": retrace,
        "cycle": int(record["cycle"]),
        "cpu": str(record["cpu"]).upper(),
        "mem1": str(record["mem1"]).upper(),
        "mem2": str(record["mem2"]).upper(),
        "aliases": str(record["aliases"]).upper(),
        "alias_spans": int(record["alias_spans"]),
        "alias_bytes": int(record["alias_bytes"]),
    }


def first_differing_retrace(left: list, right: list):
    """The earliest retrace whose checkpoint is not the same.

    A checkpoint that matches is not named, so when only a later sample
    differs, that later retrace is the one reported.
    """
    by_left = {int(item["retrace"]): _norm_checkpoint(item) for item in left}
    by_right = {int(item["retrace"]): _norm_checkpoint(item) for item in right}
    for retrace in sorted(set(by_left) | set(by_right)):
        if by_left.get(retrace) != by_right.get(retrace):
            return retrace
    return None


def _delta(before, after, places: int, suffix: str = "") -> str:
    if before is None or after is None:
        return "n/a"
    if before == 0:
        percent = "n/a"
    else:
        percent = f"{(after - before) / before * 100:+.1f}%"
    return f"{before:.{places}f}{suffix} -> {after:.{places}f}{suffix} ({percent})"


def _speed_note(before, after) -> str:
    if before is None or after is None or before == 0:
        return ""
    if abs((after - before) / before * 100) > 3:
        return " speed outside 3%"
    return ""


def compare_summaries(left: dict, right: dict) -> tuple[int, str]:
    """Print the speed and busy change. Non-zero only when checkpoints differ."""
    lines = []
    differ_at = None
    modes = list(dict.fromkeys([*left.get("modes", {}), *right.get("modes", {})]))
    for mode in modes:
        before = left.get("modes", {}).get(mode)
        after = right.get("modes", {}).get(mode)
        if len(modes) > 1:
            lines.append(f"{mode}:")
        if before is None or after is None:
            lines.append(f"{mode}: missing from one summary")
            continue
        stops_before = before.get("stops", [])
        stops_after = after.get("stops", [])
        for stop_before, stop_after in zip(stops_before, stops_after):
            speed = _delta(
                stop_before.get("game_frames_per_second"),
                stop_after.get("game_frames_per_second"),
                2,
            )
            busy = _delta(stop_before.get("busy_percent"), stop_after.get("busy_percent"), 1, "%")
            note = _speed_note(
                stop_before.get("game_frames_per_second"),
                stop_after.get("game_frames_per_second"),
            )
            lines.append(f"{stop_before['code']} {stop_before['name']}: speed {speed}, busy {busy}{note}")
        found = first_differing_retrace(before.get("checkpoints", []), after.get("checkpoints", []))
        if found is not None and differ_at is None:
            differ_at = found
    if differ_at is None:
        lines.append("checkpoints match")
        return 0, "\n".join(lines) + "\n"
    lines.append(f"checkpoints differ at retrace {differ_at}")
    return 1, "\n".join(lines) + "\n"


def qualification_errors(summary: dict) -> list[str]:
    errors = []
    expected_modes = summary.get("requested_modes")
    expected_codes = summary.get("expected_stop_codes")
    modes = summary.get("modes")
    if not isinstance(expected_modes, list) or not expected_modes:
        errors.append("missing requested_modes")
        return errors
    if expected_codes is not None and (not isinstance(expected_codes, list) or not expected_codes):
        errors.append("missing expected_stop_codes")
    if not isinstance(modes, dict) or set(modes) != set(expected_modes):
        errors.append("measured modes do not match requested_modes")
        return errors
    checkpoint_grid = None
    for mode in expected_modes:
        body = modes[mode]
        stops = body.get("stops", [])
        actual_codes = [stop.get("code") for stop in stops]
        if expected_codes is None and len(stops) != 1:
            errors.append(f"{mode}: state run did not produce exactly one stop")
        elif expected_codes is not None and actual_codes != expected_codes:
            errors.append(f"{mode}: incomplete stop grid")
        if any(not stop.get("loaded") for stop in stops):
            errors.append(f"{mode}: one or more scenes did not load")
        checkpoints = body.get("checkpoints", [])
        retraces = [item.get("retrace") for item in checkpoints]
        if not retraces or len(retraces) != len(set(retraces)):
            errors.append(f"{mode}: missing or duplicate checkpoints")
        if any(item.get("failed") for item in checkpoints):
            errors.append(f"{mode}: checkpoint generation failed")
        if checkpoint_grid is None:
            checkpoint_grid = retraces
        elif retraces != checkpoint_grid:
            errors.append(f"{mode}: checkpoint grid differs between modes")
    return errors


def _read_state_retrace(path: Path):
    """Retrace stored in a BWSTATE1 HEADER chunk, or None if this is not one.

    HostStateHeader in main.c lays the retrace down as the little-endian u64
    at byte 32 of that chunk. The file is gzip. Only the header is read.
    """
    try:
        with gzip.open(path, "rb") as handle:
            if handle.read(8) != b"BWSTATE1":
                return None
            handle.read(4)
            tag = handle.read(16)
            if not tag.startswith(b"HEADER"):
                return None
            size = int.from_bytes(handle.read(8), "little")
            if size < 40:
                return None
            header = handle.read(40)
    except OSError:
        return None
    if len(header) < 40:
        return None
    return int.from_bytes(header[32:40], "little")


def _run_app(app: str, env: dict[str, str], log_path: Path) -> int:
    with log_path.open("w", encoding="utf-8") as log:
        completed = subprocess.run([app], env=env, stdout=log, stderr=subprocess.STDOUT)
    return completed.returncode


def _prepare_scratch(scratch: Path) -> None:
    scratch.mkdir(parents=True, exist_ok=True)
    settings = scratch / "settings.ini"
    if not settings.exists():
        settings.write_text("")


def _modes(mode: str) -> list[str]:
    if mode == "both":
        return ["rendered", "headless"]
    return [mode]


def run_tour(card, app, *, stop_retraces: int, untrained: bool, mode: str,
             state, workers=None) -> int:
    stops = UNTRAINED_STOPS if untrained else DEFAULT_STOPS
    end = tour_end(stops, stop_retraces)
    warp = warp_string(stops, stop_retraces)
    scratch = Path(tempfile.mkdtemp(prefix="bluewake-bench-tour-"))
    _prepare_scratch(scratch)
    card_out = scratch / "GZLE01.card"
    base = dict(os.environ)
    print(f"scratch {scratch}", file=sys.stderr)
    failures = 0
    if state:
        load_state = Path(state)
        origin = _read_state_retrace(load_state)
        cap = end if origin is None else origin + stop_retraces
        warp_value = None
    else:
        print(" ".join(card_restart_argv(card, card_out)))
        restarted = subprocess.run(card_restart_argv(card, card_out))
        if restarted.returncode != 0:
            return restarted.returncode
        state_path = scratch / "tour.bwstate"
        setup = setup_environment(base, scratch=scratch, card=card_out, state_path=state_path)
        setup_log = scratch / "setup.log"
        setup_code = _run_app(app, setup, setup_log)
        if setup_code != 0 or not state_path.is_file():
            print(f"setup failed ({setup_code}); log {setup_log}", file=sys.stderr)
            return setup_code or 1
        load_state = state_path
        cap = end
        warp_value = warp
    modes = {}
    for mode_name in _modes(mode):
        env = measurement_environment(
            base,
            mode=mode_name,
            load_state=load_state,
            scratch=scratch,
            card=card_out,
            warp=warp_value,
            max_retraces=cap,
            overrides=({"BLUEWAKE_NATIVE_WORKERS": workers,
                        "BLUEWAKE_GAMETHREAD_DIAGNOSTICS": 1}
                       if workers is not None else None),
        )
        log_path = scratch / f"{mode_name}.log"
        code = _run_app(app, env, log_path)
        text = log_path.read_text(encoding="utf-8", errors="replace")
        if state:
            modes[mode_name] = summarize_state_log(text, stop_retraces)
        else:
            modes[mode_name] = summarize_log(text, stops, stop_retraces)
        if code != 0:
            print(f"{mode_name} exited {code}; log {log_path}", file=sys.stderr)
            failures += 1
    summary = {
        "stop_retraces": stop_retraces,
        "tour_end": cap,
        "warp": warp_value,
        "untrained": untrained,
        "modes": modes,
        "requested_modes": _modes(mode),
        # Routed tours have an exact scene grid. A state file names its scene
        # only after it is loaded, so qualification instead requires one
        # successfully loaded stop for that mode.
        "expected_stop_codes": [stop.code for stop in stops] if not state else None,
    }
    Path("summary.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print("wrote summary.json")
    for mode_name, body in modes.items():
        print(mode_name)
        for stop in body["stops"]:
            fps = stop["game_frames_per_second"]
            busy = stop["busy_percent"]
            fps_text = "n/a" if fps is None else f"{fps:.2f}"
            busy_text = "n/a" if busy is None else f"{busy:.1f}%"
            loaded = "loaded" if stop["loaded"] else "not loaded"
            causes = ", ".join(stop["dip_causes"]) or "none"
            print(f"  {stop['retrace']} {stop['code']} {stop['name']}: "
                  f"{fps_text} game frames/s, busy {busy_text}, {loaded}, dips: {causes}")
        print(f"  {len(body['checkpoints'])} checkpoints")
    return 1 if failures else 0


def _compare_main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(prog="bench_tour.py compare")
    parser.add_argument("left", type=Path)
    parser.add_argument("right", type=Path)
    parser.add_argument("--qualification", action="store_true",
                        help="reject incomplete mode, stop, load or checkpoint evidence")
    args = parser.parse_args(argv)
    try:
        left = json.loads(args.left.read_text(encoding="utf-8"))
        right = json.loads(args.right.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        print(f"compare: {error}", file=sys.stderr)
        return 1
    if args.qualification:
        errors = [*(f"left: {error}" for error in qualification_errors(left)),
                  *(f"right: {error}" for error in qualification_errors(right))]
        if errors:
            sys.stderr.write("qualification incomplete: " + "; ".join(errors) + "\n")
            return 3
    code, text = compare_summaries(left, right)
    sys.stdout.write(text)
    return code


def _run_main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(
        prog="bench_tour.py",
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("card", help="memory card. Unused when --state is set; the player's card is never modified.")
    parser.add_argument("app", help="BlueWake executable")
    parser.add_argument("--stop-retraces", type=int, default=1800,
                        help="retraces at each stop (default 1800)")
    parser.add_argument("--untrained", action="store_true",
                        help="measure M_Dai:0:0 and kaze:0:0 instead of the five default stops")
    parser.add_argument("--mode", choices=("rendered", "headless", "both"), default="rendered",
                        help="rendered is the default. headless sets BLUEWAKE_RENDERER=headless. "
                             "On Windows that variable is only filled in when unset; "
                             "if headless does not run, use rendered.")
    parser.add_argument("--state", metavar="FILE",
                        help="load this state instead of the card route; does not set BLUEWAKE_TEST_WARP")
    parser.add_argument("--workers", type=int, choices=range(0, 9), metavar="N",
                        help="explicit native worker count; also enables qualification diagnostics")
    args = parser.parse_args(argv)
    if args.stop_retraces <= 0:
        parser.error("--stop-retraces must be positive")
    return run_tour(
        args.card,
        args.app,
        stop_retraces=args.stop_retraces,
        untrained=args.untrained,
        mode=args.mode,
        state=args.state,
        workers=args.workers,
    )


def main(argv: list[str] | None = None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)
    if argv and argv[0] == "compare":
        return _compare_main(argv[1:])
    return _run_main(argv)


if __name__ == "__main__":
    raise SystemExit(main())
