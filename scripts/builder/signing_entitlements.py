#!/usr/bin/env python3
"""Validate a profile and derive signing entitlements without changing the profile."""
import copy
import fnmatch
import plistlib
import sys


def signing_entitlements(profile, bundle_id, platform):
    if platform not in ("ios", "tvos"):
        raise ValueError("platform must be ios or tvos")
    prefixes = profile.get("ApplicationIdentifierPrefix", [])
    if not prefixes:
        raise ValueError("the provisioning profile has no application identifier prefix")
    prefix = prefixes[0]
    entitlements = copy.deepcopy(profile["Entitlements"])
    app_id = entitlements.get("application-identifier", "")
    expected = f"{prefix}.{bundle_id}"
    pattern = app_id.removeprefix(prefix + ".")
    if not app_id.startswith(prefix + ".") or not fnmatch.fnmatchcase(bundle_id, pattern):
        raise ValueError(f"the profile is for {app_id}, not {expected}")
    # Preserve iOS's existing wildcard identity so in-place upgrades keep saves.
    # tvOS needs a concrete identifier to access its app sandbox.
    if platform == "tvos":
        entitlements["application-identifier"] = expected
        if "keychain-access-groups" in entitlements:
            entitlements["keychain-access-groups"] = [
                expected if group == f"{prefix}.*" else group
                for group in entitlements["keychain-access-groups"]
            ]
    return entitlements


def main():
    if len(sys.argv) != 5:
        sys.exit("usage: signing_entitlements.py PROFILE.plist OUT.plist BUNDLE_ID ios|tvos")
    profile_path, output_path, bundle_id, platform = sys.argv[1:]
    try:
        with open(profile_path, "rb") as source:
            result = signing_entitlements(plistlib.load(source), bundle_id, platform)
    except (ValueError, KeyError) as error:
        sys.exit(str(error))
    with open(output_path, "wb") as output:
        plistlib.dump(result, output, sort_keys=True)


if __name__ == "__main__":
    main()
