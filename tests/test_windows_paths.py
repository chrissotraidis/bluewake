#!/usr/bin/env python3
"""Public Windows path, subprocess and extractor configuration regressions."""
import importlib.util
import io
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
from types import SimpleNamespace
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("windows_builder", ROOT / "scripts/windows/build.py")
bw = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bw)


class WindowsPathsTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="BlueWake paths 雪 ")
        def cleanup():
            # Windows may briefly retain the just-exited executable's image mapping.
            for attempt in range(20):
                try:
                    self.temp.cleanup()
                    return
                except PermissionError:
                    if attempt == 19:
                        raise
                    time.sleep(0.1)
        self.addCleanup(cleanup)
        self.work = Path(self.temp.name)
        self.builder = bw.Builder(SimpleNamespace(out=self.work, console=False,
                                                  march="x86-64-v2", no_app_pgo=True))

    def test_extractor_and_desktop_explicitly_select_their_mode(self):
        calls = []
        self.builder.run = lambda name, argv: calls.append(argv)
        for mode in (True, False):
            self.builder.configure_app(extractor_only=mode)
        a, b = calls
        self.assertIn("-DBLUEWAKE_WINDOWS_EXTRACTOR_ONLY=ON", a)
        self.assertIn("-DBLUEWAKE_WINDOWS_EXTRACTOR_ONLY=OFF", b)
        self.assertEqual([v for v in a if "EXTRACTOR_ONLY=" not in str(v)],
                         [v for v in b if "EXTRACTOR_ONLY=" not in str(v)])

    def test_vs_environment_keeps_unicode_without_changing_ambient_values(self):
        programs = self.work / "Program Files"
        vswhere = programs / "Microsoft Visual Studio/Installer/vswhere.exe"
        vswhere.parent.mkdir(parents=True); vswhere.touch()
        install = self.work / "Visual Studio 雪"
        clang = install / "VC/Tools/Llvm/x64/bin/clang.exe"
        clang.parent.mkdir(parents=True); clang.touch()
        found = json.dumps([dict(installationPath=str(install), installationVersion="18.0")])
        result = SimpleNamespace(returncode=0, stdout="INCLUDE=雪 headers\nPATH=tools\nTEMP=雪 temp\n")
        with patch.dict(os.environ, {"ProgramFiles(x86)": str(programs), "TEMP": "original 雪"}), \
                patch.object(bw.subprocess, "check_output", return_value=found) as query, \
                patch.object(bw.subprocess, "run", return_value=result) as run:
            env = self.builder.visual_studio_env()
            self.assertEqual(env["TEMP"], "雪 temp")
            self.assertEqual(os.environ["TEMP"], "original 雪")
        self.assertIn("-utf8", query.call_args.args[0])
        self.assertEqual(query.call_args.kwargs["encoding"], "utf-8")
        self.assertIn("cmd /d /u /c", run.call_args.args[0])
        self.assertEqual(run.call_args.kwargs["encoding"], "utf-16le")

    def test_python_child_prints_unicode_without_environment_utf8(self):
        script = self.work / "child 雪.py"
        script.write_text("import sys; assert sys.flags.utf8_mode; print('path 雪')\n", encoding="utf-8")
        env = {k: v for k, v in os.environ.items() if not k.upper().startswith("PYTHON")}
        self.builder.run("unicode-child", [sys.executable, script], env=env)
        self.assertEqual((self.builder.logs / "unicode-child.log").read_bytes(),
                         b"path \xe9\x9b\xaa" + (b"\r\n" if sys.platform == "win32" else b"\n"))

    def test_cli_reconfigures_real_legacy_streams_before_status_and_errors(self):
        for relative in ("scripts/windows/build.py", "scripts/builder/padmint.py"):
            with self.subTest(script=relative):
                code = '''import importlib.util, io, sys
from unittest.mock import patch
spec=importlib.util.spec_from_file_location('subject',sys.argv[1]);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
sys.stdout=io.TextIOWrapper(sys.stdout.buffer,encoding='cp1252')
sys.stderr=io.TextIOWrapper(sys.stderr.buffer,encoding='cp1252')
sys.argv=[sys.argv[1], 'input 雪.iso', '--out', sys.argv[2], '--jobs', '1', '--source-only']
with patch.object(sys,'platform','win32'):
 if hasattr(m,'Builder'):
  with patch.object(m.Builder,'check_tools',side_effect=m.BuildError('fixture 雪')): m.main()
 else:
  with patch.object(m,'build',side_effect=ValueError('fixture 雪')): m.main()
'''
                run = subprocess.run([sys.executable, "-I", "-X", "utf8", "-c", code,
                                      str(ROOT / relative), str(self.work)], capture_output=True)
                self.assertEqual(run.returncode, 1, run.stderr)
                self.assertIn("fixture 雪", run.stderr.decode("utf-8"))
                self.assertNotIn(b"UnicodeEncodeError", run.stderr)
                if relative.startswith("scripts/windows"):
                    self.assertIn("input 雪.iso", run.stdout.decode("utf-8"))

    def test_custom_capture_stream_is_supported(self):
        stream = io.StringIO()
        with patch.object(bw.sys, "platform", "win32"), patch.object(bw.sys, "stdout", stream):
            bw.configure_output()

    @unittest.skipUnless(sys.platform == "win32", "requires Windows C compiler and SDK")
    def test_real_extractor_build_without_renderer_tree(self):
        # Copy the exact production configuration and extractor inputs. There is
        # deliberately no RecompCore/GXRuntime checkout in this public fixture.
        for name in ("windows/CMakeLists.txt", "windows/src/bluewake_version.h.in", "version.json",
                     "scripts/ios/disc_extract.c", "apple/ios/src/disc_import.c", "apple/ios/src/disc_import.h"):
            destination = self.work / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(ROOT / name, destination)
        shutil.copytree(ROOT / "windows/compat", self.work / "windows/compat")
        env = self.builder.visual_studio_env()
        cmake = shutil.which("cmake", path=env["PATH"])
        self.assertIsNotNone(cmake)
        command = [cmake, "-S", str(self.work / "windows"), "-B", str(self.work / "build"), "-G", "Ninja",
                   "-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_C_COMPILER=clang", "-DCMAKE_CXX_COMPILER=clang++",
                   "-DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=lld", "-DBLUEWAKE_WINDOWS_EXTRACTOR_ONLY=ON",
                   "-DCMAKE_SYSTEM_NAME=Windows", "-DCMAKE_SYSTEM_PROCESSOR=AMD64"]
        for argv in (command, [cmake, "--build", str(self.work / "build"), "--target", "bluewake_disc_extract", "-j1"]):
            result = subprocess.run(argv, env=env, capture_output=True)
            self.assertEqual(result.returncode, 0, (result.stdout + result.stderr).decode("utf-8", errors="replace"))
        exe = self.work / "build/bluewake_disc_extract.exe"
        self.assertTrue(exe.is_file())
        result = subprocess.run([str(exe)], capture_output=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(b"usage", result.stderr.lower())
        self.assertFalse((self.work / "build/gxruntime_build").exists())


if __name__ == "__main__":
    bw.configure_output()
    unittest.main()
