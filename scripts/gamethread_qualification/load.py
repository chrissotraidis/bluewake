import os
import signal
import subprocess
import sys
from contextlib import contextmanager
from pathlib import Path


def staged_matrix(worker_counts=(0, 1, 2, 3)):
    rows = []
    for renderer in ("headless", "rendered"):
        for workers in worker_counts:
            rows.append({"stage": "A" if renderer == "headless" else "B",
                         "renderer": renderer, "workers": workers,
                         "placement": "default", "pressure": "none"})
    rows.extend([
        {"stage": "C", "renderer": "rendered", "workers": 2,
         "placement": "separate-physical", "pressure": "none"},
        {"stage": "C", "renderer": "rendered", "workers": 2,
         "placement": "shared-smt", "pressure": "none"},
    ])
    rows.extend([
        {"stage": "D", "renderer": "rendered", "workers": 2,
         "placement": "default", "pressure": kind}
        for kind in ("remote-core", "game-thread-sibling", "all-core")
    ])
    return rows


def interleave(control, candidate, repeats=3):
    if repeats < 1:
        raise ValueError("repeats must be positive")
    return [item for _ in range(repeats) for item in (control, candidate, candidate, control)]


def pressure_argv(seconds, duty):
    if seconds <= 0 or not 1 <= duty <= 100:
        raise ValueError("pressure duration and duty cycle are out of range")
    worker = Path(__file__).with_name("pressure_worker.py")
    return [sys.executable, str(worker), "--seconds", str(seconds), "--duty", str(duty)]


def _terminate_group(process, grace=2):
    if process.poll() is not None:
        return
    if os.name == "posix":
        os.killpg(process.pid, signal.SIGTERM)
    else:
        process.terminate()
    try:
        process.wait(timeout=grace)
    except subprocess.TimeoutExpired:
        if os.name == "posix":
            os.killpg(process.pid, signal.SIGKILL)
        else:
            process.kill()
        process.wait()


@contextmanager
def pressure_process(seconds, duty, *, stdout=subprocess.DEVNULL):
    """Own one bounded pressure process and always reap its process group."""
    process = subprocess.Popen(pressure_argv(seconds, duty), stdin=subprocess.DEVNULL,
                               stdout=stdout, stderr=subprocess.STDOUT,
                               start_new_session=(os.name == "posix"))
    try:
        yield process
    finally:
        _terminate_group(process)


def validate_soak_plan(plan):
    required = {"transition", "cutscene-music", "sailing", "pressure", "save-reload"}
    if plan.get("minutes", 0) < 30:
        raise ValueError("paired soak must run for at least 30 minutes")
    stages = plan.get("stages")
    if not isinstance(stages, list) or set(stages) != required:
        raise ValueError("soak plan must contain each required stage exactly once")
    if plan.get("source_card") == plan.get("working_card"):
        raise ValueError("soak must use a copied working card")
    return plan
