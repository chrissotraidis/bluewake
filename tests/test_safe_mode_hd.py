"""Compile the desktop settings core without a game, renderer or Windows SDK.

Safe mode, the settings file and the HD look's switches are one implementation for
Mac, Linux and Windows (runtime/host/src/desktop_settings.cpp). The fixtures compile
it as it is, with the platform hooks stubbed the way each platform's file defines them
(settings_menu.cpp, windows/src/win_settings.cpp) and each platform's launch: when it
reads the file and when it asks for safe mode. Backup failure is injected; successful
backups and settings writes use real files.
Run with: python3 tests/test_safe_mode_hd.py
"""
import os
from pathlib import Path
import subprocess
import tempfile
import textwrap
import unittest

ROOT = Path(__file__).resolve().parents[1]
HOST = ROOT / "runtime/host/src"
EFFECTS = ["DISTANT_ISLANDS", "BLOOM", "FAR_BLUR", "AO", "CLOUD_SHADOWS",
           "SUN_SHADOWS", "EDGE_LIGHT", "TORCH_LIGHT"]


COMMON = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include "launch_marker.h"
bool fail_backup;
bool fixture_backup(const char* path) { return !fail_backup && bw_launch_backup(path); }
#define bw_launch_backup fixture_backup
extern "C" const char* bluewake_game_options_describe(uint32_t, const char**, bool*, bool*) { return nullptr; }
int bloom = -1, far_blur = -1, ao = -1;
void aurora_set_bloom(bool on) { bloom = on; }
void aurora_set_far_blur(bool on) { far_blur = on; }
void aurora_set_ambient_occlusion(bool on) { ao = on; }
int map_on = -1;
void aurora_set_sun_shadow_map(bool on) { map_on = on; }
void aurora_set_sun_shadow_quality(int) {}
std::string read_file(const std::string& path) {
    std::ifstream file(path);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}
#include "desktop_settings.cpp"
'''

# Each platform's hook, as its file defines the one the core calls; the rest of
# what differs is settings_table.h's, compiled for that platform (BW_SETTINGS_OS).
MAC = r'''
namespace bw_platform {
std::string default_pack_folder() { return ""; }
}
'''

WINDOWS = r'''
namespace bw_platform {
std::string default_pack_folder() { return "Load/Textures/GZLE01"; }
}
'''

# When each platform reads the file and asks for safe mode (settings_menu.cpp's
# bluewake_settings_load; win_settings.cpp's bw_settings_load).
LAUNCH = {
    "mac": r'''
        const std::string path = std::strcmp(argv[1], "none") == 0 ? "" : argv[1];
        bw_desktop::load(path);
        if (bw_desktop::env_on("BLUEWAKE_SAFE_MODE", false))
            bw_desktop::apply_safe_mode();
        bw_desktop::fill_defaults();
    ''',
    "windows": r'''
        const std::string dir = argv[1];
        const std::string path = dir + "settings.ini";
        bw_desktop::load(path);
        if (bw_desktop::env_on("BLUEWAKE_SAFE_MODE", false) || bw_launch_pending((dir + "launch.pending").c_str()))
            bw_desktop::apply_safe_mode();
        bw_desktop::fill_defaults();
    ''',
}

CHECK_HD = r'''
void check_hd(int on, int bloom_on, int sun_on) {
    assert(bluewake_hd_distant_islands == bool(on));
    assert(bloom == bloom_on && far_blur == on && ao == on);
    // The HD look switch leaves sun shadows off (#321): only their own switch turns them on.
    assert(g_clouds == on && g_sun == sun_on && g_edge == on && g_torch == on);
    // Sun shadows come from the shadow map whenever they are on.
    assert(sun_on ? map_on == 1 : map_on != 1);
}
'''

DRIVER = r'''
int main(int argc, char** argv) {
    assert(argc == 4);
    const std::string mode = argv[2];
    fail_backup = mode == "failure";
    const bool safe = mode != "normal";
    std::string original;
    if (std::strcmp(argv[1], "none") != 0)
        original = read_file(PATH_OF_ARG);
    LAUNCH
    // Without a safe launch, or when the file could not be copied aside, the
    // file is as it was (an old Windows file is rewritten in the shared format).
    if ((!safe || fail_backup) && mode != "old")
        assert(path.empty() || read_file(path) == original);
    bluewake_hd_look_reload();
    check_hd(safe ? 0 : 1, safe ? 0 : std::atoi(argv[3]), safe ? 0 : 1);
    // Another change saves the file: a session-only reset (the copy failed)
    // must not reach it, and a recorded one must stay.
    bw_desktop::set_env("DOL_AURORA_SHOW_FPS", "1");
    bw_desktop::save();
    if (!path.empty()) {
        const std::string saved = read_file(path);
        assert(saved.find(std::string("BLUEWAKE_HD_LOOK=") + (safe && !fail_backup ? "0" : "1") + "\n") !=
               std::string::npos);
        assert(saved.find("DOL_AURORA_SHOW_FPS=1\n") != std::string::npos);
    }
    if (safe) {
        // The safe flag stays set; a later user choice must work.
        bw_desktop::set_env("BLUEWAKE_HD_LOOK", "1");
        bluewake_hd_look_reload();
        check_hd(1, 1, 0);
        bw_desktop::set_env("BLUEWAKE_HD_LOOK", "0");
        bluewake_hd_look_reload();
        check_hd(0, 0, 0);
    }
}
'''


class SafeModeHDTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix="bluewake-safe-hd-")
        cls.addClassCleanup(cls.temp.cleanup)
        cls.build = Path(cls.temp.name)
        (cls.build / "aurora").mkdir()
        (cls.build / "aurora/aurora.h").write_text(
            "enum { FPS_OVERLAY_TOP_CENTER, FPS_OVERLAY_TOP_LEFT, FPS_OVERLAY_TOP_RIGHT, "
            "FPS_OVERLAY_BOTTOM_LEFT, FPS_OVERLAY_BOTTOM_RIGHT };\n")
        hd = (HOST / "hd_look.c").read_text()
        hd = hd[hd.index("bool bluewake_hd_distant_islands;"):hd.index("// GZLE01 revision 0.")]
        for platform in ("mac", "windows"):
            hooks = MAC if platform == "mac" else WINDOWS
            argument = "argv[1]" if platform == "mac" else 'std::string(argv[1]) + "settings.ini"'
            launch = textwrap.indent(textwrap.dedent(LAUNCH[platform]).strip("\n"), "    ")
            driver = DRIVER.replace("    LAUNCH", launch).replace("PATH_OF_ARG", argument)
            source = cls.build / (platform + ".cpp")
            source.write_text(COMMON + hooks + hd + CHECK_HD + driver)
            platform_os = "-DBW_SETTINGS_OS=Os::" + ("Mac" if platform == "mac" else "Windows")
            subprocess.run(["c++", "-std=c++20", platform_os, "-Wall", "-Wextra", "-Werror", "-Wno-unused-variable",
                            "-Wno-unused-function", "-I", str(cls.build), "-I", str(HOST),
                            "-I", str(ROOT / "windows/src"), str(source),
                            "-o", str(cls.build / platform)], check=True)

    def run_case(self, platform, mode, bloom):
        with tempfile.TemporaryDirectory(dir=self.build) as temp:
            directory = Path(temp)
            path = directory / "settings.ini"
            overrides = {"BLUEWAKE_HD_" + effect: "1" for effect in EFFECTS}
            overrides["BLUEWAKE_HD_BLOOM"] = str(bloom)
            if mode == "old":
                # A Windows file from before the menus were shared.
                original = "hd_look=1\nmouse_sensitivity=2.50\n"
            else:
                original = "BLUEWAKE_HD_LOOK=1\n" + "".join(k + "=" + v + "\n" for k, v in overrides.items())
                original += "BLUEWAKE_HD_TRACE=1\nUNRELATED_SETTING=kept\n"
            if mode != "none":
                path.write_text(original)
            env = {k: v for k, v in os.environ.items() if not k.startswith(("BLUEWAKE_", "DOL_"))}
            env.update(overrides, BLUEWAKE_HD_LOOK="1")
            if mode in ("safe", "failure", "none"):
                env["BLUEWAKE_SAFE_MODE"] = "1"
            if mode in ("pending", "old"):
                (directory / "launch.pending").write_text("fixture\n")
            argument = "none" if mode == "none" else str(path)
            if platform == "windows":
                argument = str(directory) + "/"
            result = subprocess.run([str(self.build / platform), argument, mode, str(bloom)],
                                    env=env, text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            backups = list(directory.glob("settings.ini.before-safe-mode-*"))
            if mode in ("safe", "pending", "old"):
                self.assertEqual(len(backups), 1)
                saved = path.read_text()
                self.assertIn("BLUEWAKE_HD_LOOK=0\n", saved)
                if mode == "old":
                    # Converted first (the old file kept once), then made safe.
                    self.assertEqual((directory / "settings.ini.before-shared-menu").read_text(), original)
                    self.assertIn("BLUEWAKE_HD_LOOK=1\n", backups[0].read_text())
                    self.assertIn("BLUEWAKE_MOUSE_SENSITIVITY=2.50\n", saved)
                else:
                    self.assertEqual(backups[0].read_text(), original)
                    for key in overrides:
                        if key == "BLUEWAKE_HD_SUN_SHADOWS":
                            # A row of the settings table (Experimental): written, but off.
                            self.assertNotIn(key + "=1", saved)
                        else:
                            self.assertNotIn(key + "=", saved)
                    self.assertIn("UNRELATED_SETTING=kept\n", saved)
                    self.assertIn("BLUEWAKE_HD_TRACE=1\n", saved)
            else:
                self.assertFalse(backups)
                if mode == "none":
                    self.assertFalse(path.exists())
                else:
                    # The player's HD look and its per-effect switches are kept.
                    saved = path.read_text()
                    self.assertIn("BLUEWAKE_HD_LOOK=1\n", saved)
                    for key, value in overrides.items():
                        self.assertIn(key + "=" + value + "\n", saved)
                    self.assertIn("UNRELATED_SETTING=kept\n", saved)

    def test_mac_linux_recovery_and_normal_overrides(self):
        for mode in ("normal", "safe", "failure", "none"):
            for bloom in (0, 1):
                with self.subTest(mode=mode, bloom=bloom):
                    self.run_case("mac", mode, bloom)

    def test_windows_recovery_session_and_normal_overrides(self):
        for mode in ("normal", "safe", "failure", "pending", "old"):
            for bloom in (0, 1):
                with self.subTest(mode=mode, bloom=bloom):
                    self.run_case("windows", mode, bloom)


if __name__ == "__main__":
    unittest.main()
