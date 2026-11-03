"""Process-list fixtures only: do not launch or stop a real game."""
from pathlib import Path
import os
import subprocess
import unittest

GUARD = Path(__file__).resolve().parents[1] / "scripts/one_game_guard.sh"
MOCK_PGREP = r"""
pgrep() {
    case "$1" in
        -x) [[ "$BW_GUARD_TEST_NAME" = "$2" ]] ;;
        -f) [[ "$BW_GUARD_TEST_COMMAND" =~ $2 ]] ;;
        *) return 1 ;;
    esac
}
export -f pgrep
exec bash "$@"
"""


class OneGameGuardTest(unittest.TestCase):
    def check_guard(self, command="", name="", ignore_sim=False):
        environment = dict(os.environ, BW_GUARD_TEST_COMMAND=command,
                           BW_GUARD_TEST_NAME=name)
        args = ["bash", "-c", MOCK_PGREP, "guard-test", str(GUARD)]
        if ignore_sim:
            args.append("--ignore-sim")
        return subprocess.run(args, env=environment, capture_output=True,
                              text=True, timeout=5)

    def test_mac_app_without_arguments(self):
        result = self.check_guard("/Applications/BlueWake.app/Contents/MacOS/BlueWake")
        self.assertEqual(result.returncode, 1)
        self.assertIn("BlueWake.app(macOS)", result.stderr)

    def test_mac_app_with_module_argument(self):
        result = self.check_guard(
            "/private/test/BlueWake.app/Contents/MacOS/BlueWake /private/test/module.dylib")
        self.assertEqual(result.returncode, 1)
        self.assertIn("BlueWake.app(macOS)", result.stderr)

    def test_mac_path_with_spaces_is_still_blocked_when_ignoring_sim(self):
        result = self.check_guard(
            "/private/fixture with spaces/BlueWake.app/Contents/MacOS/BlueWake --smooth",
            ignore_sim=True)
        self.assertEqual(result.returncode, 1)

    def test_simulator_can_be_explicitly_ignored(self):
        command = "/fixture/Bundle/Application/fixture/BlueWake.app/BlueWake --test"
        self.assertEqual(self.check_guard(command).returncode, 1)
        self.assertEqual(self.check_guard(command, ignore_sim=True).returncode, 0)

    def test_raw_host_and_dolphin_remain_blocked(self):
        for name in ("bluewake_host", "Dolphin"):
            with self.subTest(name=name):
                result = self.check_guard(name=name)
                self.assertEqual(result.returncode, 1)
                self.assertIn(name, result.stderr)

    def test_unrelated_process_and_suffix_do_not_match(self):
        for command in ("/Applications/Other.app/Contents/MacOS/Other",
                        "/fixture/BlueWake.app/Contents/MacOS/BlueWakeHelper",
                        "/fixture/Other.app/Contents/MacOS/BlueWake"):
            with self.subTest(command=command):
                self.assertEqual(self.check_guard(command).returncode, 0)

    def test_empty_process_list(self):
        self.assertEqual(self.check_guard().returncode, 0)


if __name__ == "__main__":
    unittest.main()
