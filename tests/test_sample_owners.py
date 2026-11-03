#!/usr/bin/env python3
"""Synthetic sample-parser tests, without a game or private stack capture."""
import importlib.util
import contextlib
import io
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location(
    "sample_owners", Path(__file__).resolve().parents[1] / "scripts/sample_owners.py")
owners = importlib.util.module_from_spec(spec)
spec.loader.exec_module(owners)


class SampleOwnersTest(unittest.TestCase):
    def parse(self, label):
        capture = f"""Call graph:
    100 Thread_123{label}
    + 100 start  (in dyld) + 4
    +   50 main  (in BlueWake) + 8
    500 Thread_456: graphics worker
    + 500 worker  (in BlueWake) + 16
Total number in stack (recursive counted multiple, when >=5):
"""
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "synthetic.txt"
            path.write_text(capture)
            return owners.parse(path)

    def test_dispatch_queue_label(self):
        rows = self.parse("   DispatchQueue_1: com.apple.main-thread  (serial)")
        self.assertEqual([row[1] for row in rows], [100, 50])
        self.assertEqual([row[2] for row in rows], ["start", "main"])

    def test_main_thread_label(self):
        rows = self.parse(": Main Thread   DispatchQueue_<multiple>")
        self.assertEqual([row[1] for row in rows], [100, 50])
        self.assertEqual([row[2] for row in rows], ["start", "main"])

    def test_unidentified_thread_is_not_assumed_main(self):
        with self.assertRaises(SystemExit):
            self.parse(": other thread")

    def run_cli(self, depth):
        capture = """Call graph:
    100 Thread_123: Main Thread
    + 100 start  (in dyld) + 4
    + ! 80 main  (in BlueWake) + 8
    + ! : 70 deep  (in BlueWake) + 16
    + ! : 60 repeated  (in BlueWake) + 20
    + ! 20 repeated  (in BlueWake) + 24
Total number in stack (recursive counted multiple, when >=5):
"""
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "synthetic.txt"
            path.write_text(capture)
            output = io.StringIO()
            with patch.object(sys, "argv", ["sample_owners.py", str(path), "--depth", str(depth)]), contextlib.redirect_stdout(output):
                self.assertEqual(owners.main(), 0)
            return output.getvalue()

    def test_depth_excludes_deeper_frames_before_deduplication(self):
        output = self.run_cli(2)
        self.assertNotIn("deep", output)
        self.assertIn("20.00%  d2  repeated", output)
        self.assertNotIn("60.00%", output)

    def test_depth_can_include_deeper_frames(self):
        output = self.run_cli(3)
        self.assertIn("70.00%  d3  deep", output)
        self.assertIn("60.00%  d3  repeated", output)

    def test_zero_depth_has_no_owner_rows(self):
        output = self.run_cli(0)
        self.assertIn("main-thread samples: 100", output)
        self.assertNotIn("%  d", output)


if __name__ == "__main__":
    unittest.main()
