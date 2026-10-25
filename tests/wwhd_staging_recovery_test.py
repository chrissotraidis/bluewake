#!/usr/bin/env python3
"""Real killed-child output recovery, using only the existing synthetic pack fixture.

argv[1] is the native import CLI. No private game data, keys, emulator or UI.
The ready-line barriers prove the child reached each boundary before kill().
"""
from __future__ import annotations
import os
from pathlib import Path
import queue
import subprocess
import sys
import tempfile
import threading
import time

sys.dont_write_bytecode = True
from wwhd_disc_import_test import make_bfres, make_gamecube, check_pack, ROOT

PREFIX = ".bluewake-wwhd-stage-v1-"
LOCK = ".bluewake-wwhd-import.lock"


def snapshot(root):
    result = {}
    def visit(folder):
        for entry in os.scandir(folder):
            path = Path(entry.path)
            name = path.relative_to(root).as_posix()
            stat = entry.stat(follow_symlinks=False)
            reparse = bool(getattr(stat, "st_file_attributes", 0) & 0x400)
            if entry.is_symlink() or reparse:
                result[name] = ("link", os.readlink(path))
            elif entry.is_dir(follow_symlinks=False):
                result[name] = None
                visit(path)
            elif path.name == LOCK:
                # Windows byte-range locks also deny reads. Check the empty
                # lock's identity and metadata without reading its locked byte.
                assert stat.st_size == 0
                result[name] = ("lock", stat.st_ino, stat.st_size, stat.st_mtime_ns)
            else:
                result[name] = path.read_bytes()
    visit(root)
    return result


def run(cli, root):
    source = root / "source"
    source.mkdir()
    (source / "A.bfres").write_bytes(make_bfres())
    disc = root / "synthetic.iso"
    disc.write_bytes(make_gamecube())
    parent = root / "é_日本語_😀"
    parent.mkdir()
    env = {k: v for k, v in os.environ.items() if not k.startswith("WWHD_TEST_")}
    env["WWHD_EXACT_REPORT"] = "1"
    def command(output):
        return [str(cli), str(source), str(disc), str(output)]
    def invoke(output, expected=0):
        result = subprocess.run(command(output), env=env, capture_output=True, text=True, timeout=20)
        assert result.returncode == expected, (result.returncode, result.stdout, result.stderr)
        return result
    def jobs():
        return sorted(p for p in parent.iterdir() if p.name.startswith(PREFIX))
    children = []
    def pause(output, point):
        child = subprocess.Popen(command(output), env=dict(env, WWHD_TEST_PAUSE=point),
                                 stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        children.append(child)
        lines = queue.Queue()
        def consume():
            with child.stderr:
                for line in child.stderr:
                    lines.put(line)
            lines.put(None)
        threading.Thread(target=consume, daemon=True).start()
        transcript = []
        deadline = time.monotonic() + 20
        while True:
            line = lines.get(timeout=max(0.01, deadline - time.monotonic()))
            transcript.append(line)
            assert line is not None, transcript
            if line.rstrip() == "Test paused: " + point:
                return child
    def kill(child):
        assert child.poll() is None
        child.kill()  # SIGKILL on POSIX; TerminateProcess on Windows. No destructors.
        assert child.wait(timeout=10) != 0
        child.stdin.close()
        child.stdout.close()
    def retry_after_kill(output, extra_env=None, expected=0):
        # Windows may release process locks shortly after the child is reaped.
        deadline = time.monotonic() + 10
        while True:
            result = subprocess.run(command(output), env=dict(env, **(extra_env or {})),
                                    capture_output=True, text=True, timeout=20)
            if result.returncode and "another import" in result.stderr:
                assert time.monotonic() < deadline, result.stderr
                time.sleep(0.05)
                continue
            assert result.returncode == expected, (result.returncode, result.stderr)
            return result
    try:
        invoke(parent / "existing")
        expected = check_pack(parent / "existing")
        complete = snapshot(parent / "existing")
        legacy = parent / ".WWHD-123456"
        legacy.mkdir(); (legacy / "keep").write_bytes(b"legacy unmarked data")
        # Unrelated non-ASCII sibling names must never enter a locale-dependent
        # narrow conversion while the recovery scanner decides what it owns.
        (parent / "既存パック_é_😀").mkdir()
        (parent / "既存パック_é_😀/keep").write_bytes(b"unrelated Unicode sibling")
        child = pause(parent / "killed", "stage:Finishing the pack")
        assert len(jobs()) == 1
        abandoned = jobs()[0]
        assert list((abandoned / "pack").glob("*.png")) and (abandoned / "owner").is_file()
        before = snapshot(parent)
        busy = invoke(parent / "contender", 1)
        assert "another import" in busy.stderr and snapshot(parent) == before
        invoke(root / "different-parent/pack")
        kill(child)
        assert abandoned.exists() and not (parent / "killed").exists()
        retry_after_kill(parent / "recovered")
        assert not abandoned.exists() and not jobs()
        assert check_pack(parent / "recovered") == expected and snapshot(parent / "existing") == complete
        assert (legacy / "keep").read_bytes() == b"legacy unmarked data"
        print("live contention, different parent, real kill/recovery and completed-pack preservation passed")

        # A kill during recovery leaves its marker until the remaining files go.
        child = pause(parent / "second-kill", "stage:Finishing the pack")
        kill(child)
        abandoned = jobs()[0]
        file_count = len(list((abandoned / "pack").iterdir()))
        cancelled = retry_after_kill(parent / "recovery-cancelled",
                                    {"WWHD_TEST_CANCEL": "recovery-file-removed"}, 3)
        assert "Import cancelled" in cancelled.stderr, cancelled.stderr
        assert (abandoned / "owner").is_file() and not (parent / "recovery-cancelled").exists()
        assert len(list((abandoned / "pack").iterdir())) == file_count - 1
        child = pause(parent / "recovery-interrupted", "recovery-file-removed")
        assert (abandoned / "owner").is_file()
        kill(child)
        retry_after_kill(parent / "recovery-retried")
        assert not abandoned.exists() and check_pack(parent / "recovery-retried") == expected
        child = pause(parent / "marker-only", "staging-marker-ready")
        abandoned = jobs()[0]
        assert (abandoned / "owner").is_file() and not (abandoned / "pack").exists()
        kill(child)
        retry_after_kill(parent / "after-marker-kill")
        assert not abandoned.exists()

        child = pause(parent / "committed-before-kill", "staging-published")
        committed = snapshot(parent / "committed-before-kill")
        assert check_pack(parent / "committed-before-kill") == expected
        assert len(jobs()) == 1 and not (jobs()[0] / "pack").exists()
        kill(child)
        retry_after_kill(parent / "after-commit-kill")
        assert not jobs() and snapshot(parent / "committed-before-kill") == committed
        print("recovery cancellation/interruption, marker-before-payload and post-rename kill preservation passed")

        def wrapper(n, marker=True):
            ident = f"{n:032x}"
            path = parent / (PREFIX + ident)
            path.mkdir()
            if marker:
                (path / "owner").write_bytes(("BlueWake WWHD output stage v1\n" + ident + "\n").encode("ascii"))
            return path
        unmarked = wrapper(1, False); (unmarked / "keep").write_bytes(b"unknown")
        bad = wrapper(2); (bad / "owner").write_bytes(b"wrong marker")
        mismatch = wrapper(3); (mismatch / "owner").write_bytes(b"BlueWake WWHD output stage v1\n" + b"0" * 32 + b"\n")
        extra = wrapper(9); (extra / "owner").write_bytes((extra / "owner").read_bytes() + b"extra")
        nested = wrapper(4); (nested / "pack/nested").mkdir(parents=True)
        (nested / "pack/nested/keep").write_bytes(b"not importer output")
        outside = root / "outside"; outside.mkdir(); (outside / "tex1_keep.png").write_bytes(b"outside sentinel")
        linked = wrapper(5)
        try:
            (linked / "pack").symlink_to(outside, target_is_directory=True)
        except OSError:
            assert os.name == "nt"
            subprocess.run(["cmd", "/c", "mklink", "/J", str(linked / "pack"), str(outside)], check=True, capture_output=True)
        # File symlinks need Developer Mode/privilege on Windows; the required
        # directory reparse case above still exercises refusal there.
        file_links = 0
        try:
            file_link = wrapper(6); (file_link / "pack").mkdir()
            (file_link / "pack/tex1_link.png").symlink_to(outside / "tex1_keep.png")
            marker_link = wrapper(7, False)
            (marker_link / "owner").symlink_to(bad / "owner")
            (parent / (PREFIX + "8" * 32)).symlink_to(outside, target_is_directory=True)
            file_links = 3
        except OSError:
            assert os.name == "nt"
            # Remove only our failed setup; no valid marked empty jobs in snapshot.
            for n in (6, 7):
                p = parent / (PREFIX + f"{n:032x}")
                if p.exists():
                    import shutil
                    shutil.rmtree(p)
        before = snapshot(parent)
        outside_before = snapshot(outside)
        invoke(parent / "with-sentinels")
        after = snapshot(parent)
        assert {k: v for k, v in after.items() if k != "with-sentinels" and not k.startswith("with-sentinels/")} == before
        assert snapshot(outside) == outside_before and snapshot(parent / "existing") == complete
        assert check_pack(parent / "with-sentinels") == expected
        print(f"Unicode, malformed/unmarked/nested sentinels and no-follow directory checks passed ({file_links} file/link cases)")
    finally:
        for child in children:
            if child.poll() is None:
                child.kill(); child.wait(timeout=10)
        # Python's TemporaryDirectory owns all generated data, including sentinels.


if __name__ == "__main__":
    assert len(sys.argv) == 2
    scratch = ROOT / "build/wwhd-disc-import"
    scratch.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="staging-recovery-", dir=scratch) as directory:
        run(Path(sys.argv[1]).resolve(), Path(directory))
