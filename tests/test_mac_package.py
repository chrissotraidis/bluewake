"""Packaging safety with synthetic inputs; signing is exercised on actual builds."""
import argparse
import importlib.util
import json
from pathlib import Path
import plistlib
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

BUILDER = Path(__file__).resolve().parents[1] / "scripts/builder"
sys.path.insert(0, str(BUILDER))
SPEC = importlib.util.spec_from_file_location("package_macos", BUILDER / "package_macos.py")
PACKAGE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(PACKAGE)


class MacPackageTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.args = argparse.Namespace(app=self.root / "input.app", output=self.root / "output/BlueWake.app",
            runtime=self.root / "runtime", module=None, game=None, disc=None, identity="-",
            source_commit="synthetic-source", source_modified=False, module_optimizations="none")
        contents = self.args.app / "Contents"
        (contents / "MacOS").mkdir(parents=True)
        (contents / "Resources").mkdir()
        (contents / "MacOS/BlueWake").write_bytes(b"synthetic host")
        (contents / "Info.plist").write_bytes(plistlib.dumps({"CFBundleIdentifier": "com.bluewake.host"}))
        dsp = self.args.runtime / "Data/Sys/GC"
        dsp.mkdir(parents=True)
        for name in ("dsp_rom.bin", "dsp_coef.bin"):
            (dsp / name).write_bytes(b"synthetic runtime resource")
        self.args.output.mkdir(parents=True)
        (self.args.output / "old").write_text("previous app")
        self.player_save = self.root / "player.card"
        self.player_save.write_bytes(b"existing player save")
        self.addCleanup(patch.stopall)
        self.read = patch.object(PACKAGE.subprocess, "check_output", side_effect=self.tool_output).start()
        self.run = patch.object(PACKAGE.subprocess, "run").start()

    def tool_output(self, command, **kwargs):
        if command[0] == "otool":
            return "host:\n\t/usr/lib/libSystem.B.dylib (compatibility version 1.0.0)\n"
        return "synthetic-revision\n"

    def personal(self):
        self.args.module = self.root / "module.dylib"
        self.args.module.write_bytes(b"synthetic module")
        self.args.disc = self.root / "disc.iso"
        self.args.disc.write_bytes(b"synthetic disc")
        self.args.game = self.root / "game"
        (self.args.game / "rels").mkdir(parents=True)
        (self.args.game / "main.dol").write_bytes(b"synthetic dol")
        for index in range(415):
            (self.args.game / "rels" / f"{index}.rel").write_bytes(b"synthetic rel")

    def test_personal_app_copies_inputs_and_preserves_previous_output_and_save(self):
        self.personal()
        result = PACKAGE.assemble(self.args)
        self.assertTrue(result["containsTranslatedGameCode"])
        resources = self.args.output / "Contents/Resources"
        self.assertEqual((resources / "Game/GZLE01.iso").read_bytes(), self.args.disc.read_bytes())
        self.assertEqual(result["module_sha256"], PACKAGE.sha(self.args.module))
        self.assertEqual(len(list((resources / "Game/rels").glob("*.rel"))), 415)
        self.assertEqual(len(list(self.args.output.parent.glob("mac-previous-*/BlueWake.app/old"))), 1)
        self.assertEqual(self.player_save.read_bytes(), b"existing player save")

    def test_failed_signing_keeps_previous_app(self):
        self.personal()
        self.run.side_effect = subprocess.CalledProcessError(1, "codesign")
        with self.assertRaises(subprocess.CalledProcessError):
            PACKAGE.assemble(self.args)
        self.assertEqual((self.args.output / "old").read_text(), "previous app")
        self.assertEqual(self.args.module.read_bytes(), b"synthetic module")

    def test_app_only_contains_no_personal_inputs(self):
        result = PACKAGE.assemble(self.args)
        self.assertFalse(result["containsTranslatedGameCode"])
        self.assertFalse((self.args.output / "Contents/Resources/Game").exists())
        self.assertFalse((self.args.output / "Contents/Frameworks/gGZLE01_recomp.dylib").exists())
        self.assertEqual(json.loads((self.args.output / "Contents/Resources/BuilderProvenance.json").read_text()), result)

    def test_external_library_is_rejected_before_output_changes(self):
        self.read.side_effect = lambda *a, **k: "host:\n\t/opt/homebrew/lib/developer.dylib (compatibility version 1.0.0)\n"
        with self.assertRaisesRegex(ValueError, "not self-contained"):
            PACKAGE.assemble(self.args)
        self.assertEqual((self.args.output / "old").read_text(), "previous app")
        self.run.assert_not_called()

    def test_combined_candidate_requires_module_and_records_selection(self):
        self.args.module_optimizations = "combined-v1"
        with self.assertRaisesRegex(ValueError, "require a personal module"):
            PACKAGE.assemble(self.args)
        self.assertEqual((self.args.output / "old").read_text(), "previous app")
        self.personal()
        result = PACKAGE.assemble(self.args)
        self.assertEqual(result["module_optimizations"], "combined-v1")
        self.assertEqual((self.args.output / "Contents/Resources/ModuleOptimizations").read_text(), "combined-v1\n")


if __name__ == "__main__":
    unittest.main()
