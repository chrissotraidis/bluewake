import json, urllib.request
d = json.load(urllib.request.urlopen("https://api.github.com/repos/chrissotraidis/bluewake/pulls/290"))
print("merged_at:", d.get("merged_at"))
print("merge_commit_sha:", d.get("merge_commit_sha"))
print("head ref:", d.get("head", {}).get("ref"))
print("head sha:", d.get("head", {}).get("sha"))
print("base:", d.get("base", {}).get("ref"))
print("state:", d.get("state"))
