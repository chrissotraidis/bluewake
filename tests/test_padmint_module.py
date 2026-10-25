#!/usr/bin/env python3
"""PadMint routing and prepared-module packaging with synthetic inputs only."""
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]


def load(name, relative):
    spec = importlib.util.spec_from_file_location(name, ROOT / relative)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


entry = load("padmint_entry", "scripts/builder/padmint.py")


class ModuleTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix="BlueWake module with spaces ")
        self.addCleanup(self.tmp.cleanup)
        self.work = Path(self.tmp.name)
        self.args = SimpleNamespace(disc=self.work / "owned disc.iso", out=self.work, jobs=2,
                                    source_only=False, no_mods=False, app=self.work / "base app.ipa",
                                    ipa=self.work / "personal game.ipa", ios_toolchain="", train_pgo=True)

    def prepared(self, enabled=True):
        source = self.work / "composite-src"
        source.mkdir(exist_ok=True)
        (source / "synthetic.c").write_text("/* synthetic source */\n")
        digest = subprocess.check_output(
            [entry.sys.executable, str(ROOT / "scripts/ios/composite_manifest.py"), str(source)], text=True).split()[0]
        receipt = dict.fromkeys(entry.FEATURES, enabled)
        receipt.update(dict.fromkeys(entry.EXTRA_FEATURES, False), final_digest=digest)
        (self.work / "prepared-blocks.json").write_text(json.dumps(receipt))
        (self.work / "composite-final.digest").write_text(digest + "\n")
        return receipt

    def test_mod_options_work_with_isolated_python(self):
        # PadMint's embedded Windows Python omits the script directory from
        # sys.path. Exercise the real options emitter with synthetic inputs.
        source = self.work / "composite source"
        source.mkdir()
        (source / "generated_composite.h").write_text(
            "static const u32 s_dolrecomp_chunk_starts[] = {\n0x80000000u,\n};\n"
            "static const u32 s_ends[] = {\n0x80000004u,\n};\n")
        (source / "rel_data.inc").write_text("/* no synthetic REL data */\n")
        disc = self.work / "synthetic.dol"
        disc.write_bytes(bytes(256))
        options = self.work / "synthetic options.txt"
        options.write_text("option example 1 on Example option\n"
                           "write 0x80000000 example u8 42\n")
        result = subprocess.run([
            entry.sys.executable, "-I", str(ROOT / "scripts/mods/build_mod_variants.py"),
            "--composite-src", str(source), "--base-dol", str(disc), "--options", str(options),
            "--rels-bin-dir", str(self.work)], cwd=self.work, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        generated = (source / "mod_variants.inc").read_text()
        self.assertIn("#define MODULE_OPTION_COUNT 1u", generated)
        self.assertIn("0x80000000u, 1u, s_option_write_0", generated)
        self.assertIn("0x2Au", generated)

    def test_mac_modes_keep_exact_current_recipe_arguments(self):
        # Snapshot of main b1bf26a0: keep the actual player argv contract stable.
        target = {"command": ["/bin/bash", "{repo}/scripts/builder/build.sh", "{disc}",
                              "--out", "{work}", "--jobs", "{jobs}"],
                  "modes": {"full": ["--app", "{app}", "--train-pgo", "--ipa", "{output}"],
                            "source-only": ["--source-only"]},
                  "options": {"no-mods": ["--no-mods"]}}
        variables = {"repo": str(ROOT), "disc": str(self.args.disc), "work": str(self.work),
                     "jobs": "2", "app": str(self.args.app), "output": str(self.args.ipa)}
        for source_only in (False, True):
            for no_mods in (False, True):
                with self.subTest(source_only=source_only, no_mods=no_mods):
                    self.args.source_only, self.args.no_mods = source_only, no_mods
                    expected = target["command"] + target["modes"]["source-only" if source_only else "full"]
                    if no_mods:
                        expected += target["options"]["no-mods"]
                    with patch.object(entry.subprocess, "run") as run:
                        entry.build(self.args, "Darwin")
                    expected = [part.format(**variables) for part in expected]
                    # The Mac branch is also simulated by native Windows CI.
                    expected[1] = str(Path(expected[1]))
                    run.assert_called_once_with(expected, check=True)

    def test_source_only_stops_before_preparation_and_packaging(self):
        self.args.source_only = self.args.no_mods = True
        for host in ("Windows", "Linux"):
            with self.subTest(host=host), patch.object(entry.subprocess, "run") as run:
                entry.build(self.args, host)
                self.assertEqual(run.call_count, 1)
                self.assertIn("--source-only", run.call_args.args[0])
                self.assertIn("--no-mods", run.call_args.args[0])
                self.assertNotIn("--module-sources", run.call_args.args[0])
                self.assertFalse((self.work / "ios-module-result.json").exists())

    def test_completed_preparation_drives_flags_and_marker(self):
        for enabled, mode in ((True, "combined-v1"), (False, "none")):
            self.prepared(enabled)
            flags, actual = entry.prepared_options(self.work)
            self.assertEqual(actual, mode)
            self.assertIn("-DBLUEWAKE_FIXED_CPU=" + ("ON" if enabled else "OFF"), flags)
            module = self.work / "composite-ios" / entry.MODULE
            module.parent.mkdir(exist_ok=True)
            module.write_bytes(b"synthetic module")
            entry.write_result(self.work, module, actual)
            result = json.loads((self.work / "ios-module-result.json").read_text())
            marker = result["companions"][0]
            self.assertEqual((self.work / marker["file"]).read_bytes(), (mode + "\n").encode())
            self.assertEqual(marker["into"], "ModuleOptimizations")
            self.assertEqual(marker["sha256"], hashlib.sha256((mode + "\n").encode()).hexdigest())
            self.assertEqual(result["into"], "Frameworks/" + entry.MODULE)

    def test_changed_or_partial_preparation_is_rejected(self):
        receipt = self.prepared()
        (self.work / "composite-src/synthetic.c").write_text("changed\n")
        with self.assertRaisesRegex(ValueError, "source changed"):
            entry.prepared_options(self.work)
        receipt = self.prepared()
        receipt["direct_calls"] = False
        (self.work / "prepared-blocks.json").write_text(json.dumps(receipt))
        with self.assertRaisesRegex(ValueError, "complete combined-v1"):
            entry.prepared_options(self.work)
        del receipt["direct_calls"]
        (self.work / "prepared-blocks.json").write_text(json.dumps(receipt))
        with self.assertRaisesRegex(ValueError, "missing its module feature"):
            entry.prepared_options(self.work)

    def test_failed_preparation_cannot_reuse_old_result(self):
        toolchain = self.work / "toolchain.cmake"
        toolchain.touch()
        self.args.ios_toolchain = str(toolchain)
        result = self.work / "ios-module-result.json"
        result.write_text("old result")
        with patch.object(entry.subprocess, "run", side_effect=subprocess.CalledProcessError(1, "prepare")):
            with self.assertRaises(subprocess.CalledProcessError):
                entry.build(self.args, "Windows")
        self.assertFalse(result.exists())

    def test_missing_toolchain_fails_before_source_changes(self):
        with patch.object(entry.subprocess, "run") as run:
            with self.assertRaisesRegex(ValueError, "toolchain is missing"):
                entry.build(self.args, "Windows")
            run.assert_not_called()

    def test_full_route_uses_completed_features_and_writes_exact_result(self):
        self.prepared()
        toolchain = self.work / "toolchain with spaces.cmake"
        toolchain.touch()
        self.args.ios_toolchain = str(toolchain)
        self.args.no_mods = True
        calls = []

        def run(argv, **kwargs):
            calls.append(argv)
            if argv[:2] == ["cmake", "--fresh"]:
                folder = Path(argv[argv.index("-B") + 1])
                folder.mkdir(exist_ok=True)
                (folder / "CMakeCache.txt").write_text("synthetic cache")
            if argv[:2] == ["cmake", "--build"]:
                folder = Path(argv[2])
                folder.mkdir(exist_ok=True)
                (folder / entry.MODULE).write_bytes(b"synthetic module")

        options = entry.prepared_options(self.work)
        with patch.object(entry.subprocess, "run", side_effect=run), \
                patch.object(entry, "prepared_options", return_value=options):
            entry.build(self.args, "Windows")
        self.assertIn("--module-sources", calls[0])
        self.assertIn("--no-mods", calls[0])
        self.assertIn("--fresh", calls[1])
        self.assertIn("-DBLUEWAKE_FIXED_CPU=ON", calls[1])
        self.assertIn("-DBLUEWAKE_NATIVE_SET4=OFF", calls[1])
        self.assertIn("-DCMAKE_C_FLAGS=-mcpu=apple-a13", calls[1])
        self.assertFalse(any("profile" in arg or "march=" in arg for arg in calls[1]))
        result = json.loads((self.work / "ios-module-result.json").read_text())
        self.assertEqual(result["sha256"], hashlib.sha256(b"synthetic module").hexdigest())
        self.assertEqual(result["companions"][0]["into"], "ModuleOptimizations")

    def test_configure_receipt_reuses_only_known_cache_and_invalidates_failure(self):
        toolchain = self.work / "toolchain.cmake"
        toolchain.write_text("synthetic toolchain")
        folder = self.work / "build"
        folder.mkdir()
        cache = folder / "CMakeCache.txt"
        command = ["cmake", "-S", "fixture", "-B", str(folder)]

        def configured(*a, **k):
            cache.write_text("known cache")

        with patch.object(entry.subprocess, "run", side_effect=configured) as run:
            entry.configure(command, folder, toolchain)
            self.assertIn("--fresh", run.call_args.args[0])
            entry.configure(command, folder, toolchain)
            self.assertNotIn("--fresh", run.call_args.args[0])
            cache.write_text("borrowed flags")
            entry.configure(command, folder, toolchain)
            self.assertIn("--fresh", run.call_args.args[0])
            toolchain.write_text("changed toolchain")
            entry.configure(command, folder, toolchain)
            self.assertIn("--fresh", run.call_args.args[0])
        with patch.object(entry.subprocess, "run", side_effect=subprocess.CalledProcessError(1, "cmake")):
            with self.assertRaises(subprocess.CalledProcessError):
                entry.configure(command, folder, toolchain)
        self.assertFalse((folder / "padmint-configure.json").exists())

    def test_platform_builders_stop_after_mods_and_preparation(self):
        for family in ("windows", "linux"):
            builder = load(f"{family}_module_test", f"scripts/{family}/build.py")
            for source_only, no_mods in ((False, False), (False, True), (True, False)):
                with self.subTest(family=family, source_only=source_only, no_mods=no_mods):
                    folder = self.work / f"{family}-{source_only}-{no_mods}"
                    folder.mkdir()
                    iso = folder / "owned.iso"
                    iso.write_bytes(b"synthetic")
                    args = SimpleNamespace(out=folder, disc=iso, no_mods=no_mods, check_only=False,
                                           source_only=source_only, module_sources=not source_only)
                    b = builder.Builder(args)
                    b.iso = iso
                    calls = []
                    for name in ("check_tools", "dependencies", "disc", "build_dolrecomp", "configure_app",
                                 "translate", "generate", "build_mods", "prepare_blocks"):
                        setattr(b, name, lambda *a, _name=name: calls.append(_name))
                    b.extract = lambda *a: (folder / "game").mkdir()
                    with patch.object(builder, "sha256_file", return_value="fixture"):
                        b.build()
                    self.assertEqual("prepare_blocks" in calls, not source_only)
                    self.assertEqual("build_mods" in calls, not source_only and not no_mods)
                    if not source_only and not no_mods:
                        self.assertLess(calls.index("build_mods"), calls.index("prepare_blocks"))


if __name__ == "__main__":
    unittest.main()
