#!/bin/bash
curl -s "https://api.github.com/repos/chrissotraidis/bluewake/issues/215/comments?per_page=100" > /tmp/c215.json
python3 - <<'PY'
import json
cs = json.load(open("/tmp/c215.json"))
print(f"{len(cs)} comments")
for c in cs:
    print("="*20, c["user"]["login"], "="*20)
    print(c["body"][:3000])
    print()
PY
