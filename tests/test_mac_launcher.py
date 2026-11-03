"""Synthetic cards and a fake host only; never launch a game or read a disc."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

LAUNCHER = Path(__file__).resolve().parents[1] / "scripts/mac/run_host.sh"
MOCK_GUARD = r"""
pgrep() { [[ "${BW_LAUNCHER_TEST_BUSY:-0}" = 1 && "$1" = -x && "$2" = Dolphin ]]; }
export -f pgrep
exec bash "$@"
"""


class MacLauncherTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.output = self.root / "output with spaces"
        self.host = self.root / "fake-host.sh"
        self.host.write_text("""#!/bin/bash
if [ ! -e "$BLUEWAKE_CARD_PATH" ]; then
    printf 'synthetic-new-card' > "$BLUEWAKE_CARD_PATH"
fi
printf 'CARD=%s\n' "$BLUEWAKE_CARD_PATH"
""")
        self.host.chmod(0o700)

    def card(self):
        self.output.mkdir(exist_ok=True)
        card = self.output / "test.card"
        card.write_bytes(b"synthetic-old-progress")
        return card

    def launch(self, route="title", output=None, **extra):
        environment = dict(os.environ)
        for name in ("CARD", "COMPOSITE", "EXTRA_ENV", "BWW", "WALK", "OPTIONS"):
            environment.pop(name, None)
        environment.update(extra)
        return subprocess.run(
            ["bash", "-c", MOCK_GUARD, "launcher-test", str(LAUNCHER),
             str(output or self.output), "0", "10", route, "", "", str(self.host)],
            cwd=self.root, env=environment, capture_output=True, text=True, timeout=10)

    def backups(self):
        return list(self.output.glob("card-previous.*/test.card"))

    def test_fresh_run_preserves_previous_card(self):
        self.card()
        result = self.launch()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual((self.output / "test.card").read_bytes(), b"synthetic-new-card")
        self.assertEqual(len(self.backups()), 1)
        self.assertEqual(self.backups()[0].read_bytes(), b"synthetic-old-progress")
        self.assertIn("Previous test card preserved", result.stderr)

    def test_load_reuses_existing_card(self):
        card = self.card()
        result = self.launch("load")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(card.read_bytes(), b"synthetic-old-progress")
        self.assertEqual(self.backups(), [])

    def test_explicit_load_preserves_replaced_card(self):
        self.card()
        source = self.root / "source.card"
        source.write_bytes(b"synthetic-source-progress")
        result = self.launch("load", CARD=str(source))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual((self.output / "test.card").read_bytes(), source.read_bytes())
        self.assertEqual(self.backups()[0].read_bytes(), b"synthetic-old-progress")

    def test_explicit_same_card_is_not_moved(self):
        card = self.card()
        result = self.launch("load", CARD=str(card))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(card.read_bytes(), b"synthetic-old-progress")
        self.assertEqual(self.backups(), [])

    def test_missing_source_stops_before_replacing_existing_card(self):
        card = self.card()
        result = self.launch("load", CARD=str(self.root / "missing.card"))
        self.assertEqual(result.returncode, 2)
        self.assertEqual(card.read_bytes(), b"synthetic-old-progress")
        self.assertEqual(self.backups(), [])
        self.assertFalse((self.output / "run.log").exists())

    def test_relative_output_is_resolved_before_changing_directory(self):
        result = self.launch(output=self.output.name)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("CARD=" + str(self.output.resolve()), (self.output / "run.log").read_text())

    def test_new_game_routes_reach_fake_host(self):
        for route in ("outset", "save"):
            with self.subTest(route=route):
                result = self.launch(route)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertTrue((self.output / "test.card").exists())

    def test_busy_game_stops_before_creating_output(self):
        result = self.launch(BW_LAUNCHER_TEST_BUSY="1")
        self.assertEqual(result.returncode, 1)
        self.assertIn("one_game_guard", result.stderr)
        self.assertFalse(self.output.exists())


if __name__ == "__main__":
    unittest.main()
