#!/usr/bin/env python3
"""The Android builder's #552 and #553 options, on synthetic inputs only.

The Android builder is built on scripts/windows/build.py: fp_checks (#553)
comes with the Windows builder's defaults and --no-fp-checks turns it off;
--direct-gate (#552) opens the gate only for the module's training on the
device, and is part of the module's record, so a module trained the other way
is not reused. Nothing is built and no device is needed."""
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

REPO = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("android_builder", REPO / "scripts/android/build.py")
ab = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ab)


class Pushed(Exception):
    pass


class AndroidPerfOptionsTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)

    def parse(self, *options):
        return ab.parse_args(["GZLE01.iso", "--out", str(self.root / "out"), "--jobs", "1", *options])

    def test_fp_checks_is_a_default(self):
        self.assertIn("fp_checks", ab.wb.WINDOWS_DEFAULT_OPTIMIZATIONS)
        self.assertTrue(self.parse().fp_checks)
        self.assertFalse(self.parse("--no-fp-checks").fp_checks)
        self.assertFalse(self.parse("--conservative").fp_checks)

    def test_direct_gate_is_opt_in(self):
        self.assertFalse(self.parse().direct_gate)
        self.assertTrue(self.parse("--direct-gate").direct_gate)

    def test_direct_gate_requires_direct_calls(self):
        with self.assertRaises(SystemExit):
            self.parse("--conservative", "--direct-gate")

    def test_module_request_records_both(self):
        plain = ab.module_request(self.parse())
        gated = ab.module_request(self.parse("--direct-gate"))
        no_fp = ab.module_request(self.parse("--no-fp-checks"))
        self.assertTrue(plain["fp_checks"])
        self.assertFalse(plain["direct_gate"])
        self.assertTrue(gated["direct_gate"])
        self.assertFalse(no_fp["fp_checks"])
        self.assertNotEqual(plain, gated)
        self.assertNotEqual(plain, no_fp)

    def launch_env(self, *options, rendered=False):
        builder = ab.AndroidBuilder(self.parse(*options))
        run = Path(tempfile.mkdtemp(dir=self.root)) / "run"  # the builder makes it

        def adb(*command, check=True, capture=False, timeout=None):
            if command and command[0] == "push":
                raise Pushed
            return ""

        with patch.object(builder, "adb", adb), self.assertRaises(Pushed):
            builder.device_training_run(run, "plain", None, rendered=rendered)
        return (run / "launch.env").read_text()

    def test_training_opens_the_gate_only_with_direct_gate(self):
        self.assertNotIn("BLUEWAKE_DIRECT_GATE", self.launch_env())
        self.assertIn("BLUEWAKE_DIRECT_GATE=1\n", self.launch_env("--direct-gate"))
        # The app's own (rendered) training plays with the player's settings.
        self.assertNotIn("BLUEWAKE_DIRECT_GATE", self.launch_env("--direct-gate", rendered=True))

    def fingerprint(self, *options):
        builder = ab.AndroidBuilder(self.parse(*options))
        builder.clang_version = "clang fixture"
        builder.mods = []
        with patch.object(builder, "git", lambda *a: "fixture"), \
                patch.object(ab.wb, "tree_digest", lambda path: "fixture"):
            return builder.training_fingerprint()

    def test_training_fingerprint_follows_the_gate(self):
        self.assertNotEqual(self.fingerprint(), self.fingerprint("--direct-gate"))
        self.assertNotEqual(self.fingerprint(), self.fingerprint("--no-fp-checks"))


if __name__ == "__main__":
    unittest.main()
