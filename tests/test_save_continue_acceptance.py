"""Source-only contracts for the portable save acceptance entry point."""

from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).parents[1]
SCRIPT = ROOT / "scripts" / "save_continue_acceptance.sh"


class SaveContinueAcceptanceTests(unittest.TestCase):
    def test_missing_inputs_are_incomplete_and_source_is_untouched(self):
        with tempfile.TemporaryDirectory() as directory:
            base = Path(directory)
            card = base / "source card.raw"
            card.write_bytes(b"private fixture placeholder")
            before = card.read_bytes()
            result = subprocess.run(
                ["bash", str(SCRIPT), str(base / "missing host"),
                 str(base / "missing module"), str(card)],
                cwd=ROOT, text=True, stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT, check=False,
            )
            self.assertEqual(result.returncode, 3)
            self.assertIn("missing required input", result.stdout)
            self.assertEqual(card.read_bytes(), before)

    def test_entry_point_keeps_legacy_variables_and_bounded_predicates(self):
        source = SCRIPT.read_text(encoding="utf-8")
        self.assertIn("BLUEWAKE_ACCEPTANCE_HOST", source)
        self.assertIn("BLUEWAKE_ACCEPTANCE_ROUTE_CARD", source)
        self.assertIn("BLUEWAKE_ACCEPTANCE_OPENING_BOUND", source)
        self.assertIn("BLUEWAKE_ACCEPTANCE_PLAY_BOUND", source)
        self.assertIn("BLUEWAKE_ACCEPTANCE_WORKERS", source)
        self.assertNotIn("opening-complete retrace=13850'", source)
        self.assertNotIn("play-scene retrace=13910'", source)
        self.assertIn("source card changed during acceptance", source)


if __name__ == "__main__":
    unittest.main()
