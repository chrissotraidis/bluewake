import json
import os
from pathlib import Path

SCHEMA_VERSION = 1
STATUSES = {"pass", "failure", "incomplete"}


def new_report(provenance=None, configuration=None):
    return {
        "schema_version": SCHEMA_VERSION,
        "provenance": provenance or {},
        "configuration": configuration or {},
        "correctness": {},
        "coverage": {},
        "metrics": {},
        "capabilities": {},
        "outcomes": [],
        "status": "incomplete",
    }


def add_outcome(report, name, status, detail=""):
    if status not in STATUSES:
        raise ValueError(f"invalid outcome status: {status}")
    report["outcomes"].append({"name": name, "status": status, "detail": detail})
    statuses = {item["status"] for item in report["outcomes"]}
    report["status"] = ("failure" if "failure" in statuses else
                        "incomplete" if "incomplete" in statuses else "pass")


def validate_report(report):
    if not isinstance(report, dict) or report.get("schema_version") != SCHEMA_VERSION:
        raise ValueError("unsupported or missing report schema_version")
    if not isinstance(report.get("outcomes"), list):
        raise ValueError("report outcomes must be a list")
    expected = new_report().keys()
    missing = [key for key in expected if key not in report]
    if missing:
        raise ValueError("report is missing fields: " + ", ".join(missing))
    for item in report["outcomes"]:
        if not isinstance(item, dict) or item.get("status") not in STATUSES:
            raise ValueError("report contains an invalid outcome")
    statuses = {item["status"] for item in report["outcomes"]}
    aggregate = ("failure" if "failure" in statuses else
                 "incomplete" if "incomplete" in statuses or not statuses else "pass")
    if report.get("status") != aggregate:
        raise ValueError("report status does not match its outcomes")
    return report


def read_report(path):
    with Path(path).open(encoding="utf-8") as stream:
        return validate_report(json.load(stream))


def write_report(path, report):
    validate_report(report)
    destination = Path(path)
    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = destination.with_name(destination.name + f".tmp.{os.getpid()}")
    try:
        with temporary.open("w", encoding="utf-8") as stream:
            json.dump(report, stream, indent=2, sort_keys=True)
            stream.write("\n")
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, destination)
    finally:
        temporary.unlink(missing_ok=True)
