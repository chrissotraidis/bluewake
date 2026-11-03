#!/usr/bin/env python3
"""Reject commits that credit a bot or AI tool as an author or co-author.

Usage: python3 scripts/check_attribution.py [REVISION_RANGE]
With no range, every commit reachable from HEAD is checked.
"""
import re
import subprocess
import sys

BOT_NAME = re.compile(
    r"\[bot\]|\b(claude|anthropic|openai|chatgpt|codex|copilot|gemini|devin|aider|cursor ?agent)\b",
    re.IGNORECASE,
)
BOT_EMAIL = re.compile(
    r"(noreply@anthropic\.com|noreply@openai\.com|cursoragent@cursor\.com"
    r"|\[bot\]@users\.noreply\.github\.com|\+copilot@users\.noreply\.github\.com)",
    re.IGNORECASE,
)
CO_AUTHOR = re.compile(r"^\s*co-authored-by:\s*(.*?)\s*<([^>]*)>", re.IGNORECASE | re.MULTILINE)
MARKERS = re.compile(
    r"^\s*(claude-session:|.*generated (with|by) \[?(claude|codex|copilot|chatgpt|cursor|gemini|devin|aider))",
    re.IGNORECASE | re.MULTILINE,
)


def is_bot(name, email):
    return bool(BOT_NAME.search(name) or BOT_EMAIL.search(email))


def main():
    rev = sys.argv[1] if len(sys.argv) > 1 else "HEAD"
    log = subprocess.run(
        ["git", "log", "--format=%H%x1f%an%x1f%ae%x1f%B%x1e", rev],
        check=True, capture_output=True, text=True,
    ).stdout
    problems = []
    for entry in filter(str.strip, log.split("\x1e")):
        sha, name, email, body = entry.strip("\n").split("\x1f", 3)
        found = []
        if is_bot(name, email):
            found.append(f"author {name} <{email}>")
        found += [f"co-author {n} <{e}>" for n, e in CO_AUTHOR.findall(body) if is_bot(n, e)]
        found += [f"attribution line: {m.group(0).strip()}" for m in MARKERS.finditer(body)]
        problems += [f"{sha[:10]} {item}" for item in found]
    if problems:
        print("Bot or AI attribution found. Remove these credits from the commit messages:")
        print("\n".join(problems))
        return 1
    print("No bot or AI contributor attribution found.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
