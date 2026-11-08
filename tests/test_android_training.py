#!/usr/bin/env python3
"""Android training acceptance checks; synthetic adb replies, no device or game."""
import importlib.util
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

REPO = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("android_builder", REPO / "scripts/android/build.py")
ab = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ab)


class DeviceTraining(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        args = ab.parse_args(["GZLE01.iso", "--out", str(self.root / "out"), "--jobs", "1"])
        self.builder = ab.AndroidBuilder(args)

    def playback(self, control=False, retraces=None, profile=True, marker=False):
        finished = "retrace=100\n[android] host returned\noptimization profile written\n"
        if control:
            finished += "[player-milestone] control-admitted\n"
        # Include an ordinary progress poll before termination: this must not
        # change whether the run is a full training playback or a short probe.
        logs = iter(["retrace=100\n", finished, finished])
        first = "session-in-progress" if marker else ""
        listings = iter([first, "session-in-progress session-fixture.log" if marker else "session-fixture.log"])
        files = self.builder.DEVICE_FILES

        def adb(*command, check=True, capture=False, timeout=None):
            if command == ("shell", f"ls {files}/logs"):
                return next(listings)
            if command == ("shell", f"cat {files}/logs/session-fixture.log"):
                return next(logs)
            if command == ("shell", f"ls {files}/pgo/plain"):
                return "fixture.profraw" if profile else ""
            if command[0] == "pull":
                Path(command[2]).write_bytes(b"synthetic profile")
            return "" if capture else SimpleNamespace(stdout="")

        with patch.object(self.builder, "adb", side_effect=adb), patch.object(ab.time, "sleep"):
            return self.builder.device_training_run(self.root / "run", "plain", None, retraces=retraces)

    def test_full_training_rejects_exit_before_player_control(self):
        with self.assertRaisesRegex(ab.wb.BuildError, "did not reach player control"):
            self.playback()

    def test_full_training_accepts_player_control_and_profile(self):
        profiles = self.playback(control=True)
        self.assertEqual(len(profiles), 1)
        self.assertEqual(profiles[0].read_bytes(), b"synthetic profile")

    def test_short_profile_probe_does_not_require_player_control(self):
        self.assertEqual(len(self.playback(retraces=600)), 1)

    def test_short_profile_probe_still_requires_a_profile(self):
        with self.assertRaisesRegex(ab.wb.BuildError, "wrote no profile"):
            self.playback(retraces=600, profile=False)

    def test_the_session_marker_beside_the_logs_is_not_read_as_the_log(self):
        # The app keeps `session-in-progress` in the logs directory and it sorts
        # after every `session-*.log`: reading it finds no `host returned` line,
        # which used to kill a playback that was running.
        self.assertEqual(len(self.playback(control=True, retraces=600, marker=True)), 1)


class StalledPlayback(unittest.TestCase):
    """#571: a playback that stops advancing has to say where it stopped."""

    # A synthetic /proc/<pid>/task/*/stat: the guest thread burning a core, the
    # rest asleep. utime is fields[11] and stime fields[12] after the comm.
    CENSUS = (
        "11054 (SDLThread) R 10828 10828 0 0 -1 4194624 0 0 0 0 {utime} 11 0 0 20 0 1 0 12345 67890\n"
        "10828 (SDLActivity) S 1002 1002 0 0 -1 4194560 0 0 0 0 19 0 0 0 20 0 1 0 12344 67890\n"
        "11044 (binder:10828_1) S 1002 1002 0 0 -1 4194560 0 0 0 0 2 0 0 0 20 0 1 0 12343 67890\n"
    )

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        args = ab.parse_args(["GZLE01.iso", "--out", str(self.root / "out"), "--jobs", "1"])
        self.builder = ab.AndroidBuilder(args)
        self.builder.STALL_SECONDS = 60
        self.run_dir = self.root / "run"

    def stall(self, census=None):
        files = self.builder.DEVICE_FILES
        frozen = "retrace=178\n" * 40
        # Five seconds at 100 ticks a second: a thread pinning one core spends
        # about 500 jiffies of utime in that window, and the census is a delta.
        samples = iter([self.CENSUS.format(utime=0), self.CENSUS.format(utime=511)]
                       if census is None else census)
        # Before the playback starts the directory holds nothing new; the first
        # poll after it starts finds the session log.
        listings = iter(["", "session-fixture.log"])

        def adb(*command, **options):
            capture = options.get("capture", False)
            if command == ("shell", f"ls {files}/logs"):
                listing = next(listings, "session-fixture.log")
                return listing if capture else SimpleNamespace(returncode=0, stdout=listing, stderr="")
            if command == ("shell", f"cat {files}/logs/session-fixture.log"):
                return frozen
            if command == ("shell", "pidof", self.builder.app_id):
                return "10828" if capture else SimpleNamespace(returncode=0, stdout="10828", stderr="")
            if command[0] == "shell" and command[1].startswith("cat /proc/"):
                return next(samples)
            if command == ("shell", "debuggerd", "-b", "10828"):
                return SimpleNamespace(returncode=1, stdout="", stderr="debuggerd: root is required\n")
            if command[0] == "shell" and command[1] == "run-as":
                return SimpleNamespace(returncode=0, stdout="", stderr="")
            if command[:2] == ("logcat", "-d"):
                return SimpleNamespace(returncode=0, stdout="Fatal signal 6 ... SDLThread\nbacktrace:\n", stderr="")
            return SimpleNamespace(returncode=0, stdout="", stderr="")

        clock = SimpleNamespace(now=0.0)
        clock.sleep = lambda seconds: setattr(clock, "now", clock.now + seconds)
        clock.monotonic = lambda: clock.now
        with patch.object(self.builder, "adb", side_effect=adb), \
                patch.object(ab.time, "sleep", clock.sleep), \
                patch.object(ab.time, "monotonic", clock.monotonic):
            self.builder.device_training_run(self.run_dir, "check", None, retraces=600)

    def test_a_stalled_playback_reports_where_it_stopped(self):
        with self.assertRaisesRegex(ab.wb.BuildError, "stopped advancing at retrace 178"):
            self.stall()

    def test_a_stalled_playback_names_the_thread_that_is_spinning(self):
        with self.assertRaises(ab.wb.BuildError):
            self.stall()
        census = (self.run_dir / "threads.txt").read_text()
        rows = [line.split(None, 3) for line in census.splitlines() if line[:1].isdigit()]
        self.assertEqual(rows[0][3], "SDLThread", census)
        self.assertEqual(rows[0][2], "511", census)

    def test_a_stall_that_moves_on_is_watched_rather_than_killed(self):
        # The census takes five seconds, and a room load that was merely slow can
        # finish inside it: the evidence is kept, the process is left alone, and
        # the run goes on to complete.
        files = self.builder.DEVICE_FILES
        kills, state = [], {"log": 0, "done": False}

        def next_log():
            state["log"] += 1
            if state["log"] <= 6:
                return "retrace=178\n" * 20
            if state["log"] == 7:
                return "retrace=240\n"
            state["done"] = True
            return "retrace=240\n[android] host returned\noptimization profile written\n"

        listings = iter(["", "session-fixture.log"])

        def adb(*command, **options):
            capture = options.get("capture", False)
            if command == ("shell", f"ls {files}/logs"):
                listing = next(listings, "session-fixture.log")
                return listing if capture else SimpleNamespace(returncode=0, stdout=listing, stderr="")
            if command == ("shell", f"cat {files}/logs/session-fixture.log"):
                return next_log()
            if command == ("shell", "pidof", self.builder.app_id):
                pid = "" if state["done"] else "10828"
                return pid if capture else SimpleNamespace(returncode=0, stdout=pid, stderr="")
            if command[0] == "shell" and command[1].startswith("cat /proc/"):
                return self.CENSUS.format(utime=511)
            if command[0] == "shell" and command[1] == "run-as":
                kills.append(command[-1])
                return SimpleNamespace(returncode=0, stdout="", stderr="")
            if command == ("shell", f"ls {files}/pgo/check"):
                return "fixture.profraw" if capture else SimpleNamespace(returncode=0, stdout="", stderr="")
            if command[0] == "pull":
                Path(command[2]).write_bytes(b"synthetic profile")
            return SimpleNamespace(returncode=0, stdout="", stderr="")

        clock = SimpleNamespace(now=0.0)
        clock.sleep = lambda seconds: setattr(clock, "now", clock.now + seconds)
        clock.monotonic = lambda: clock.now
        with patch.object(self.builder, "adb", side_effect=adb), \
                patch.object(ab.time, "sleep", clock.sleep), \
                patch.object(ab.time, "monotonic", clock.monotonic):
            profiles = self.builder.device_training_run(self.run_dir, "check", None, retraces=600)
        self.assertEqual(len(profiles), 1)
        self.assertEqual(kills, [], "a playback that moved on must not be aborted")
        self.assertTrue((self.run_dir / "threads.txt").exists())

    def test_a_stalled_playback_aborts_the_busiest_thread_for_its_stack(self):
        with self.assertRaises(ab.wb.BuildError):
            self.stall()
        stacks = (self.run_dir / "stacks.txt").read_text()
        self.assertIn("SIGABRT to tid 11054 (SDLThread)", stacks)
        self.assertIn("backtrace:", stacks)
        self.assertTrue((self.run_dir / "session.log").read_text().count("retrace=178"))


if __name__ == "__main__":
    unittest.main()
