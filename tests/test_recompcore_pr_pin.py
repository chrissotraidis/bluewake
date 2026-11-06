import json
import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PROFILE = ROOT / "scripts/builder/profiles/bluewake.sh"


def profile_values():
    values = {}
    for line in PROFILE.read_text().splitlines():
        match = re.fullmatch(r"(RECOMPCORE_(?:URL|REF|SHA))=(.+)", line)
        if match:
            values[match.group(1)] = match.group(2)
    return values


class RecompCorePullRequestPinTest(unittest.TestCase):
    def test_profile_and_lock_pin_the_same_upstream_draft_and_sha(self):
        values = profile_values()
        lock = json.loads((ROOT / "config/dependencies.lock.json").read_text())
        runtime = next(item for item in lock["dependencies"] if item["id"] == "recompcore")

        self.assertEqual(values["RECOMPCORE_URL"].removesuffix(".git"), runtime["url"])
        self.assertEqual(values["RECOMPCORE_REF"], runtime["ref"])
        self.assertEqual(values["RECOMPCORE_SHA"], runtime["sha"])
        self.assertRegex(runtime["ref"], r"^refs/pull/[0-9]+/head$")
        self.assertRegex(runtime["sha"], r"^[0-9a-f]{40}$")
        self.assertEqual(runtime["pull_request_base"]["pull_request"], int(runtime["ref"].split("/")[2]))

    def test_every_fetch_recipe_uses_the_pr_ref_and_checks_the_sha(self):
        linux = (ROOT / "scripts/linux/build.py").read_text()
        windows = (ROOT / "scripts/windows/build.py").read_text()
        shell = PROFILE.read_text()
        linux_ci = (ROOT / ".github/workflows/linux-host.yml").read_text()
        windows_ci = (ROOT / ".github/workflows/windows-host.yml").read_text()

        self.assertIn('profile_value("RECOMPCORE_REF")', linux)
        self.assertIn('"bluewake", ref', linux)
        self.assertIn('profile_value("RECOMPCORE_REF")', windows)
        self.assertIn('"bluewake", ref', windows)
        self.assertIn('bluewake "$RECOMPCORE_REF"', shell)
        self.assertIn('rev-parse HEAD)" = "$RECOMPCORE_SHA"', shell)
        self.assertIn('"$url" "$ref"', linux_ci)
        self.assertIn('rev-parse HEAD)" = "$sha"', linux_ci)
        self.assertIn('$runtime.url $runtime.ref', windows_ci)
        self.assertIn("Runtime ref does not match pinned SHA", windows_ci)


if __name__ == "__main__":
    unittest.main()
