#!/usr/bin/env python3
"""Focused regression tests for completed-task evidence validation."""
from __future__ import annotations

import importlib.util
from pathlib import Path
import tempfile
import unittest


MODULE_PATH = Path(__file__).with_name("validate_plan.py")
SPEC = importlib.util.spec_from_file_location("validate_plan", MODULE_PATH)
assert SPEC and SPEC.loader
VALIDATOR = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(VALIDATOR)


PRD = """# PRD — Evidence fixture

## Requirements
### FR-01 — Preserve evidence
- Completed work has concrete evidence.
"""

TECH_PLAN = """# TECH_PLAN — Evidence fixture

FR-01

## Impact register
| ID | Requirement | Entity / flow | Baseline | Expected impact / target | Measurement | Risk | Owner |
|---|---|---|---|---|---|---|---|
| IMP-01 | FR-01 | Validator | unknown | Reject empty evidence | Regression test | Low | M0 |

## Implementation order
| Milestone | Depends | Can run with | Deliverable / integration gate |
|---|---|---|---|
| M0 | — | — | Evidence validation |
"""


def progress(task: str, evidence_row: str = "") -> str:
    return f"""# PROGRESS — Evidence fixture

FR-01

## Milestones
### M0: Evidence validation
{task}
- [ ] M0-T02: Keep open task valid (FR-01)
- [ ] M0-T03: Keep another open task valid (FR-01)

## Impact evidence
| ID | Actual | Outcome | Evidence |
|---|---|---|---|
| IMP-01 | not measured | untested | none |

## Evidence ledger
| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
{evidence_row}
"""


class EvidenceValidationTest(unittest.TestCase):
    def validate(self, progress_text: str) -> list[str]:
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            (directory / "PRD.md").write_text(PRD, encoding="utf-8")
            (directory / "TECH_PLAN.md").write_text(TECH_PLAN, encoding="utf-8")
            (directory / "PROGRESS.md").write_text(progress_text, encoding="utf-8")
            errors, _ = VALIDATOR.validate(directory)
            return errors

    def test_completed_task_without_evidence_fails(self) -> None:
        errors = self.validate(progress("- [x] M0-T01: Complete task (FR-01)"))
        self.assertIn("M0-T01: completed without evidence", "\n".join(errors))

    def test_completed_task_with_empty_row_fails(self) -> None:
        errors = self.validate(progress(
            "- [x] M0-T01: Complete task (FR-01)", "| M0-T01 | | | | |"))
        diagnostic = "\n".join(errors)
        self.assertIn("M0-T01: evidence row missing", diagnostic)
        self.assertIn("commit / artifact", diagnostic)
        self.assertIn("command / method", diagnostic)
        self.assertIn("result", diagnostic)

    def test_completed_task_with_partial_row_fails(self) -> None:
        errors = self.validate(progress(
            "- [x] M0-T01: Complete task (FR-01)",
            "| M0-T01 | commit abc123 | | local | |"))
        self.assertIn(
            "M0-T01: evidence row missing command / method, result", errors)

    def test_completed_task_with_valid_row_passes(self) -> None:
        errors = self.validate(progress(
            "- [x] M0-T01: Complete task (FR-01)",
            "| M0-T01 | commit abc123 | python3 test.py | local | passed |"))
        self.assertEqual([], errors)

    def test_open_task_without_evidence_passes(self) -> None:
        errors = self.validate(progress("- [ ] M0-T01: Open task (FR-01)"))
        self.assertEqual([], errors)

    def test_empty_inline_evidence_fails(self) -> None:
        errors = self.validate(progress(
            "- [x] M0-T01: Complete task (FR-01); evidence:"))
        self.assertIn("M0-T01: inline evidence missing", "\n".join(errors))

    def test_partial_inline_evidence_fails(self) -> None:
        errors = self.validate(progress(
            "- [x] M0-T01: Complete task (FR-01); "
            "evidence: artifact=commit abc123; result=passed"))
        self.assertIn("M0-T01: inline evidence missing command / method", errors)

    def test_valid_inline_evidence_passes(self) -> None:
        errors = self.validate(progress(
            "- [x] M0-T01: Complete task (FR-01); "
            "evidence: artifact=commit abc123; method=python3 test.py; result=passed"))
        self.assertEqual([], errors)


if __name__ == "__main__":
    unittest.main()
