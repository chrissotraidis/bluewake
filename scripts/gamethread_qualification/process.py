import os
import signal
import subprocess
import time
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class RunResult:
    argv: tuple
    returncode: int
    timed_out: bool
    elapsed_seconds: float
    log_path: Path


def _stop(process, grace_seconds):
    try:
        if os.name == "posix":
            os.killpg(process.pid, signal.SIGTERM)
        else:
            process.terminate()
    except ProcessLookupError:
        process.wait()
        return
    try:
        process.wait(timeout=grace_seconds)
        return
    except subprocess.TimeoutExpired:
        pass
    try:
        if os.name == "posix":
            os.killpg(process.pid, signal.SIGKILL)
        else:
            process.kill()
    except ProcessLookupError:
        pass
    process.wait()


def run_process(argv, output_dir, name, timeout_seconds, *, cwd=None, env=None,
                grace_seconds=2.0):
    if not argv or timeout_seconds <= 0:
        raise ValueError("argv must be nonempty and timeout_seconds must be positive")
    directory = Path(output_dir)
    directory.mkdir(parents=True, exist_ok=True)
    log_path = directory / f"{name}.log"
    start = time.monotonic()
    with log_path.open("wb") as log:
        process = subprocess.Popen(
            list(argv), cwd=cwd, env=env, stdin=subprocess.DEVNULL,
            stdout=log, stderr=subprocess.STDOUT,
            start_new_session=(os.name == "posix"))
        timed_out = False
        try:
            returncode = process.wait(timeout=timeout_seconds)
        except subprocess.TimeoutExpired:
            timed_out = True
            _stop(process, grace_seconds)
            returncode = process.returncode
    return RunResult(tuple(argv), returncode, timed_out,
                     time.monotonic() - start, log_path)
