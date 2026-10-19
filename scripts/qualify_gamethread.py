#!/usr/bin/env python3
"""Public orchestration entry point; private game artifacts remain local."""

import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gamethread_qualification.load import staged_matrix
from gamethread_qualification.provenance import (
    collect_provenance, deck_evidence_errors, read_manifest,
)
from gamethread_qualification.report import add_outcome, new_report, read_report, write_report


def preflight(manifest_path, out):
    manifest = read_manifest(manifest_path)
    provenance = collect_provenance(manifest)
    report = new_report(provenance=provenance,
                        configuration=manifest.get("configuration", {}))
    if provenance["missing_artifacts"]:
        add_outcome(report, "preflight", "incomplete",
                    "missing roles: " + ", ".join(provenance["missing_artifacts"]))
    else:
        add_outcome(report, "preflight", "pass", "all local inputs hashed")
    # Preflight is never gameplay qualification by itself.
    add_outcome(report, "hardware-evidence", "incomplete", "Deck runs not collected")
    write_report(out, report)
    return 3


def finalize(report_path):
    report = read_report(report_path)
    errors = deck_evidence_errors(report)
    if errors:
        print("qualification incomplete: " + "; ".join(errors), file=sys.stderr)
        return 3
    print("Deck qualification evidence is complete")
    return 0


def main(argv=None):
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="command", required=True)
    prepare = sub.add_parser("preflight")
    prepare.add_argument("--manifest", required=True)
    prepare.add_argument("--out", required=True)
    sub.add_parser("matrix")
    finish = sub.add_parser("finalize")
    finish.add_argument("report")
    args = parser.parse_args(argv)
    try:
        if args.command == "preflight":
            return preflight(args.manifest, args.out)
        if args.command == "matrix":
            print(json.dumps(staged_matrix(), indent=2))
            return 0
        return finalize(args.report)
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"qualification: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
