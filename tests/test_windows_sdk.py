#!/usr/bin/env python3
"""Windows SDK fallback when vcvars cannot find the SDK, using a synthetic Windows Kits folder."""
import importlib.util
import io
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

REPO = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("windows_builder", REPO / "scripts/windows/build.py")
bw = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bw)


class WindowsSdkTest(unittest.TestCase):
    def test_windows_sdk_added_when_vcvars_cannot_find_it(self):
        # Policy can block reg.exe, which vcvars uses; a partial newer SDK is skipped.
        with tempfile.TemporaryDirectory(prefix="windows sdk 雪 ") as tmp:
            kits = Path(tmp) / "Windows Kits/10"
            for version, parts in (("10.0.26100.0", ("Include/{v}/ucrt/stdlib.h", "Include/{v}/um/windows.h",
                                                     "Lib/{v}/ucrt/x64/ucrt.lib", "Lib/{v}/um/x64/kernel32.lib")),
                                   ("10.0.28000.0", ("Include/{v}/ucrt/stdlib.h",))):
                for part in parts:
                    path = kits / part.format(v=version)
                    path.parent.mkdir(parents=True, exist_ok=True); path.touch()
            (kits / "bin/10.0.26100.0/x64").mkdir(parents=True)
            env = {"INCLUDE": "msvc", "LIB": "msvc lib", "PATH": "tools"}
            with patch.object(bw.sys, "stdout", io.StringIO()):
                bw.add_windows_sdk(env, kits)
            include = env["INCLUDE"].split(os.pathsep)
            self.assertEqual(include[0], str(kits / "Include/10.0.26100.0/ucrt"))
            self.assertIn(str(kits / "Include/10.0.26100.0/um"), include)
            self.assertEqual(include[-1], "msvc")
            self.assertEqual(env["LIB"].split(os.pathsep), [str(kits / "Lib/10.0.26100.0/ucrt/x64"),
                                                            str(kits / "Lib/10.0.26100.0/um/x64"), "msvc lib"])
            self.assertEqual(env["PATH"].split(os.pathsep), [str(kits / "bin/10.0.26100.0/x64"), "tools"])
            self.assertEqual(env["UCRTVersion"], "10.0.26100.0")

    def test_no_sdk_leaves_environment_unchanged(self):
        with tempfile.TemporaryDirectory() as tmp:
            env = {"INCLUDE": "msvc"}
            bw.add_windows_sdk(env, Path(tmp))
            self.assertEqual(env, {"INCLUDE": "msvc"})


if __name__ == "__main__":
    unittest.main()
