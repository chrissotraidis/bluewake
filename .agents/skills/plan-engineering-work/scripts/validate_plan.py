#!/usr/bin/env python3
"""Validate planning structure and local references; never infer execution truth."""
from __future__ import annotations

import argparse
from collections import Counter
from pathlib import Path
import re
import sys
import unicodedata
from urllib.parse import unquote, urlsplit

FILES = ("PRD.md", "TECH_PLAN.md", "PROGRESS.md")
FR = re.compile(r"\bFR-\d+\b")
MILESTONE = re.compile(r"^### (M\d+): (.*)$")
TASK = re.compile(r"^- \[([ xX!])\] (M\d+-T\d+): (.+)$")
LINK = re.compile(r"(?<!!)\[[^\]]+\]\(([^\s)]+)\)")
INLINE_EVIDENCE = re.compile(r"\bevidence\s*:\s*(.*)$", re.I)
EVIDENCE_FIELDS = (
    ("commit / artifact", "artifact"),
    ("command / method", "method"),
    ("result", "result"),
)


def prose(text: str) -> tuple[str, bool]:
    lines = []
    fence = None
    for line in text.splitlines():
        match = re.match(r"^\s*(\x60{3,}|~{3,})", line)
        if match:
            marker = match.group(1)
            if fence is None:
                fence = marker
            elif marker[0] == fence[0] and len(marker) >= len(fence):
                fence = None
            continue
        if fence is None:
            lines.append(line)
    return "\n".join(lines), fence is None


def anchors(text: str) -> set[str]:
    content, _ = prose(text)
    result, counts = set(), Counter()
    for line in content.splitlines():
        match = re.match(r"^#{1,6}\s+(.+?)(?:\s+#+)?$", line)
        if not match:
            continue
        title = re.sub(r"\[([^\]]+)\]\([^)]+\)", r"\1", match.group(1))
        title = re.sub(r"<[^>]+>", "", title).lower()
        base = "".join(c for c in title if c in " _-" or
                       unicodedata.category(c)[0] in ("L", "N")).replace(" ", "-")
        count = counts[base]
        counts[base] += 1
        result.add(base if count == 0 else f"{base}-{count}")
    return result


def cells(line: str) -> list[str]:
    return [cell.strip() for cell in line.strip().strip("|").split("|")]


def duplicates(values: list[str]) -> list[str]:
    return [key for key, count in Counter(values).items() if count > 1]


def evidence_rows(content: str) -> dict[str, dict[str, str]]:
    """Return structural evidence fields keyed by completed task ID."""
    result = {}
    for line in content.splitlines():
        if not line.startswith("|"):
            continue
        row = cells(line)
        if not row or not re.fullmatch(r"M\d+-T\d+", row[0]):
            continue
        result[row[0]] = {
            "artifact": row[1].strip() if len(row) > 1 else "",
            "method": row[2].strip() if len(row) > 2 else "",
            "result": row[4].strip() if len(row) > 4 else "",
        }
    return result


def inline_evidence(body: str) -> dict[str, str] | None:
    """Parse `evidence: artifact=...; method=...; result=...` task metadata."""
    match = INLINE_EVIDENCE.search(body)
    if not match:
        return None
    aliases = {
        "artifact": "artifact",
        "commit": "artifact",
        "commit / artifact": "artifact",
        "method": "method",
        "command": "method",
        "command / method": "method",
        "result": "result",
    }
    result = {"artifact": "", "method": "", "result": ""}
    for part in match.group(1).split(";"):
        field = re.match(r"\s*([^=:]+?)\s*[=:]\s*(.*?)\s*$", part)
        if field and field.group(1).strip().lower() in aliases:
            result[aliases[field.group(1).strip().lower()]] = field.group(2).strip()
    return result


def missing_evidence_fields(entry: dict[str, str]) -> list[str]:
    return [label for label, key in EVIDENCE_FIELDS if not entry.get(key, "").strip()]


def validate(directory: Path) -> tuple[list[str], list[str]]:
    errors, warnings = [], []
    docs, plain = {}, {}
    for name in FILES:
        path = directory / name
        try:
            docs[name] = path.read_text(encoding="utf-8")
        except (OSError, UnicodeError) as exc:
            errors.append(f"{name}: cannot read: {exc}")
            continue
        plain[name], closed = prose(docs[name])
        if not closed:
            errors.append(f"{name}: unclosed fenced block")
        if not plain[name].strip():
            errors.append(f"{name}: no document prose")
        if re.search(r"\b(?:TODO|TBD)\b|\[Feature Name\]|<Feature>", plain[name]):
            errors.append(f"{name}: unresolved template placeholder")
    if len(docs) != len(FILES):
        return errors, warnings

    definitions = re.findall(r"^#{2,4}\s+(FR-\d+)\b", plain["PRD.md"], re.M)
    requirements = set(definitions)
    if not requirements:
        errors.append("PRD.md: no FR requirement headings")
    for item in duplicates(definitions):
        errors.append(f"PRD.md: duplicate {item}")
    for name in ("TECH_PLAN.md", "PROGRESS.md"):
        refs = set(FR.findall(plain[name]))
        for item in sorted(refs - requirements):
            errors.append(f"{name}: undefined requirement {item}")
        for item in sorted(requirements - refs):
            errors.append(f"{name}: missing requirement coverage {item}")

    headings, tasks, dependencies = {}, {}, {}
    task_requirements = set()
    current = None
    for line in plain["PROGRESS.md"].splitlines():
        match = MILESTONE.match(line)
        if match:
            current, title = match.groups()
            if current in headings:
                errors.append(f"PROGRESS.md: duplicate milestone {current}")
            headings[current] = title
            deps = re.search(r"\(depends:\s*([^)]*)\)", title)
            dependencies[current] = set(re.findall(r"M\d+", deps.group(1))) if deps else set()
            continue
        if re.match(r"^#{1,2}\s", line):
            current = None
        match = TASK.match(line)
        if not match:
            if re.match(r"^## M\d+:", line):
                errors.append("PROGRESS.md: milestone headings must use ###")
            continue
        state, task_id, body = match.groups()
        if task_id in tasks:
            errors.append(f"PROGRESS.md: duplicate task {task_id}")
        owner = task_id.split("-")[0]
        if current != owner:
            errors.append(f"PROGRESS.md: {task_id} outside owning milestone")
        refs = set(FR.findall(body))
        if not refs:
            errors.append(f"PROGRESS.md: {task_id} has no FR reference")
        task_requirements |= refs
        tasks[task_id] = (owner, state, body)

    if not headings or not tasks:
        errors.append("PROGRESS.md: missing milestones/tasks")
    for item in sorted(requirements - task_requirements):
        errors.append(f"PROGRESS.md: no task implements {item}")
    for owner in headings:
        count = sum(1 for task in tasks.values() if task[0] == owner)
        if not 3 <= count <= 5:
            warnings.append(f"{owner}: {count} tasks; target 3-5, soft ceiling 6")
        if count > 6:
            warnings.append(f"{owner}: exceeds milestone soft ceiling; split or document exception")
        for dep in dependencies[owner]:
            if dep not in headings:
                errors.append(f"{owner}: unknown dependency {dep}")
            if dep == owner:
                errors.append(f"{owner}: depends on itself")

    plan_dependencies = {}
    for line in plain["TECH_PLAN.md"].splitlines():
        if not line.startswith("|"):
            continue
        row = cells(line)
        if len(row) >= 2 and re.fullmatch(r"M\d+", row[0]):
            if row[0] in plan_dependencies:
                errors.append(f"TECH_PLAN.md: duplicate implementation-order row {row[0]}")
            plan_dependencies[row[0]] = set(re.findall(r"M\d+", row[1]))
    if set(plan_dependencies) != set(headings):
        errors.append("TECH_PLAN/PROGRESS: milestone sets differ")
    for owner in set(plan_dependencies) & set(dependencies):
        if plan_dependencies[owner] != dependencies[owner]:
            errors.append(f"{owner}: dependency declarations differ")

    visiting, visited = set(), set()
    def visit(node: str) -> None:
        if node in visiting:
            errors.append(f"PROGRESS.md: dependency cycle at {node}")
            return
        if node in visited:
            return
        visiting.add(node)
        for dep in dependencies.get(node, set()):
            if dep in headings:
                visit(dep)
        visiting.remove(node)
        visited.add(node)
    for owner in headings:
        visit(owner)

    ledger = evidence_rows(plain["PROGRESS.md"])
    for task_id, (_, state, body) in tasks.items():
        if state.lower() == "x":
            entry = ledger.get(task_id)
            source = "evidence row"
            if entry is None:
                entry = inline_evidence(body)
                source = "inline evidence"
            if entry is None:
                errors.append(
                    f"{task_id}: completed without evidence; add a populated evidence row "
                    "or structured inline evidence"
                )
            else:
                missing = missing_evidence_fields(entry)
                if missing:
                    errors.append(f"{task_id}: {source} missing {', '.join(missing)}")
        if state == "!" and not re.search(r"blocked|depends|waiting|reason", body, re.I):
            errors.append(f"{task_id}: blocked without reason")

    remote = 0
    for name, content in plain.items():
        for match in LINK.finditer(content):
            target = match.group(1)
            parts = urlsplit(target)
            if parts.scheme or parts.netloc:
                remote += 1
                continue
            path = (directory / unquote(parts.path)).resolve() if parts.path else directory / name
            if not path.is_file():
                errors.append(f"{name}: broken local link {target}")
                continue
            if parts.fragment and path.suffix.lower() == ".md":
                try:
                    if unquote(parts.fragment) not in anchors(path.read_text(encoding="utf-8")):
                        errors.append(f"{name}: missing local anchor {target}")
                except (OSError, UnicodeError) as exc:
                    errors.append(f"{name}: cannot inspect link {target}: {exc}")
    if remote:
        warnings.append(f"{remote} remote references require provider/manual verification; no network checks performed")
    warnings.append("Structural checks do not prove source accuracy, acceptance completeness, or real execution.")
    return errors, warnings


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("planning_dir", type=Path)
    args = parser.parse_args()
    errors, warnings = validate(args.planning_dir.resolve())
    for warning in warnings:
        print(f"NOTE: {warning}")
    for error in errors:
        print(f"ERROR: {error}")
    print("PASS: structural checks" if not errors else f"FAIL: {len(errors)} structural error(s)")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
