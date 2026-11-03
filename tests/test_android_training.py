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

    def playback(self, control=False, retraces=None, profile=True):
        finished = "retrace=100\n[android] host returned\noptimization profile written\n"
        if control:
            finished += "[player-milestone] control-admitted\n"
        # Include an ordinary progress poll before termination: this must not
        # change whether the run is a full training playback or a short probe.
        logs = iter(["retrace=100\n", finished, finished])
        listings = iter(["", "session-fixture.log"])
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


if __name__ == "__main__":
    unittest.main()
