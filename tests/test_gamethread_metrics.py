from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).parents[1] / "scripts"))
from gamethread_qualification.metrics import (
    job_histogram, join_wait_summary, linux_topology, percentile, timing_summary,
)


class GameThreadMetricsTests(unittest.TestCase):
    def test_percentiles_use_nearest_rank_and_explicit_units(self):
        values = list(range(1, 101))
        self.assertEqual(percentile(values, 50), 50)
        self.assertEqual(percentile(values, 95), 95)
        self.assertEqual(percentile(values, 99), 99)
        summary = timing_summary(values, hitch_ns=98)
        self.assertEqual(summary["unit"], "ns")
        self.assertEqual(summary["sample_count"], 100)
        self.assertEqual(summary["hitches"], 2)

    def test_missing_or_dropped_events_are_not_qualified_metrics(self):
        self.assertFalse(timing_summary([], hitch_ns=1)["available"])
        summary = timing_summary([10, 20], hitch_ns=15, dropped_events=1)
        self.assertFalse(summary["available"])
        self.assertIn("event loss", summary["reason"])

    def test_join_wait_grid_and_timeline_are_checked(self):
        summary = join_wait_summary([10, 30], [15, 31])
        self.assertEqual(summary["p50"], 1)
        self.assertEqual(summary["p99"], 5)
        with self.assertRaises(ValueError):
            join_wait_summary([10], [9])
        with self.assertRaises(ValueError):
            join_wait_summary([10], [11, 12])

    def test_histogram_is_stable(self):
        self.assertEqual(job_histogram([2, 1, 2, 8]), {"1": 1, "2": 2, "8": 1})

    def test_topology_groups_allowed_smt_siblings(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for cpu, core in ((0, 0), (1, 0), (2, 1)):
                topology = root / f"cpu{cpu}" / "topology"
                topology.mkdir(parents=True)
                (topology / "physical_package_id").write_text("0\n")
                (topology / "core_id").write_text(f"{core}\n")
            result = linux_topology(root, allowed={2, 0, 1})
        self.assertTrue(result["available"])
        self.assertEqual(result["allowed_cpus"], [0, 1, 2])
        self.assertEqual(result["physical_cores"], {"0:0": [0, 1], "0:1": [2]})
        self.assertTrue(result["smt_present"])

    def test_topology_capability_gap_is_explicit(self):
        with tempfile.TemporaryDirectory() as directory:
            result = linux_topology(Path(directory), allowed={7})
        self.assertFalse(result["available"])
        self.assertIn("CPU 7", result["reason"])


if __name__ == "__main__":
    unittest.main()
