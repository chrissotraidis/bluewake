"""Exercise selection changes and failed preparation using synthetic source."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts/builder"))
import module_optimizations as optimizations


class MacOptimizationCacheTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.out = Path(self.temp.name)

    def generate(self, mode):
        script = r'''
set -euo pipefail
root=$1; out=$2; module_optimizations=$3; mods=0; accept_new=0; logs=$out/logs
mkdir -p "$logs"
. "$root/scripts/builder/profiles/bluewake.sh"
mkdir -p "$out/fixture"
printf 'synthetic unprepared source\n' > "$out/fixture/generated.h"
COMPOSITE_DIGEST=$(python3 "$root/scripts/ios/composite_manifest.py" "$out/fixture" | awk '{print $1}')
die() { echo "$*" >&2; exit 1; }
run() { cp -R "$out/fixture" "$out/composite-src.new"; : > "$logs/composite-generate.log"; }
profile_generate
'''
        return subprocess.run(["bash", "-c", script, "cache-test", str(ROOT), str(self.out), mode],
                              capture_output=True, text=True, check=True)

    def test_same_mode_reuses_but_switching_preserves_and_regenerates_source(self):
        self.generate("none")
        source = self.out / "composite-src/generated.h"
        original_time = source.stat().st_mtime_ns
        self.generate("none")
        self.assertEqual(source.stat().st_mtime_ns, original_time)
        self.generate("combined-v1")
        self.assertEqual(len(list(self.out.glob("composite-previous.*/source/generated.h"))), 1)
        source.write_text("synthetic prepared source\n")
        (self.out / "composite-final.digest").write_text(optimizations.tree_digest(source.parent) + "\n")
        self.generate("none")
        self.assertEqual(source.read_text(), "synthetic unprepared source\n")
        self.assertTrue(any(p.read_text() == "synthetic prepared source\n"
                            for p in self.out.glob("composite-previous.*/source/generated.h")))
        self.assertTrue(all(flag.endswith("=OFF") for flag in optimizations.cmake_flags("none")))

    def test_partial_preparation_is_not_reused_on_resume(self):
        self.generate("combined-v1")
        source = self.out / "composite-src/generated.h"
        original_digest = (self.out / "composite-final.digest").read_text()
        def fail(*args, **kwargs):
            source.write_text("synthetic interrupted preparation\n")
            raise subprocess.CalledProcessError(1, args[0])
        with patch.object(optimizations, "STEPS", (("scripts/windows/fast_blocks.py",),)), \
             patch.object(optimizations.subprocess, "run", side_effect=fail):
            with self.assertRaises(subprocess.CalledProcessError):
                optimizations.prepare(self.out, "combined-v1")
        self.assertEqual((self.out / "composite-final.digest").read_text(), original_digest)
        self.assertFalse((self.out / "module-optimizations.json").exists())
        self.generate("combined-v1")
        self.assertEqual(source.read_text(), "synthetic unprepared source\n")
        self.assertTrue(any(p.read_text() == "synthetic interrupted preparation\n"
                            for p in self.out.glob("composite-previous.*/source/generated.h")))


if __name__ == "__main__":
    unittest.main()
