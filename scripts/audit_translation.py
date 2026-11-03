#!/usr/bin/env python3
"""P2 translation audit for BlueWake.

Scans DolRecomp generated output (DOL + RELs) and reports:
- per-image chunk/function-label counts
- total instruction-emission sites
- unknown/unsupported markers
- SMC patch warning count

Usage: python3 scripts/audit_translation.py [--json]
Requires generated/full/out_dol and generated/full/out_rels from prepare.py.
"""

import argparse
import json
import re
import sys
from pathlib import Path

LABEL_RE = re.compile(r"^label_[0-9A-Fa-f]+:", re.M)
OP_RE = re.compile(r"ctx->(?:gpr|fpr|cr|lr|ctr|msr|xer|dar|dsisr|srr)\w*\s*(?:=|\|=)")
UNKNOWN_RE = re.compile(r"UNKNOWN|unknown_instruction|recomp_unsupported", re.I)


def scan_dir(chunk_dir):
    labels = ops = unknown = 0
    chunks = 0
    if not chunk_dir.is_dir():
        return None
    for f in sorted(chunk_dir.glob("*.c")):
        src = f.read_text(errors="replace")
        labels += len(LABEL_RE.findall(src))
        ops += len(OP_RE.findall(src))
        unknown += len(UNKNOWN_RE.findall(src))
        chunks += 1
    return {"chunks": chunks, "labels": labels, "ops": ops, "unknown": unknown}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args()
    base = Path("generated/full")
    result = {"dol": None, "rels": {"modules": 0, "totals": {}}, "pass": False}
    dol = scan_dir(base / "out_dol/generated/chunks")
    smc_file = base / "out_dol/generated/generated_smc.txt"
    smc_count = 0
    if smc_file.exists():
        smc_count = sum(1 for l in smc_file.read_text().splitlines() if l.startswith("0x"))
    if dol:
        dol["smc_warnings"] = smc_count
        result["dol"] = dol
    rel_base = base / "out_rels/generated/rels"
    if rel_base.is_dir():
        total_labels = total_ops = total_unknown = total_chunks = 0
        module_count = 0
        all_complete = True
        for m in sorted(rel_base.iterdir()):
            r = scan_dir(m / "chunks")
            if r is None:
                all_complete = False
                continue
            module_count += 1
            total_labels += r["labels"]
            total_ops += r["ops"]
            total_unknown += r["unknown"]
            total_chunks += r["chunks"]
        result["rels"]["modules"] = module_count
        result["rels"]["all_have_chunks"] = all_complete
        result["rels"]["totals"] = {
            "labels": total_labels,
            "ops": total_ops,
            "unknown": total_unknown,
            "chunks": total_chunks,
        }
    ok = (
        result["dol"] is not None
        and result["dol"]["chunks"] > 0
        and result["dol"]["unknown"] == 0
        and result["rels"].get("modules", 0) == 415
        and result["rels"].get("all_have_chunks", False)
        and result["rels"]["totals"].get("unknown", -1) == 0
    )
    result["pass"] = ok
    if args.json:
        print(json.dumps(result, indent=2))
    else:
        d = result["dol"]
        t = result["rels"].get("totals", {})
        print(f"DOL: {d['chunks']} chunks, {d['labels']:,} labels, {d['ops']:,} ops, {d['unknown']} unknown, {d['smc_warnings']} SMC warnings")
        print(f"REL: {result['rels']['modules']} modules, {t.get('chunks',0):,} chunks, {t.get('labels',0):,} labels, {t.get('ops',0):,} ops, {t.get('unknown',0)} unknown")
        print(f"Result: {'PASS' if ok else 'FAIL'}")
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
