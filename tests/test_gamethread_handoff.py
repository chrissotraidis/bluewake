from pathlib import Path
import json
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).parents[1] / "scripts"))
import qualify_gamethread
from gamethread_qualification.provenance import collect_provenance, deck_evidence_errors
from gamethread_qualification.report import add_outcome, new_report, read_report


class GameThreadHandoffTests(unittest.TestCase):
    def test_preflight_hashes_roles_without_publishing_paths(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            artifacts = {}
            for role in ("app", "candidate_module", "reference_module", "source_card"):
                path = root / f"private {role}"
                path.write_bytes(role.encode())
                artifacts[role] = str(path)
            manifest = root / "manifest.json"
            manifest.write_text(json.dumps({"artifacts": artifacts,
                                            "device": {"class": "steam-deck"}}))
            output = root / "report.json"
            self.assertEqual(qualify_gamethread.main(
                ["preflight", "--manifest", str(manifest), "--out", str(output)]), 3)
            report = read_report(output)
            encoded = json.dumps(report)
            self.assertNotIn(str(root), encoded)
            self.assertEqual(set(report["provenance"]["artifacts"]), set(artifacts))
            self.assertEqual(report["status"], "incomplete")

    def test_finalization_requires_every_gate_and_deck_identity(self):
        report = new_report(provenance={"declared_device": {"class": "steam-deck"},
                                        "missing_artifacts": []})
        names = ("source-only", "private-oracle", "repeatability", "rendered",
                 "save-reload", "soak")
        for name in names:
            add_outcome(report, name, "pass")
        self.assertEqual(deck_evidence_errors(report), [])
        report["outcomes"].pop()
        self.assertTrue(deck_evidence_errors(report))

    def test_missing_private_artifacts_are_named_by_role_only(self):
        manifest = {"artifacts": {role: "/does/not/exist" for role in
                                  ("app", "candidate_module", "reference_module",
                                   "source_card")}}
        result = collect_provenance(manifest)
        self.assertEqual(len(result["missing_artifacts"]), 4)
        self.assertNotIn("/does/not/exist", json.dumps(result))


if __name__ == "__main__":
    unittest.main()
