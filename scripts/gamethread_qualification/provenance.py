import hashlib
import json
import platform
from pathlib import Path


REQUIRED_ARTIFACTS = ("app", "candidate_module", "reference_module", "source_card")


def file_identity(path):
    source = Path(path)
    digest = hashlib.sha256()
    with source.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return {"sha256": digest.hexdigest(), "bytes": source.stat().st_size}


def read_manifest(path):
    with Path(path).open(encoding="utf-8") as stream:
        manifest = json.load(stream)
    if not isinstance(manifest, dict):
        raise ValueError("manifest must be an object")
    artifacts = manifest.get("artifacts")
    if not isinstance(artifacts, dict):
        raise ValueError("manifest artifacts must be an object")
    missing = [name for name in REQUIRED_ARTIFACTS if not artifacts.get(name)]
    if missing:
        raise ValueError("manifest is missing artifacts: " + ", ".join(missing))
    return manifest


def collect_provenance(manifest):
    identities = {}
    missing = []
    for role in REQUIRED_ARTIFACTS:
        try:
            identities[role] = file_identity(manifest["artifacts"][role])
        except OSError:
            missing.append(role)
    device = manifest.get("device", {})
    return {
        "artifacts": identities,
        "missing_artifacts": missing,
        # Record declared device facts separately from automatically observed
        # host facts; a desktop preflight must not masquerade as a Deck run.
        "declared_device": device,
        "collection_host": {
            "system": platform.system(), "machine": platform.machine(),
            "release": platform.release(),
        },
    }


def deck_evidence_errors(report):
    required = {"source-only", "private-oracle", "repeatability", "rendered",
                "save-reload", "soak"}
    outcomes = report.get("outcomes", [])
    names = {item.get("name") for item in outcomes if item.get("status") == "pass"}
    errors = [f"missing passing evidence: {name}" for name in sorted(required - names)]
    device = report.get("provenance", {}).get("declared_device", {})
    if device.get("class") != "steam-deck":
        errors.append("device class is not steam-deck")
    if report.get("provenance", {}).get("missing_artifacts"):
        errors.append("one or more required artifacts are missing")
    return errors
