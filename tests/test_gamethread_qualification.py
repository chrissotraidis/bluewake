import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

from gamethread_qualification.process import run_process
from gamethread_qualification.report import (
    add_outcome, new_report, read_report, validate_report, write_report)


class ReportTests(unittest.TestCase):
    def test_status_precedence_and_atomic_round_trip(self):
        with tempfile.TemporaryDirectory(prefix="bluewake report ") as temporary:
            path = Path(temporary) / "result.json"
            report = new_report({"commit": "abc"}, {"workers": 2})
            add_outcome(report, "source", "pass")
            self.assertEqual(report["status"], "pass")
            add_outcome(report, "hardware", "incomplete", "not available")
            self.assertEqual(report["status"], "incomplete")
            add_outcome(report, "correctness", "failure", "mismatch")
            self.assertEqual(report["status"], "failure")
            write_report(path, report)
            self.assertEqual(read_report(path), report)
            self.assertFalse(list(path.parent.glob("*.tmp.*")))

    def test_rejects_malformed_evidence(self):
        for report in ({}, {"schema_version": 2, "outcomes": []},
                       {**new_report(), "outcomes": [{"status": "maybe"}]},
                       {**new_report(), "status": "pass"}):
            with self.assertRaises(ValueError):
                validate_report(report)


class ProcessTests(unittest.TestCase):
    def test_success_failure_and_path_with_spaces(self):
        with tempfile.TemporaryDirectory(prefix="bluewake runs ") as temporary:
            result = run_process(
                [sys.executable, "-c", "print('complete')"], temporary,
                "success case", 5)
            self.assertEqual(result.returncode, 0)
            self.assertFalse(result.timed_out)
            self.assertEqual(result.log_path.read_text().strip(), "complete")
            failed = run_process(
                [sys.executable, "-c", "raise SystemExit(7)"], temporary,
                "failure", 5)
            self.assertEqual(failed.returncode, 7)

    @unittest.skipUnless(os.name == "posix", "process-group assertion is POSIX-specific")
    def test_timeout_reaps_child_process_group(self):
        with tempfile.TemporaryDirectory() as temporary:
            marker = Path(temporary) / "child-finished"
            code = (
                "import subprocess,sys,time;"
                "subprocess.Popen([sys.executable,'-c',"
                f"\"import time,pathlib;time.sleep(2);pathlib.Path({str(marker)!r}).write_text('bad')\"]);"
                "time.sleep(30)")
            result = run_process([sys.executable, "-c", code], temporary,
                                 "timeout", 0.1, grace_seconds=0.1)
            self.assertTrue(result.timed_out)
            subprocess.run([sys.executable, "-c", "import time;time.sleep(.3)"], check=True)
            self.assertFalse(marker.exists())


if __name__ == "__main__":
    unittest.main()
