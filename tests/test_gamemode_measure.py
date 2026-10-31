#!/usr/bin/env python3
"""Contract tests for scripts/gamemode_measure.py: parsing what the Deck reports, and the A/B report."""
import importlib.util
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).parents[1] / "scripts/gamemode_measure.py"
SPEC = importlib.util.spec_from_file_location("gamemode_measure", SCRIPT)
GM = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(GM)

PERF = """15003.21;msec;task-clock:u;15003210000;100.00;;
46651090000;;cycles:u;15003210000;100.00;;
85843000000;;instructions:u;15003210000;100.00;;
"""

# Lines as BlueWake writes them (RecompCore #49 builds), from a Lava Room session.
LOG = [
    "00:59:01.100 [interp-stats] budget=1 delivered=29 skipped=0 cut=0 rejected=0 idle=0 unstaged=1 drops=0 "
    "raises=0 render_worker=5/119ms gpu_staging=0/0ms gpu_behind=1/4ms late_drawable=7/206ms jobs=930 copied=720 "
    "pooled=720 blended=930",
    "00:59:02.100 [interp-stats] budget=1 delivered=30 skipped=0 cut=0 rejected=0 idle=0 unstaged=0 drops=0 "
    "raises=0 render_worker=0/0ms gpu_staging=0/0ms gpu_behind=0/0ms late_drawable=1/26ms jobs=931 copied=721 "
    "pooled=721 blended=931",
    "00:59:02.500 [fps-dip] retrace=9096 shown=51.2 game=26 speed=87% interpolated=26/26 draws/frame=930",
    "00:59:03.500 [fps-dip] retrace=9141 shown=55.0 game=28 speed=93% interpolated=28/28 draws/frame=930",
    "00:59:04.000 [gpu-prof] frames=240 frame_ms=22.942 max=25.212 busy_ms=22.892 copy_kb verts=9409 uniforms=2082 "
    "indices=260 storage=0 textures=0 | EFB 1 21.971 x1.0 | Staging copies 0.410 x4.0 | IB EFB 1 @0 1.972 x0.5",
]


class ParsingTest(unittest.TestCase):
    def test_perf_and_game_thread(self):
        values = GM.parse_perf(PERF)
        self.assertEqual(values["cycles"], 46651090000)
        thread = GM.game_thread(values, 15)
        self.assertAlmostEqual(thread["ghz"], 3.109, places=3)
        self.assertAlmostEqual(thread["ipc"], 1.84, places=2)
        self.assertAlmostEqual(thread["busy"], 1.0, places=2)

    def test_perf_not_counted(self):
        self.assertEqual(GM.game_thread(GM.parse_perf("<not counted>;;cycles:u;0;0;;\n"), 15), {})

    def test_clocks(self):
        self.assertEqual(GM.current_sclk("0: 200Mhz \n1: 1040Mhz *\n2: 1600Mhz \n"), 1040)
        self.assertIsNone(GM.current_sclk("0: 200Mhz \n"))
        self.assertEqual(GM.mean_cpu_mhz("cpu MHz\t\t: 3000.000\ncpu MHz\t\t: 3500.000\n"), 3250)

    def test_interp_stats_are_summed(self):
        stats = GM.parse_interp_stats(LOG)
        self.assertEqual(stats["counts"]["delivered"], 59)
        self.assertEqual(stats["counts"]["unstaged"], 1)
        self.assertEqual(stats["counts"]["blended"], 1861)
        self.assertNotIn("budget", stats["counts"])
        self.assertEqual(stats["overloads"]["late_drawable"], {"count": 8, "wait_ms": 232.0})
        self.assertEqual(stats["overloads"]["render_worker"], {"count": 5, "wait_ms": 119.0})

    def test_fps_dips(self):
        self.assertEqual(GM.parse_fps_dips(LOG), {"seconds": 2, "speed": 90.0, "game_fps": 27.0})
        self.assertEqual(GM.parse_fps_dips([]), {"seconds": 0, "speed": None, "game_fps": None})

    def test_gpu_prof(self):
        prof = GM.parse_gpu_prof(LOG[-1])
        self.assertEqual(prof["busy_ms"], 22.892)
        self.assertEqual(prof["zones"][0], {"name": "EFB 1", "ms": 21.971, "count": 1.0})
        self.assertEqual(prof["zones"][2]["name"], "IB EFB 1 @0")
        self.assertIsNone(GM.parse_gpu_prof("no report"))

    def test_settings(self):
        text = "# comment\nDOL_AURORA_RENDER_SCALE=3\nDOL_AURORA_EFFICIENT_EFFECTS=1\nOTHER=x\n"
        self.assertEqual(GM.read_settings(text, GM.DEFAULT_KEYS),
                         {"DOL_AURORA_RENDER_SCALE": "3", "DOL_AURORA_EFFICIENT_EFFECTS": "1"})


def window(label, ghz, watts, delivered, dips):
    return {"label": label, "window": 1, "seconds": 15, "game_thread": {"ghz": ghz, "ipc": 1.88},
            "power_w": watts, "gpu_mhz": 1000, "cpu_mhz": 3200,
            "interp": {"counts": {"delivered": delivered}}, "dips": {"seconds": dips}, "gpu_prof": None}


class ReportTest(unittest.TestCase):
    RECORDS = [window("off", 3.14, 14.8, 387, 15), window("off2", 3.12, 15.0, 386, 15),
               window("on", 3.45, 12.7, 435, 3), window("on", 3.44, 12.6, 450, 3)]

    def test_compare_means_change_and_spread(self):
        rows = {row["metric"]: row for row in GM.compare(self.RECORDS, ["off", "off2"], ["on"])}
        clock = rows["Game-thread clock"]
        self.assertAlmostEqual(clock["base"], 3.13)
        self.assertAlmostEqual(clock["test"], 3.445)
        self.assertAlmostEqual(clock["change"], 10.06, places=1)
        self.assertAlmostEqual(clock["spread"], 0.02)
        self.assertTrue(clock["beyond"])
        self.assertNotIn("GPU busy per profiled frame", rows)  # no window has it

    def test_change_within_the_spread_is_not_beyond(self):
        records = [window("a", 3.10, 14, 380, 15), window("a", 3.20, 14, 380, 15), window("b", 3.15, 14, 380, 15)]
        rows = {row["metric"]: row for row in GM.compare(records, ["a"], ["b"])}
        self.assertFalse(rows["Game-thread clock"]["beyond"])

    def test_report_prints_tables(self):
        with tempfile.TemporaryDirectory() as folder:
            (Path(folder) / "windows.jsonl").write_text("".join(json.dumps(r) + "\n" for r in self.RECORDS))
            result = subprocess.run([sys.executable, SCRIPT, "report", folder, "--base", "off,off2", "--test", "on"],
                                    capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("| Game-thread clock | 3.13 GHz | 3.45 GHz | +10.1% | 0.02 GHz | yes |", result.stdout)
        self.assertIn("| on | 1 |", result.stdout)

    def test_remote_without_bluewake_reports_an_error(self):
        result = subprocess.run([sys.executable, SCRIPT, "remote", "--windows", "1", "--seconds", "1"],
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 1)
        self.assertEqual(json.loads(result.stdout), {"error": "BlueWake is not running"})


if __name__ == "__main__":
    unittest.main()
