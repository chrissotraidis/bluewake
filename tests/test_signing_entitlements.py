#!/usr/bin/env python3
"""Synthetic profiles only: never uses signing identities or a physical device."""
import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts" / "builder"))
from signing_entitlements import signing_entitlements


class SigningTests(unittest.TestCase):
    def setUp(self):
        self.profile = {
            "ApplicationIdentifierPrefix": ["TESTTEAM"],
            "Entitlements": {
                "application-identifier": "TESTTEAM.*",
                "keychain-access-groups": ["TESTTEAM.*", "TESTTEAM.shared"],
                "get-task-allow": True,
                "unrelated": {"nested": [1, 2]},
            },
        }

    def test_ios_preserves_wildcard_and_all_other_entitlements(self):
        self.assertEqual(signing_entitlements(self.profile, "dev.bluewake.BlueWake", "ios"),
                         self.profile["Entitlements"])

    def test_tvos_concrete_id_preserves_shared_groups_and_profile(self):
        before = copy.deepcopy(self.profile)
        result = signing_entitlements(self.profile, "dev.bluewake.BlueWake", "tvos")
        self.assertEqual(result["application-identifier"], "TESTTEAM.dev.bluewake.BlueWake")
        self.assertEqual(result["keychain-access-groups"],
                         ["TESTTEAM.dev.bluewake.BlueWake", "TESTTEAM.shared"])
        self.assertEqual(result["unrelated"], before["Entitlements"]["unrelated"])
        self.assertEqual(self.profile, before)

    def test_exact_id_is_unchanged_on_both_platforms(self):
        self.profile["Entitlements"]["application-identifier"] = "TESTTEAM.dev.bluewake.BlueWake"
        for platform in ("ios", "tvos"):
            self.assertEqual(signing_entitlements(self.profile, "dev.bluewake.BlueWake", platform)
                             ["application-identifier"], "TESTTEAM.dev.bluewake.BlueWake")

    def test_mismatch_rejected(self):
        for app_id in ("OTHERTEAM.*", "TESTTEAM.dev.other.App", "TESTTEAM.dev.bluewake.Other"):
            self.profile["Entitlements"]["application-identifier"] = app_id
            with self.assertRaises(ValueError):
                signing_entitlements(self.profile, "dev.bluewake.BlueWake", "tvos")

    def test_missing_prefix_rejected(self):
        self.profile.pop("ApplicationIdentifierPrefix")
        with self.assertRaises(ValueError):
            signing_entitlements(self.profile, "dev.bluewake.BlueWake", "ios")

    def test_missing_groups_not_invented(self):
        self.profile["Entitlements"].pop("keychain-access-groups")
        self.assertNotIn("keychain-access-groups",
                         signing_entitlements(self.profile, "dev.bluewake.BlueWake", "tvos"))

    def test_invalid_platform_rejected(self):
        with self.assertRaises(ValueError):
            signing_entitlements(self.profile, "dev.bluewake.BlueWake", "macos")


if __name__ == "__main__":
    unittest.main()
