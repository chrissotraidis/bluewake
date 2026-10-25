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
                     'scripts/builder/profiles/bluewake/host.profdata',
                     # The Windows app's own code (host, Aurora, libraries); no game functions.
                     'windows/pgo/app.profdata'}
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

# The active plan: what is being done, in what order, and how speed is measured.
for f in docs/GOAL_LOOP.md docs/PRIORITIES.md docs/PERFORMANCE.md docs/DIRECTION.md docs/WINDOWS_TASKS.md tests/coverage/catalog.json; do
  if [ ! -f "$f" ]; then echo "FAIL: required file $f missing"; fail=1; fi
done

# Merge conflict markers left in tracked text files (a merge committed half-done).
markers=$(git ls-files -z -- '*.md' '*.c' '*.h' '*.cpp' '*.hpp' '*.m' '*.mm' '*.py' '*.sh' '*.json' '*.txt' '*.yml' '*.cmake' \
  | xargs -0 grep -l -E '^(<<<<<<< |>>>>>>> )' 2>/dev/null || true)
if [ -n "$markers" ]; then
  echo "FAIL: merge conflict markers in:"; echo "$markers" | sed 's/^/  /'; fail=1
else
  echo "OK: no merge conflict markers"
fi

if [ "$fail" -eq 0 ]; then echo "AUDIT PASS"; else echo "AUDIT FAIL"; exit 1; fi
