#!/usr/bin/env bash
# BlueWake repository safety audit (P0)
set -euo pipefail

fail=0
tracked=$(git ls-files)

echo "=== BlueWake repository audit ==="

if ! python3 - <<'PY_AUDIT'
import fnmatch
import subprocess
import sys
paths = subprocess.check_output(['git', 'ls-files', '-z']).decode().split('\0')
patterns = ('*.iso', '*.gcm', '*.rvz', '*.nfs', '*.wbfs', '*.wia', '*.ciso',
            '*.gcz', '*.dol', '*.rel', '*.sav', '*.gci', '*.card', '*.raw',
            '*.p12', '*.mobileprovision', '*.provisionprofile', 'dolphin_*.bin',
            '*.ipa', '*.profraw', '*.profdata', '*.dylib', '*.dll', '*.exe')
reviewed_profiles = {'scripts/builder/profiles/bluewake/composite-rt.profdata',
                     'scripts/builder/profiles/bluewake/host.profdata'}
forbidden_dirs = ('ref/', 'local-research/', 'generated/', 'build/', 'route_b/', 'patches/tww/')
bad = [p for p in paths if p and (p.startswith(forbidden_dirs) or
       (p not in reviewed_profiles and
        any(fnmatch.fnmatch(p.lower().rsplit('/', 1)[-1], pattern) for pattern in patterns)))]
for path in bad:
    print('FAIL: tracked private/generated file:', path)
sys.exit(bool(bad))
PY_AUDIT
then
  fail=1
fi

if [ ! -f config/dependencies.lock.json ]; then
  echo "FAIL: config/dependencies.lock.json missing"; fail=1
elif ! python3 -c "import json; json.load(open('config/dependencies.lock.json'))" 2>/dev/null; then
  echo "FAIL: config/dependencies.lock.json is not valid JSON"; fail=1
else
  echo "OK: dependency lock is valid JSON"
fi

for f in docs/status/CURRENT.md docs/status/GATES.md docs/status/BLOCKERS.md docs/status/DECISIONS.md docs/status/COMPATIBILITY.md docs/status/PERFORMANCE.md docs/status/RELEASE.md tests/coverage/catalog.json; do
  if [ ! -f "$f" ]; then echo "FAIL: required ledger $f missing"; fail=1; fi
done

if [ "$fail" -eq 0 ]; then echo "AUDIT PASS"; else echo "AUDIT FAIL"; exit 1; fi
