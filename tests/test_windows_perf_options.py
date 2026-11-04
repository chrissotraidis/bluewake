#!/usr/bin/env python3
"""The Windows builder's #552 and #553 options, on synthetic inputs only.

fp_checks (#553) is prepared by default and --no-fp-checks turns it off;
--direct-gate (#552) only opens the gate for training and is recorded apart
from the composite-profile key the build reuses. Runs on Linux CI: nothing is
built, Builder is replaced by a stub that keeps the parsed options."""
import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

REPO = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("windows_builder", REPO / "scripts/windows/build.py")
bw = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bw)
SOURCE = (REPO / "scripts/windows/build.py").read_text()


class Parsed:
    def __init__(self):
        self.args = None

    def __call__(self, args):
        self.args = args
        return self

    def build(self):
        pass


def parse(*options):
    with tempfile.TemporaryDirectory(prefix="windows options ") as out:
        parsed = Parsed()
        argv = ["build.py", "--out", out, *options, str(Path(out) / "owned-disc.iso")]
        with patch.object(bw, "Builder", parsed), patch.object(sys, "argv", argv):
            bw.main()
        return parsed.args


class WindowsPerfOptionsTest(unittest.TestCase):
    def test_fp_checks_is_a_default(self):
        self.assertIn("fp_checks", bw.WINDOWS_DEFAULT_OPTIMIZATIONS)
        self.assertTrue(parse().fp_checks)
        self.assertFalse(parse("--no-fp-checks").fp_checks)

    def test_direct_gate_is_opt_in(self):
        self.assertNotIn("direct_gate", bw.WINDOWS_DEFAULT_OPTIMIZATIONS)
        self.assertFalse(parse().direct_gate)
        self.assertTrue(parse("--direct-gate").direct_gate)

    def test_conservative_drops_fp_checks(self):
        self.assertFalse(parse("--conservative").fp_checks)

    def test_fp_checks_runs_after_direct_calls_and_before_lean_memory(self):
        order = [SOURCE.index(f'self.run("{name}"') for name in ("direct-calls", "fp-checks", "lean-memory")]
        self.assertEqual(order, sorted(order))

    def test_training_opens_the_gate_only_with_the_option(self):
        self.assertIn('env["BLUEWAKE_DIRECT_GATE"] = "1" if getattr(self.args, "direct_gate", False) else "0"',
                      SOURCE)

    def test_provenance_records_both(self):
        self.assertIn('"direct_gate_training": getattr(self.args, "direct_gate", False)', SOURCE)
        self.assertIn('"fp_checks": getattr(self.args, "fp_checks", False)', SOURCE)


if __name__ == "__main__":
    unittest.main()
