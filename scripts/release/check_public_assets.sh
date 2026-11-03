#!/usr/bin/env bash
# Validate source archives and run the maintainer's external content gate.
# Personal game modules, disc images, saves and signing files may not be public.
# ZIP/TAR members are safely normalized without extraction; malformed archives,
# ambiguous paths and nonregular members fail closed. See the adjacent helper.
set -euo pipefail
exec python3 "$(cd "$(dirname "$0")" && pwd)/check_public_assets.py" "$@"
