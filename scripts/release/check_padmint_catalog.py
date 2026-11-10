#!/usr/bin/env python3
"""Check that PadMint offers BlueWake on every computer the recipe builds it on, and nowhere else.

Usage: python3 scripts/release/check_padmint_catalog.py [--recipe padmint.json] [--catalog bluewake.json]

PadMint takes the build from BlueWake's published padmint.json, but its menu lists a game only on the
hosts in its own catalog entry (PadMint's catalog/bluewake.json). When a release changes the recipe's
hosts, the catalog has to change with it, or players see BlueWake dimmed (0.11.0 added Windows, and
the catalog still said Mac only). By default this reads this checkout's padmint.json and the catalog
on PadMint's main branch. Exit status 1 when they disagree.
"""
import argparse
import json
import sys
import urllib.request
from pathlib import Path

CATALOG_URL = "https://raw.githubusercontent.com/chrissotraidis/padmint/main/catalog/bluewake.json"
RUNNABLE = {"verified", "experimental"}  # PadMint's RUNNABLE_STATES (padmint/manifest.py)
OFF_MAC = ("windows-x86_64", "windows-arm64", "linux-x86_64", "linux-arm64")


def catalog_ios_hosts(entry):
    """The hosts PadMint's menu offers the iPhone app on (padmint.manifest.ios_build_hosts)."""
    if "ios" not in entry.get("player_targets", []):
        return set()
    if "ios_hosts" in entry:
        return set(entry["ios_hosts"])
    hosts = {"macos-arm64"}
    if entry.get("ios_intel_mac"):
        hosts.add("macos-x86_64")
    if entry.get("ios_off_mac"):
        hosts.update(OFF_MAC)
    return hosts


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--recipe", type=Path, default=Path(__file__).resolve().parents[2] / "padmint.json")
    parser.add_argument("--catalog", help="a catalog/bluewake.json file (default: PadMint's main branch)")
    args = parser.parse_args()
    recipe = json.loads(args.recipe.read_text())
    if args.catalog:
        entry = json.loads(Path(args.catalog).read_text())
    else:
        with urllib.request.urlopen(CATALOG_URL, timeout=30) as response:
            entry = json.load(response)
    built = {host for host, state in recipe.get("targets", {}).get("ios", {}).get("hosts", {}).items()
             if state in RUNNABLE}
    offered = catalog_ios_hosts(entry)
    problems = [f"the recipe builds the iPhone app on {host}, but PadMint's menu hides it" for host in sorted(built - offered)]
    problems += [f"PadMint's menu offers the iPhone app on {host}, but the recipe can't build there" for host in sorted(offered - built)]
    print("recipe builds iPhone on: " + (", ".join(sorted(built)) or "nothing"))
    print("PadMint offers it on:    " + (", ".join(sorted(offered)) or "nothing"))
    for problem in problems:
        print("PROBLEM: " + problem)
    if problems:
        print("Change PadMint's catalog/bluewake.json (ios_hosts) in the same release, and release PadMint with it.")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())

