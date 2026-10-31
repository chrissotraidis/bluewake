#!/usr/bin/env python3
"""A stand-in for BlueWake in tests/test_render_bench.py: the log lines and frames the scripts read.

Its file name picks how it behaves: "bright" draws a brighter picture than "plain", "slow" runs at 50 retraces
a second instead of 120 with a GPU twice as busy, "diverge" reports a different guest checkpoint at retrace
2700, "hang" never ends, "empty" finds quest log 1 empty on a stage-select warp and then never ends (as the
game waits at the file select), "empty12" finds quest logs 1 and 2 empty, "late" captures each frame a retrace late, "realtime" works 50 ms
for each [perf] line (60 retraces) for a profiler to attach to. From a state (BLUEWAKE_LOAD_STATE) it runs from the state's
retrace to BLUEWAKE_MAX_RETRACES, with a [load] line 30 retraces after each test warp. A stage-select warp
arrives at retrace 840. FAKE_BLUEWAKE_SLOW=1 is "slow" whatever the name. BLUEWAKE_SAVE_STATE writes a state with only its header, on the retrace after the one
asked for, as the game does.
"""
import gzip
import os
import sys
import time
from pathlib import Path

name = sys.argv[0]  # its path: the behaviours below are words in the file's or its folder's name
env = os.environ
quest_log = int(env.get("BLUEWAKE_WARP_FILE", "1"))
if env.get("BLUEWAKE_WARP") and (("empty" in name and quest_log == 1) or ("empty12" in name and quest_log == 2)):
    print(f"[warp] quest log {quest_log} is empty; stopped pressing A", flush=True)
    time.sleep(600)
if "hang" in name:
    time.sleep(600)
shade = 140 if "bright" in name else 100
W, H = 32, 24


def picture(n):
    rows = bytearray()
    for y in range(H):
        for x in range(W):
            v = shade + (x + n) % 8
            rows += bytes((v, v, v))
    return f"P6 {W} {H} 255\n".encode() + bytes(rows)


def state_retrace(path):
    with gzip.open(path, "rb") as handle:
        return int.from_bytes(handle.read(76)[68:76], "little")


def save_state(path, retrace):
    with gzip.open(path, "wb") as out:
        out.write(b"BWSTATE1" + bytes(4) + b"HEADER".ljust(16, b"\0") + (40).to_bytes(8, "little")
                  + bytes(32) + retrace.to_bytes(8, "little"))
    print(f"[state] saved {path} retrace={retrace}")


start = state_retrace(env["BLUEWAKE_LOAD_STATE"]) if env.get("BLUEWAKE_LOAD_STATE") else 600
end = int(env.get("BLUEWAKE_MAX_RETRACES", start + 720))
if env.get("BLUEWAKE_LOAD_STATE"):
    print(f"[state] loaded {env['BLUEWAKE_LOAD_STATE']} retrace={start} pc=0x80322BC8 mem1=0x0 fields=1")
if env.get("BLUEWAKE_CAPTURE_OPENING_FRAME"):
    folder = Path(env["BLUEWAKE_CAPTURE_OPENING_FRAME"]).parent
    first, every = int(env["BLUEWAKE_CAPTURE_RETRACE"]), int(env["BLUEWAKE_CAPTURE_INTERVAL"])
    for n, retrace in enumerate(range(first, end, every)):
        # "late" captures a retrace after the one asked for, as an unpaced run can.
        (folder / f"frame-{retrace + ('late' in name)}.ppm").write_bytes(picture(n))
warps = [item.split(":") for item in env.get("BLUEWAKE_TEST_WARP", "").split(",") if item]
saves = [item.rpartition("@") for item in env.get("BLUEWAKE_SAVE_STATE", "").split(",") if item]
# FAKE_BLUEWAKE_SLOW=1 makes any of them slow too: one app, a setting apart (render_bench --set-a / --set-b).
slow = "slow" in name or env.get("FAKE_BLUEWAKE_SLOW") == "1"
rate = 50.0 if slow else 120.0
gpu = 8.0 if slow else 4.0
checkpoint = int(env.get("BLUEWAKE_GUEST_CHECKPOINT_INTERVAL", "0"))
for retrace in range(start - start % 60 + 60, end, 60):
    for at, stage, *_ in warps:
        if retrace - 60 < int(at) + 30 <= retrace:
            print(f"[load] retrace={int(at) + 30} overlap=0x0 peek=0 fader=0 alpha=0 stage={stage} cpu_us=1")
    if env.get("BLUEWAKE_WARP") and retrace == 840:
        print("[warp] arrived: Link is in the room")
    print(f"[perf] retrace={retrace} rate={rate} worst_ms=17.0 hitches=0 busy={60 + retrace % 120 // 60}.0%")
    if "realtime" in name:
        # Work for 50 ms a [perf] line, as the game does between them: a profiler can attach and detach on time.
        until = time.time() + 0.05
        while time.time() < until:
            pass
        sys.stdout.flush()
    print(f"[gpu-prof] frames=240 frame_ms=16.6 max=17.0 busy_ms={gpu} | ssao {gpu / 2:.2f} x1")
    if checkpoint and retrace % checkpoint == 0:
        mem = "D" if "diverge" in name and retrace >= 2700 else "C"
        print(f"[guest-checkpoint] version=1 retrace={retrace} cycle={retrace * 8100000} cpu={'A' * 32} "
              f"mem1={mem * 32} mem2={'B' * 32} aliases={'E' * 32} alias_spans=2 alias_bytes=1024")
for path, _, at in saves:
    save_state(path, int(at) + 1)
sys.exit(1 if env.get("BLUEWAKE_WARP") else 0)
