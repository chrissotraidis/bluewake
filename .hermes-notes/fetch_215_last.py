import json, urllib.request
url = "https://api.github.com/repos/chrissotraidis/bluewake/issues/215/comments?per_page=100"
with urllib.request.urlopen(url) as r:
    cs = json.load(r)
print(f"{len(cs)} comments total")
print("="*30, "LAST COMMENT", "="*30)
print(cs[-1]["user"]["login"])
print(cs[-1]["body"])
