#!/usr/bin/env python3
"""Windows player-training contract checks using public, synthetic inputs only."""
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

REPO = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("windows_builder", REPO / "scripts/windows/build.py")
bw = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bw)
OPTIONS = ("prepared_blocks", "fixed_cpu", "fixed_mem1", "inline_fp", "gather_pipe", "direct_calls",
           "inline_gpr", "native_j3d", "native_vec", "native_math", "native_skin", "native_game_math",
           "lean_memory", "native_entries")


class TrainingTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix="windows training ")
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        args = SimpleNamespace(out=self.root, march="x86-64-v3", jobs=2, opt_level="2", retrain=False,
                               console=False, **dict.fromkeys(OPTIONS, False))
        self.b = bw.Builder(args)
        self.b.env = dict(os.environ, BLUEWAKE_CARD_PATH="player.card", BLUEWAKE_MODS="user-mod",
                          DOL_AURORA_FRAME_INTERP="1", LLVM_PROFILE_FILE="player.profraw")
        self.b.mods = True
        self.b.clang_version = "clang version fixture"
        self.b.llvm_profdata = "llvm-profdata"
        self.b.iso = self.root / "owned-disc.iso"
        self.b.logs.mkdir()

    def test_compile_modes_keep_link_instrumentation_and_quote_profile_path(self):
        calls = []
        def run(name, argv, **kwargs):
            calls.append((name, [str(a) for a in argv]))
            if name.endswith("-build"):
                folder = Path(argv[2]); folder.mkdir(parents=True, exist_ok=True)
                (folder / bw.MODULE).write_bytes(b"synthetic module")
        self.b.run = run
        self.b.compile_composite(self.root / "pgo-local/composite", "0", ["-fprofile-instr-generate"],
                                 ["-fprofile-instr-generate"], "training-composite")
        config = calls[0][1]
        self.assertIn("-DCOMPOSITE_OPTIMIZATION_LEVEL=0", config)
        self.assertIn("-DCMAKE_SHARED_LINKER_FLAGS=-fuse-ld=lld -fprofile-instr-generate", config)
        self.b.profile = self.root / "profile with spaces.profdata"
        self.b.cold_sources = lambda: None  # tiering reads real counts; not what this checks
        self.b.compile_module()
        config = calls[2][1]
        self.assertIn("-DCOMPOSITE_OPTIMIZATION_LEVEL=2", config)
        flags = next(x for x in config if x.startswith("-DCMAKE_C_FLAGS="))
        self.assertIn('"-fprofile-instr-use=', flags)
        self.assertNotIn("-fprofile-instr-generate", " ".join(config))

    def test_app_configure_has_no_training_linker_flags(self):
        # The app may use its own committed profile and ThinLTO, never the module's training flags.
        def check(name, argv, **kw):
            shared = [a for a in argv if str(a).startswith("-DCMAKE_SHARED_LINKER_FLAGS=-fuse-ld=lld")]
            self.assertEqual(len(shared), 1)
            self.assertNotIn("-fprofile-instr-generate", " ".join(map(str, argv)))
        self.b.run = check
        self.b.configure_app()

    def test_app_profile_skipped_when_this_clang_cannot_read_it(self):
        # #153: the committed profile needs clang 22. An llvm-profdata that rejects it means
        # the app is configured without -fprofile-instr-use instead of failing CMake's check.
        tools = self.root / "Llvm/x64/bin"; tools.mkdir(parents=True)
        (tools / "llvm-profdata.exe").write_bytes(b"")
        rejected = "error: app.profdata: unsupported instrumentation profile format version"
        for case, result, expect in [
                ("rejects", SimpleNamespace(returncode=1, stdout="", stderr=rejected), False),
                ("reads", SimpleNamespace(returncode=0, stdout="Total functions: 6390", stderr=""), True)]:
            with self.subTest(case=case), patch.object(bw.subprocess, "run", return_value=result) as shown:
                self.b.clang = str(tools / "clang.exe"); self.b._app_profile_readable = None
                seen = []
                self.b.run = lambda name, argv, **kw: seen.append(" ".join(map(str, argv)))
                self.b.configure_app()
                self.assertEqual(shown.call_args[0][0][1:], ["show", str(bw.Builder.APP_PROFILE)])
                self.assertEqual("-fprofile-instr-use=" in seen[0], expect)
                self.assertEqual("-flto=thin" in seen[0], expect)
        self.b.clang = str(self.root / "nowhere/clang.exe"); self.b._app_profile_readable = None
        self.assertFalse(self.b.app_profile_readable())

    def test_playback_isolated_and_requires_control_and_profile(self):
        for marker, profile, succeeds in [(False, True, False), (True, False, False), (True, True, True)]:
            with self.subTest(marker=marker, profile=profile):
                run = self.root / f"run-{marker}-{profile}"
                def execute(name, argv, env):
                    self.assertNotIn("BLUEWAKE_CARD_PATH", env)
                    self.assertNotEqual(env.get("BLUEWAKE_MODS"), "user-mod")
                    self.assertEqual(env["BLUEWAKE_DATA_DIR"], str(run))
                    self.assertEqual(env["BLUEWAKE_RENDERER"], "headless")
                    self.assertEqual(env["DOL_AURORA_FRAME_INTERP"], "0")
                    self.assertEqual(env["BLUEWAKE_OVERLAP_OBSERVATION"], "1")
                    self.assertTrue(env["LLVM_PROFILE_FILE"].startswith(str(run)))
                    if profile: (run / "fixture.profraw").write_bytes(b"fixture")
                    log = self.b.logs / "playback.log"
                    log.write_text("[player-milestone] control-admitted" if marker else "boot only")
                    return log
                self.b.run = execute
                if succeeds:
                    self.assertEqual(len(self.b.training_run("host", "module", run, None)), 1)
                else:
                    with self.assertRaises(bw.BuildError): self.b.training_run("host", "module", run, None)

    def setup_training(self, count=7):
        self.b.training_fingerprint = lambda: "current-inputs"
        self.b.build_app = lambda: Path("fixture.exe")
        self.b.compile_composite = lambda *a: Path("fixture.dll")
        observed = []
        def playback(exe, module, run, mods, **kw):
            observed.append(mods);run.mkdir();p = run / "fixture.profraw";p.write_bytes(b"raw");return [p]
        self.b.training_run = playback
        def merge(name, argv, **kwargs):
            Path(argv[3]).write_bytes(b"new profile")
        self.b.run = merge
        mock = patch.object(bw.subprocess, "run", return_value=SimpleNamespace(
            returncode=0, stdout=f"  func_80001000:\n    Hash: 0x1\n    Function count: {count}\n"))
        mock.start(); self.addCleanup(mock.stop)
        return observed

    def test_profile_requires_executed_game_functions_preserves_prior_result(self):
        self.setup_training(count=0)
        work = self.root / "pgo-local";work.mkdir()
        (work / "composite.profdata").write_bytes(b"previous profile")
        (work / "training.json").write_text('{"fingerprint":"old"}')
        with self.assertRaises(bw.BuildError): self.b.train()
        self.assertEqual((work / "composite.profdata").read_bytes(), b"previous profile")
        self.assertEqual(json.loads((work / "training.json").read_text()), {"fingerprint":"old"})
        self.assertTrue(list(work.glob("attempt-*/composite.profdata")))

    def test_success_uses_plain_and_mod_runs_and_reuses_matching_profile(self):
        observed = self.setup_training()
        profile = self.b.train()
        self.assertEqual(observed, [None, "widescreen,betterww"])
        self.assertEqual(profile.read_bytes(), b"new profile")
        self.assertEqual(self.b.train(), profile)
        self.assertEqual(len(observed), 2)
        profile.write_bytes(b"corrupt hashed copy")
        self.assertEqual(self.b.train().read_bytes(), b"new profile")
        self.b.args.retrain = True
        self.b.train()
        self.assertEqual(len(observed), 4)
        self.assertEqual(len(list((self.root / 'pgo-local').glob('attempt-*'))), 2)

    def test_package_includes_atomic_wait_runtime_and_training_identity(self):
        self.b.app_build = self.root / "app";self.b.app_build.mkdir()
        (self.b.app_build / "BlueWake.exe").write_bytes(b"synthetic host")
        crt = self.root / "redist/x64/Microsoft.VC.fixture.CRT";crt.mkdir(parents=True)
        (crt / "msvcp140_atomic_wait.dll").write_bytes(b"synthetic CRT")
        self.b.env["VCToolsRedistDir"] = str(self.root / "redist")
        game = self.root / "game";(game / "rels").mkdir(parents=True)
        (game / "main.dol").write_bytes(b"synthetic input")
        self.b.recompcore = self.root / "runtime"
        dsp = self.b.recompcore / "Data/Sys/GC";dsp.mkdir(parents=True)
        for name in ("dsp_rom.bin", "dsp_coef.bin"): (dsp / name).write_bytes(b"synthetic DSP")
        self.b.iso.write_bytes(b"synthetic disc")
        (self.root / "composite-src.digest").write_text("synthetic digest")
        module = self.root / "synthetic.dll";module.write_bytes(b"synthetic module")
        self.b.profile = self.root / "local.profdata";self.b.profile.write_bytes(b"local profile")
        self.b.git = lambda *args: "" if args[0] == "status" else "test-source"
        app = self.b.package(module)
        self.assertEqual((app / "msvcp140_atomic_wait.dll").read_bytes(), b"synthetic CRT")
        receipt = json.loads((app / "BuilderProvenance.json").read_text())
        self.assertTrue(receipt["local_training"])
        self.assertEqual(receipt["composite_profile_sha256"], bw.sha256_file(self.b.profile))

    def test_fingerprint_tracks_prepared_source_runtime_options_and_host(self):
        self.b.git = lambda *a: "runtime1"
        with patch.object(bw, "ROOT", self.root), patch.object(bw, "tree_digest", return_value="source1") as digest:
            host = self.root / "runtime/host/src/main.c";host.parent.mkdir(parents=True);host.write_text("old")
            original = self.b.training_fingerprint()
            digest.return_value = "source2";self.assertNotEqual(original, self.b.training_fingerprint());digest.return_value = "source1"
            self.b.args.fixed_mem1 = True;self.assertNotEqual(original, self.b.training_fingerprint());self.b.args.fixed_mem1 = False
            host.write_text("new");self.assertNotEqual(original, self.b.training_fingerprint());host.write_text("old")
            self.b.git = lambda *a: "runtime2";self.assertNotEqual(original, self.b.training_fingerprint())


if __name__ == "__main__":
    unittest.main()
