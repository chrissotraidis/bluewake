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

class MacPerfOptionsTest(unittest.TestCase):
    """#552 and #553 on the Apple builder: fp_checks.py where Linux runs it, the
    gate only in training's environment, and the builder's option checks."""

    def test_fp_checks_runs_after_direct_calls_and_before_lean_memory(self):
        names = [step[0] for step in optimizations.steps(True, True, True, True, fp_checks=True)]
        at = names.index("scripts/windows/fp_checks.py")
        self.assertEqual(names[at - 1], "scripts/windows/direct_calls.py")
        self.assertEqual(names[at + 1], "scripts/windows/lean_memory.py")
        self.assertNotIn("scripts/windows/fp_checks.py",
                         [step[0] for step in optimizations.steps(True, True, True, True)])

    def test_fp_checks_changes_the_recipe(self):
        self.assertNotEqual(optimizations.fingerprint("combined-v1", True),
                            optimizations.fingerprint("combined-v1", True, fp_checks=True))
        self.assertEqual(optimizations.fingerprint("none", fp_checks=True), "none")

    def test_training_opens_the_gate_only_when_asked(self):
        text = (ROOT / "scripts/builder/train_local_pgo.py").read_text()
        self.assertIn('environment["BLUEWAKE_DIRECT_GATE"] = "1" if args.direct_gate else "0"', text)
        self.assertIn('digest.update(b"\\0direct-gate")', text)
        self.assertIn("args+=(--direct-gate)", (ROOT / "scripts/builder/profiles/bluewake.sh").read_text())

    def builder(self, *args):
        with tempfile.TemporaryDirectory() as bin_dir:
            stub = Path(bin_dir) / "sysctl"
            stub.write_text("#!/bin/sh\necho 8\n")
            stub.chmod(0o755)
            env = {"PATH": f"{bin_dir}:/usr/bin:/bin"}
            return subprocess.run(["bash", str(ROOT / "scripts/builder/build.sh"), *args],
                                  capture_output=True, text=True, env=env)

    def test_builder_option_checks(self):
        for args, message in ((("--fp-checks", "--platform", "macos", "x.iso"), "--fp-checks needs --lean-blocks"),
                              (("--conservative", "--fp-checks", "x.iso"), "--conservative builds without")):
            result = self.builder(*args)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn(message, result.stderr)
        self.assertIn("--no-fp-checks", self.builder("--help").stdout)
        self.assertIn("--direct-gate", self.builder("--help").stdout)


if __name__ == "__main__":
    unittest.main()
