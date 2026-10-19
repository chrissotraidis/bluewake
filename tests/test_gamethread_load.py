from pathlib import Path
import time
import sys
import unittest

sys.path.insert(0, str(Path(__file__).parents[1] / "scripts"))
from gamethread_qualification.load import (
    interleave, pressure_argv, pressure_process, staged_matrix, validate_soak_plan,
)


class GameThreadLoadTests(unittest.TestCase):
    def test_matrix_covers_renderers_workers_topology_and_pressure(self):
        rows = staged_matrix()
        self.assertEqual({row["workers"] for row in rows if row["stage"] == "A"}, {0, 1, 2, 3})
        self.assertEqual({row["renderer"] for row in rows if row["stage"] in {"A", "B"}},
                         {"headless", "rendered"})
        self.assertEqual({row["placement"] for row in rows if row["stage"] == "C"},
                         {"separate-physical", "shared-smt"})
        self.assertEqual({row["pressure"] for row in rows if row["stage"] == "D"},
                         {"remote-core", "game-thread-sibling", "all-core"})

    def test_interleaving_is_balanced(self):
        self.assertEqual(interleave("serial", "candidate", 2),
                         ["serial", "candidate", "candidate", "serial"] * 2)

    def test_pressure_is_bounded_and_reaped_on_early_exit(self):
        start = time.monotonic()
        with pressure_process(10, 20) as process:
            self.assertIsNone(process.poll())
        self.assertIsNotNone(process.poll())
        self.assertLess(time.monotonic() - start, 3)

    def test_pressure_arguments_are_bounded(self):
        self.assertIn("--duty", pressure_argv(1, 50))
        for seconds, duty in ((0, 50), (1, 0), (1, 101)):
            with self.assertRaises(ValueError):
                pressure_argv(seconds, duty)

    def test_soak_requires_duration_stages_and_a_card_copy(self):
        plan = {"minutes": 30, "source_card": "source", "working_card": "copy",
                "stages": ["transition", "cutscene-music", "sailing", "pressure",
                           "save-reload"]}
        self.assertEqual(validate_soak_plan(plan), plan)
        for change in ({"minutes": 29}, {"working_card": "source"}, {"stages": []}):
            broken = {**plan, **change}
            with self.assertRaises(ValueError):
                validate_soak_plan(broken)


if __name__ == "__main__":
    unittest.main()
