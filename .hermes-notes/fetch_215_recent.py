import json, urllib.request
url = "https://api.github.com/repos/chrissotraidis/bluewake/issues/215/comments?per_page=100"
cs = json.load(urllib.request.urlopen(url))
print(f"{len(cs)} comments total; showing last 5:\n")
for c in cs[-5:]:
    print("="*20, c["user"]["login"], "="*20)
    print(c["body"][:2500])
    print()
