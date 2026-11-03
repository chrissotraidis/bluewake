"""Mac dependency regression; downloads pinned public Aurora/SQLite, never game data."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
ENTRYPOINT = ROOT / "scripts/builder/training"


@unittest.skipUnless(sys.platform == "darwin", "macOS framework discovery regression")
class MacSQLiteTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not shutil.which("cmake"):
            raise RuntimeError("CMake 3.25+ is required for the Mac SQLite regression")
        cls.temp = tempfile.TemporaryDirectory(prefix="bluewake-sqlite-")
        cls.addClassCleanup(cls.temp.cleanup)
        cls.fixture = Path(cls.temp.name)
        lock = json.loads((ROOT / "config/dependencies.lock.json").read_text())
        runtime = next(item for item in lock["dependencies"] if item["id"] == "recompcore")
        repository = runtime["url"].removeprefix("https://github.com/").removesuffix(".git")
        url = ("https://raw.githubusercontent.com/" + repository + "/" + runtime["sha"]
               + "/GXRuntime/graphics/aurora/extern/CMakeLists.txt")
        with urllib.request.urlopen(url, timeout=60) as response:
            extern = response.read().decode("utf-8")
        # Run the pinned provider itself, without building unrelated Dawn/SDL/DSP code.
        start = extern.index("  aurora_find_package_global(SQLite3)")
        end = extern.index("  if (AURORA_CACHE_USE_ZSTD)", start)
        (cls.fixture / "sqlite.cmake").write_text("include(FetchContent)\n" + extern[start:end])
        cls.headers = cls.fixture / "Frameworks/Mono.framework/Headers"
        cls.headers.mkdir(parents=True)
        (cls.headers / "sqlite3.h").write_text(
            '#define SQLITE_VERSION "3.9.2"\n#error Unexpected Mono SQLite header\n')
        cls.external = cls.fixture / "homebrew/opt/sqlite"
        (cls.external / "include").mkdir(parents=True)
        (cls.external / "include/sqlite3.h").write_text('#define SQLITE_VERSION "3.54.0"\n')
        (cls.external / "lib").mkdir()
        # Discovery only needs a file. Linking this deliberately invalid dylib must fail.
        cls.library = cls.external / "lib/libsqlite3.dylib"
        cls.library.write_text("Unusable external SQLite fixture\n")
        (cls.fixture / "probe.c").write_text(r'''
#include <stdio.h>
#include <sqlite3.h>
int main(void) {
    sqlite3 *db = NULL;
    sqlite3_stmt *stmt = NULL;
    if (SQLITE_VERSION_NUMBER != sqlite3_libversion_number()) return 1;
    if (sqlite3_open(":memory:", &db) != SQLITE_OK) return 2;
    if (sqlite3_exec(db, "CREATE TABLE probe(value); INSERT INTO probe VALUES(1)",
                     NULL, NULL, NULL) != SQLITE_OK) return 3;
    if (sqlite3_changes64(db) != 1) return 4;
    if (sqlite3_prepare_v3(db, "SELECT value FROM probe", -1, SQLITE_PREPARE_PERSISTENT,
                          &stmt, NULL) != SQLITE_OK) return 5;
    if (sqlite3_step(stmt) != SQLITE_ROW || sqlite3_column_int(stmt, 0) != 1) return 6;
    printf("SQLite %s: persistent preparation and changes64 passed\n", SQLITE_VERSION);
    sqlite3_finalize(stmt);
    return sqlite3_close(db) == SQLITE_OK ? 0 : 7;
}
''')
        (cls.fixture / "hook.cmake").write_text(r'''
function(aurora_find_package_global)
    set(CMAKE_FIND_PACKAGE_TARGETS_GLOBAL ON)
    find_package(${ARGV})
endfunction()
# Execute the real entrypoint through its dependency policy, replacing only the
# expensive host subdirectory. return() in this macro ends the caller's file.
macro(add_subdirectory source_dir)
    if(NOT "${source_dir}" STREQUAL "${BLUEWAKE_REPO_ROOT}/runtime/host")
        message(FATAL_ERROR "Unexpected subdirectory: ${source_dir}")
    endif()
    if(SQLITE_PROBE_EXPECT_SYSTEM)
        # Negative control: reproduce the old policy and leave its discoveries cached.
        set(CMAKE_DISABLE_FIND_PACKAGE_SQLite3 FALSE)
    endif()
    include("${SQLITE_PROBE_FIXTURE}/sqlite.cmake")
    get_target_property(sqlite_type sqlite3 TYPE)
    if(SQLITE_PROBE_EXPECT_SYSTEM)
        if(NOT sqlite_type STREQUAL "UNKNOWN_LIBRARY")
            message(FATAL_ERROR "Negative control did not discover external SQLite")
        endif()
    else()
        if(NOT sqlite_type STREQUAL "STATIC_LIBRARY")
            message(FATAL_ERROR "Mac host selected external SQLite: ${sqlite_type}")
        endif()
        get_target_property(sqlite_includes sqlite3 INTERFACE_INCLUDE_DIRECTORIES)
        if(NOT sqlite_includes STREQUAL "${sqlite3_SOURCE_DIR}")
            message(FATAL_ERROR "SQLite header does not come from its pinned source")
        endif()
        add_executable(sqlite_probe "${SQLITE_PROBE_FIXTURE}/probe.c")
        target_link_libraries(sqlite_probe PRIVATE sqlite3)
    endif()
    return()
endmacro()
''')

    def run_command(self, command, **kwargs):
        result = subprocess.run(command, capture_output=True, text=True, **kwargs)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        return result.stdout

    def configure(self, build, negative_control=False):
        environment = os.environ.copy()
        environment["SQLite3_ROOT"] = str(self.external)
        self.run_command([
            "cmake", "-S", str(ENTRYPOINT), "-B", str(build),
            "-DCMAKE_BUILD_TYPE=Debug", "-DBUILD_TESTING=OFF",
            "-DCMAKE_PROJECT_INCLUDE=" + str(self.fixture / "hook.cmake"),
            "-DSQLITE_PROBE_FIXTURE=" + str(self.fixture),
            "-DSQLITE_PROBE_EXPECT_SYSTEM=" + ("ON" if negative_control else "OFF"),
            "-DCMAKE_FRAMEWORK_PATH=" + str(self.fixture / "Frameworks"),
            "-DCMAKE_FIND_FRAMEWORK=FIRST", "-DCMAKE_DISABLE_FIND_PACKAGE_PkgConfig=TRUE",
            "-DCMAKE_DISABLE_FIND_PACKAGE_SQLite3=FALSE",
            "-DFETCHCONTENT_BASE_DIR=" + str(self.fixture / "dependencies"),
        ], env=environment)

    def check_probe(self, build):
        self.run_command(["cmake", "--build", str(build), "--target", "sqlite_probe", "--parallel", "2"])
        self.assertIn("persistent preparation and changes64 passed",
                      self.run_command([str(build / "sqlite_probe")]))
        dependencies = self.run_command(["otool", "-L", str(build / "sqlite_probe")])
        self.assertNotIn("sqlite", "\n".join(dependencies.splitlines()[1:]).lower())

    def test_fresh_build_ignores_framework_and_external_root(self):
        build = self.fixture / "fresh"
        self.configure(build)
        self.check_probe(build)

    def test_reconfigure_replaces_cached_header_and_library(self):
        build = self.fixture / "cached"
        self.configure(build, negative_control=True)
        cache = (build / "CMakeCache.txt").read_text()
        self.assertIn("SQLite3_INCLUDE_DIR:PATH=" + str(self.headers) + "\n", cache)
        self.assertIn("SQLite3_LIBRARY:FILEPATH=" + str(self.library), cache)
        self.configure(build)
        self.check_probe(build)


if __name__ == "__main__":
    unittest.main()
