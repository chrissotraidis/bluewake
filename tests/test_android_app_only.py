#!/usr/bin/env python3
"""Synthetic Android --app-only checks; no disc, no NDK and no translated game source.

--app-only reuses OUT/composite/gGZLE01_recomp.so. Before any step runs it checks the module against the
record its compile wrote (OUT/composite/module.json) and refuses a request for other module options,
behaviour timing either way, leaving the source and the caches as they were. When the request matches it
skips the game-source steps, and the APK's record describes the module it reused."""
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

REPO = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("android_builder", REPO / "scripts/android/build.py")
ab = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ab)

DISC = "GZLE01.iso"  # never opened: an --app-only build does not read the disc
COMMIT = "0123456789abcdef0123456789abcdef01234567"
# Every step build() can run; an --app-only build may run only the first two, the app and the package.
STEPS = ("check_tools", "dependencies", "disc", "build_dolrecomp", "configure_app", "extract", "translate",
         "generate", "build_mods", "prepare_blocks", "compile_module", "train_on_device", "train_app_on_device",
         "build_app", "package")


def fake_git(*command):
    return "" if command[0] == "status" else COMMIT


def snapshot(root):
    """Every file under root, with the hash of its bytes and its modification time."""
    return {str(p.relative_to(root)): (hashlib.sha256(p.read_bytes()).hexdigest(), p.stat().st_mtime_ns)
            for p in sorted(root.rglob("*")) if p.is_file()}


class AppOnly(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.out = Path(self.tmp.name) / "out"
        # A generated stand-in for a finished build: the module, its source and the caches beside it.
        files = {
            "composite/gGZLE01_recomp.so": b"\x7fELF synthetic game module\n",
            "composite-src/chunks/chunk_0000.c": b"void synthetic(void) {}\n",
            "composite-src.digest": b"synthetic-composite-digest\n",
            "composite-final.digest": b"synthetic-final-digest\n",
            "composite-inputs.digest": b"synthetic-inputs-digest\n",
            "translated/dol/generated/chunks/chunk_0000.c": b"/* synthetic translation */\n",
            "game/main.dol": b"synthetic dol\n",
            "game.json": b"{}\n",
            "mods.done": b"complete\n",
            "mods/widescreen/composite-src/chunk_0000.c": b"/* synthetic mod */\n",
        }
        for name, data in files.items():
            path = self.out / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)

    def tearDown(self):
        self.tmp.cleanup()

    def args(self, *options):
        return ab.parse_args([DISC, "--out", str(self.out), "--jobs", "1", *options])

    def compiled(self, *options):
        """The module's record, as a full build with these options writes it after the compile."""
        builder = ab.AndroidBuilder(self.args(*options))
        builder.profile = None
        builder.clang_version = "synthetic clang"
        builder.iso = Path(DISC)
        with patch.object(ab.AndroidBuilder, "git", side_effect=fake_git):
            return builder.write_module_record(self.out / "composite/gGZLE01_recomp.so")

    def app_only(self, *options):
        """build() for an --app-only command, with every step replaced by a stand-in that only notes it ran."""
        self.calls, self.provenance = [], None
        builder = ab.AndroidBuilder(self.args("--app-only", *options))

        def stand_in(name):
            def run(_self, *args, **kwargs):
                self.calls.append(name)
                if name == "build_app":
                    return self.out / "app/libmain.so"
                if name == "package":
                    self.provenance = _self.apk_provenance()
                    return self.out / "WindWakerRecomp.apk"
                return None
            return run

        with patch.multiple(ab.AndroidBuilder, **{name: stand_in(name) for name in STEPS}), \
                patch.object(ab.AndroidBuilder, "git", side_effect=fake_git):
            builder.build()

    def assert_refused(self, message, *options):
        before = snapshot(self.out)
        with self.assertRaises(ab.wb.BuildError) as refused:
            self.app_only(*options)
        self.assertIn(message, str(refused.exception))
        self.assertEqual(self.calls, [], "a refused request must not run any step")
        self.assertEqual(snapshot(self.out), before, "a refused request must leave the source and caches as they were")

    def test_refuses_the_other_timing(self):
        for module, request in ((), ("--no-behaviour-timing",)), (("--no-behaviour-timing",), ()):
            with self.subTest(module=module, request=request):
                self.compiled(*module)
                self.assert_refused("behaviour_timing", *request)

    def test_reuses_a_matching_module_and_describes_it(self):
        for options, timing in ((), True), (("--no-behaviour-timing",), False):
            with self.subTest(options=options):
                record = self.compiled(*options)
                before = snapshot(self.out)
                self.app_only(*options)
                self.assertEqual(self.calls, ["check_tools", "dependencies", "build_app", "package"])
                self.assertEqual(snapshot(self.out), before, "the source and the caches must stay as they were")
                self.assertIs(self.provenance["behaviour_timing"], timing)
                self.assertTrue(self.provenance["module_reused"])
                self.assertEqual(self.provenance["module_sha256"], record["module_sha256"])
                self.assertEqual(self.provenance["module_options"], record["options"])
                self.assertEqual(self.provenance["iso"], DISC)

    def test_refuses_other_module_options(self):
        self.compiled()
        self.assert_refused("cpu", "--cpu", "oryon-1")
        self.assert_refused("mods", "--no-mods")
        self.assert_refused("lean_memory", "--no-lean-memory")

    def test_refuses_a_module_its_record_does_not_describe(self):
        self.compiled()
        (self.out / "composite/gGZLE01_recomp.so").write_bytes(b"\x7fELF another module\n")
        self.assert_refused("is not the module")

    def test_refuses_a_module_without_a_record(self):
        self.assert_refused("is missing")

    def test_refuses_no_tiered_for_a_tiered_module(self):
        self.compiled()
        path = self.out / "composite/module.json"
        record = json.loads(path.read_text(encoding="utf-8"))
        record["tiered"] = True  # as a trained module's compile records it
        path.write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
        self.assert_refused("tiered: the module has True, this build asks for False", "--no-tiered")
        # Without --no-tiered the tiered module is reused unchanged.
        self.app_only()
        self.assertEqual(self.calls, ["check_tools", "dependencies", "build_app", "package"])
        self.assertTrue(self.provenance["module_reused"])


if __name__ == "__main__":
    unittest.main()
