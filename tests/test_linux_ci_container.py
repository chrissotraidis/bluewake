#!/usr/bin/env python3
"""Contract tests for the Linux CI-container wrapper (no Docker required)."""
import argparse
import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import Mock, patch


SCRIPT = Path(__file__).parents[1] / "scripts/linux/build.py"
SPEC = importlib.util.spec_from_file_location("bluewake_linux_build", SCRIPT)
BUILD = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BUILD)


class ToolchainPolicyTest(unittest.TestCase):
    """Protect the deliberate compiler split in the shared environment.

    Clang 18 remains the reproducible default. Developers can explicitly test
    the LLVM version used for current performance work without silently moving
    CI or ordinary container builds to a new compiler.
    """

    def test_clang_18_is_the_default_while_the_image_also_supplies_llvm_22(self):
        """PATH selects LLVM 18 unless the builder handles --clang22."""
        dockerfile = (BUILD.ROOT / "scripts/linux/Dockerfile").read_text()
        workflow = (BUILD.ROOT / ".github/workflows/linux-host.yml").read_text()

        self.assertIn("ARG LLVM_VERSION=22", dockerfile)
        self.assertLess(dockerfile.index("/usr/lib/llvm-18/bin"),
                        dockerfile.index("/usr/lib/llvm-22/bin"))
        self.assertIn("-DCMAKE_C_COMPILER=clang-18", workflow)
        self.assertIn("-DCMAKE_CXX_COMPILER=clang++-18", workflow)

    def test_clang_22_opt_in_prepends_its_tool_directory(self):
        """The opt-in changes lookup only inside that builder process."""
        args = argparse.Namespace(clang22=True)
        with tempfile.TemporaryDirectory() as temporary, \
                patch.object(BUILD, "LLVM22_BIN", Path(temporary)), \
                patch.dict(BUILD.os.environ, {"PATH": "/default/bin"}, clear=True):
            BUILD.select_container_toolchain(args)
            self.assertEqual(BUILD.os.environ["PATH"], f"{temporary}:/default/bin")

    def test_default_does_not_modify_the_tool_path(self):
        """Omitting --clang22 preserves the image's Clang 18 selection."""
        with patch.dict(BUILD.os.environ, {"PATH": "/default/bin"}, clear=True):
            BUILD.select_container_toolchain(argparse.Namespace(clang22=False))
            self.assertEqual(BUILD.os.environ["PATH"], "/default/bin")

    def test_clang_22_requires_the_container(self):
        """The host toolchain cannot be changed implicitly by this option."""
        result = subprocess.run(
            [sys.executable, SCRIPT, "/nonexistent/disc.iso", "--clang22"],
            capture_output=True, text=True)
        self.assertEqual(result.returncode, 2)
        self.assertIn("--clang22 requires --ci-container", result.stderr)


class GatherAliasCachePolicyTest(unittest.TestCase):
    """Keep the measured gather optimization in normal Linux builds.

    The plain conservative build remains available, and an explicit opt-out
    keeps source-matched control builds possible without changing source.
    """

    def test_normal_linux_build_enables_the_cache(self):
        self.assertIn("gather_alias_cache", BUILD.LINUX_DEFAULT_OPTIMIZATIONS)

    def test_builder_exposes_an_explicit_control_opt_out(self):
        result = subprocess.run(
            [sys.executable, SCRIPT, "--help"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0)
        self.assertIn("--no-gather-alias-cache", result.stdout)


class LinuxContainerMountTest(unittest.TestCase):
    """The wrapper exposes only the paths the unchanged builder needs.

    A checkout must stay writable because the builder keeps dependencies and
    normal outputs there. Worktree metadata is read-only, external output is
    writable, and the player's disc is always narrowed back to read-only.
    """

    def test_mounts_checkout_metadata_external_output_and_read_only_disc(self):
        """A worktree and external output receive the minimum required access."""
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary).resolve()  # macOS temp folders resolve through /private
            root = base / "checkout"
            git_dir = base / "git/worktrees/checkout"
            out = base / "output"
            disc = base / "disc.iso"
            for directory in (root, git_dir, out):
                directory.mkdir(parents=True)
            disc.touch()
            with patch.object(BUILD, "ROOT", root), \
                    patch.object(BUILD, "_git_metadata_paths", return_value=[git_dir]):
                self.assertEqual(BUILD.container_mounts(disc, out), [
                    (root, "rw"), (git_dir, "ro"), (out, "rw"), (disc, "ro")])

    def test_output_inside_checkout_needs_no_duplicate_mount(self):
        """The checkout mount already covers its normal build/linux output."""
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary).resolve()  # macOS temp folders resolve through /private
            root = base / "checkout"
            out = root / "build/linux"
            disc = base / "disc.iso"
            out.mkdir(parents=True)
            disc.touch()
            with patch.object(BUILD, "ROOT", root), \
                    patch.object(BUILD, "_git_metadata_paths", return_value=[]):
                self.assertEqual(BUILD.container_mounts(disc, out), [
                    (root, "rw"), (disc, "ro")])


class LinuxContainerRunTest(unittest.TestCase):
    """The Docker layer wraps the CLI; it does not reinterpret the build."""

    def test_reexecutes_the_original_arguments_and_returns_its_status(self):
        """Every user argument and the container's exit status pass through."""
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary).resolve()  # macOS temp folders resolve through /private
            root = base / "checkout"
            context = root / "scripts/linux"
            out = base / "output"
            disc = base / "disc.iso"
            context.mkdir(parents=True)
            disc.touch()
            args = argparse.Namespace(disc=disc, out=out)
            original = [str(SCRIPT), str(disc), "--out", str(out), "--ci-container", "--clang22",
                        "--appimage", "--no-train"]
            completed = [subprocess.CompletedProcess(["docker", "info"], 0),
                         subprocess.CompletedProcess(["docker", "build"], 0),
                         subprocess.CompletedProcess(["docker", "run"], 17)]
            with patch.object(BUILD, "ROOT", root), \
                    patch.object(BUILD, "CI_CONTAINER_IMAGE", "test-image"), \
                    patch.object(BUILD, "container_mounts", return_value=[(root, "rw"), (disc, "ro")]), \
                    patch.object(BUILD.shutil, "which", return_value="/usr/bin/docker"), \
                    patch.object(BUILD.subprocess, "run", side_effect=completed) as run, \
                    patch.object(sys, "argv", original):
                self.assertEqual(BUILD.run_in_ci_container(args), 17)

            self.assertEqual(run.call_args_list[1].args[0], [
                "/usr/bin/docker", "build", "--platform", "linux/amd64", "--file", context / "Dockerfile",
                "--tag", "test-image", context])
            container = run.call_args_list[2].args[0]
            forwarded = ["python3", str(SCRIPT), *original[1:]]
            self.assertEqual(container[-len(forwarded):], forwarded)
            self.assertIn(f"{BUILD.CI_CONTAINER_ENV}=1", container)
            self.assertIn(f"{disc}:{disc}:ro", container)
            self.assertIn("linux/amd64", container)

    def test_missing_docker_is_a_clear_builder_error(self):
        """Fail before touching the disc or output when Docker is unavailable."""
        args = argparse.Namespace(disc=Path("disc.iso"), out=Path("build/linux"))
        with patch.object(BUILD.shutil, "which",
                          side_effect=lambda tool: "/usr/bin/git" if tool == "git" else None):
            with self.assertRaisesRegex(BUILD.BuildError, "requires Docker"):
                BUILD.run_in_ci_container(args)


class CpuProbeContractTest(unittest.TestCase):
    """Keep compiler selection changes isolated to the container invocation.

    LLVM releases differ on two feature names used by the existing probe, while
    Ubuntu's system C compiler accepts them. The probe only selects -march;
    every actual BlueWake artifact must continue to use Clang.
    """

    REPORT = "sse4.2=1 popcnt=1 movbe=1 fma=1 avx=1 avx2=1 bmi=1 bmi2=1 lzcnt=1\n"

    def _probe(self, container):
        """Capture the selected probe compiler and generated C without compiling."""
        with tempfile.TemporaryDirectory() as temporary:
            args = argparse.Namespace(out=Path(temporary), march="x86-64-v3", ci_container=container)
            builder = BUILD.Builder(args)
            builder.clang = "/selected/clang"
            environment = {BUILD.CI_CONTAINER_ENV: "1"} if container else {}
            with patch.dict(BUILD.os.environ, environment, clear=True), \
                    patch.object(BUILD.shutil, "which", return_value="/usr/bin/cc"), \
                    patch.object(BUILD.subprocess, "run") as run, \
                    patch.object(BUILD.subprocess, "check_output", return_value=self.REPORT):
                builder.check_march()
                source = (args.out / "tools/march_probe.c").read_text()
            return run.call_args.args[0][0], source

    def test_regular_build_retains_the_clang_probe(self):
        """No flag means the pre-existing probe source and compiler are retained."""
        compiler, source = self._probe(False)
        self.assertEqual(compiler, "/selected/clang")
        self.assertNotIn("!!__builtin_cpu_supports", source)

    def test_container_uses_cc_only_for_the_compatible_probe(self):
        """The container normalizes GCC's feature bitmasks to boolean results."""
        compiler, source = self._probe(True)
        self.assertEqual(compiler, "/usr/bin/cc")
        self.assertIn("!!__builtin_cpu_supports", source)


class AppImagePackagingTest(unittest.TestCase):
    """The explicit option delegates to the established packaging script.

    These tests exercise only the new builder-to-packager boundary. The shell
    packager's actual ELF audit and launch behavior are covered by the Docker
    smoke test used by Linux CI.
    """

    def test_packager_receives_the_build_directory_and_checks_both_outputs(self):
        """A successful packaging step must create the AppImage and its zsync."""
        with tempfile.TemporaryDirectory() as temporary:
            out = Path(temporary)
            builder = BUILD.Builder(argparse.Namespace(out=out))

            def produce_outputs(name, command):
                if name == "appimage":
                    Path(command[2]).touch()
                    Path(f"{command[2]}.zsync").touch()

            builder.run = Mock(side_effect=produce_outputs)
            image, zsync = builder.package_appimage()

            self.assertEqual(image, out / "BlueWake-x86_64.AppImage")
            self.assertEqual(zsync, out / "BlueWake-x86_64.AppImage.zsync")
            self.assertEqual(builder.run.call_args_list[0].args, ("appimage", [
                BUILD.ROOT / "scripts/linux/make_appimage.sh", out, image]))
            self.assertEqual(builder.run.call_args_list[1].args, ("appimage-abi", [
                sys.executable, BUILD.ROOT / "scripts/linux/check_appimage_abi.py", image, "2.39"]))

    def test_missing_packaging_output_is_reported(self):
        """A zero-exit packager cannot silently omit either promised artifact."""
        with tempfile.TemporaryDirectory() as temporary:
            builder = BUILD.Builder(argparse.Namespace(out=Path(temporary)))
            builder.run = Mock()
            with self.assertRaisesRegex(BUILD.BuildError, "did not produce"):
                builder.package_appimage()


class DependencyPreflightTest(unittest.TestCase):
    """Optional packaging dependencies are rejected before compilation starts."""

    def test_missing_appimage_tools_are_reported_together(self):
        """The error names every absent tool instead of failing hours later."""
        builder = BUILD.Builder(argparse.Namespace(out=Path("build/linux")))
        with patch.dict(BUILD.os.environ, {}, clear=True), \
                patch.object(BUILD.shutil, "which", return_value=None):
            with self.assertRaisesRegex(
                    BUILD.BuildError,
                    "appimagetool, desktop-file-validate, zsyncmake, readelf, ImageMagick"):
                builder.check_appimage_tools()


class AppImageRuntimeTest(unittest.TestCase):
    """The AppImage's host uses LLVM libc++, as CI's checked AppImage does.

    The prebuilt Dawn package links libstdc++, so a host built with it needs the
    player's libstdc++ and check_appimage_abi.py rejects the AppImage
    (GLIBCXX_3.4.32 on a container build, reported on #371)."""

    def configure(self, appimage):
        builder = BUILD.Builder(argparse.Namespace(out=Path("build/linux"), appimage=appimage,
                                                   march="x86-64-v3"))
        seen = []
        builder.run = lambda name, argv, **kw: seen.append([str(a) for a in argv])
        builder.configure_app()
        return builder, seen[0]

    def test_appimage_builds_with_libcxx_and_dawn_from_source(self):
        builder, argv = self.configure(True)
        self.assertIn("-DCMAKE_CXX_FLAGS=-march=x86-64-v3 -g -stdlib=libc++", argv)
        for flag in ("-DCMAKE_EXE_LINKER_FLAGS=-stdlib=libc++", "-DCMAKE_SHARED_LINKER_FLAGS=-stdlib=libc++",
                     "-DAURORA_DAWN_PROVIDER=vendor"):
            self.assertIn(flag, argv)
        self.assertNotIn("-DAURORA_DAWN_PROVIDER=package", argv)
        self.assertEqual(builder.app_build, Path("build/linux/app-appimage"))
        # The same runtime as the CI job whose AppImage passes the ABI check.
        workflow = (BUILD.ROOT / ".github/workflows/linux-host.yml").read_text()
        for flag in ("-DCMAKE_CXX_FLAGS=-stdlib=libc++", "-DCMAKE_EXE_LINKER_FLAGS=-stdlib=libc++",
                     "-DAURORA_DAWN_PROVIDER=vendor"):
            self.assertIn(flag, workflow)

    def test_folder_build_keeps_the_prebuilt_dawn(self):
        builder, argv = self.configure(False)
        self.assertIn("-DAURORA_DAWN_PROVIDER=package", argv)
        self.assertFalse(any("-stdlib=libc++" in a for a in argv))
        self.assertEqual(builder.app_build, Path("build/linux/app"))


if __name__ == "__main__":
    unittest.main()
