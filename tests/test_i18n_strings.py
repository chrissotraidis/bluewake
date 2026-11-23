#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""The language catalogs and the code that reads them, checked against each other.

Every string the settings menu can show in another language is written twice:
in the source (the English a row falls back to) and in `languages/en.txt`
(what a translator starts from). This is what keeps the two the same file's
worth of keys:

  * every setting row's label, note, group and experimental line has a line in
    `languages/en.txt` saying exactly what the source says;
  * every `bw_lang::text`/`bw_lang::fill` key in the host sources has one too;
  * no key in `languages/en.txt` is one the code does not ask for, so a
    reworded row cannot leave a stale key behind;
  * every other catalog only names keys English has, and keeps each one's
    printf conversions (%s, %d) so a translated line cannot break the call
    that fills it in.

Run it with no arguments to check, and with `--write` to regenerate
`languages/en.txt` from the sources after changing a row.

The sources are read, never compiled: the check is seconds and runs in the
audit job (.github/workflows/audit.yml).
"""

import glob
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TABLE = os.path.join(ROOT, "runtime/host/src/settings_table.h")
CATALOGS = os.path.join(ROOT, "languages")
ENGLISH = os.path.join(CATALOGS, "en.txt")
SCANNED = ["runtime/host/src/menu_draw.h", "runtime/host/src/desktop_menu.cpp",
           "runtime/host/src/language.cpp", "runtime/host/src/texture_pack_menu.cpp"]

SPEC = re.compile(r"%[-+ #0-9.*]*[a-zA-Z]")


def fail(problems, message):
    problems.append(message)


def strip_code(text):
    """The source with its comments blanked, so a `//` inside a string is kept
    and a quoted string inside a comment is not read as code."""
    out = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c == '"':
            out.append(c)
            i += 1
            while i < n:
                if text[i] == "\\" and i + 1 < n:
                    out.append(text[i:i + 2])
                    i += 2
                    continue
                out.append(text[i])
                if text[i] == '"':
                    i += 1
                    break
                i += 1
            continue
        if c == "/" and i + 1 < n and text[i + 1] == "/":
            while i < n and text[i] != "\n":
                i += 1
            continue
        if c == "/" and i + 1 < n and text[i + 1] == "*":
            end = text.find("*/", i + 2)
            i = n if end < 0 else end + 2
            continue
        out.append(c)
        i += 1
    return "".join(out)


def read_string(code, pos):
    """One C string at `pos` (which must be a quote), escapes decoded."""
    assert code[pos] == '"', code[max(0, pos - 30):pos + 30]
    i, n = pos + 1, len(code)
    raw = []
    while i < n:
        c = code[i]
        if c == "\\" and i + 1 < n:
            e = code[i + 1]
            raw.append({"n": "\n", "t": "\t", "\\": "\\", '"': '"'}.get(e, "\\" + e))
            i += 2
            continue
        if c == '"':
            return "".join(raw), i + 1
        raw.append(c)
        i += 1
    raise AssertionError("unterminated string")


def read_value(code, pos):
    """A field's value at `pos`: `nullptr`, or C strings one after another."""
    while pos < len(code) and code[pos] in " \t\r\n":
        pos += 1
    if code.startswith("nullptr", pos):
        return None, pos + len("nullptr")
    if pos >= len(code) or code[pos] != '"':
        return None, pos
    text = []
    while True:
        piece, pos = read_string(code, pos)
        text.append(piece)
        while pos < len(code) and code[pos] in " \t\r\n":
            pos += 1
        if pos < len(code) and code[pos] == '"':
            continue
        return "".join(text), pos


def row_text(body, start):
    """One initializer: from its `{` to the `}` that closes it, strings skipped."""
    depth, i, n = 0, start, len(body)
    while i < n:
        c = body[i]
        if c == '"':
            i += 1
            while i < n:
                if body[i] == "\\" and i + 1 < n:
                    i += 2
                    continue
                if body[i] == '"':
                    i += 1
                    break
                i += 1
            continue
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return body[start:i + 1]
        i += 1
    return body[start:]


def settings_rows():
    """Every row of kSettings: its key and the four texts a catalog can carry."""
    code = strip_code(open(TABLE, encoding="utf-8").read())
    start = code.find("kSettings[] = {")
    if start < 0:
        raise SystemExit("settings_table.h: kSettings[] = { not found")
    body = code[start:]
    rows = []
    for match in re.finditer(r"\{\s*\.key\s*=\s*", body):
        row = row_text(body, match.start())
        key, _ = read_value(row, match.end() - match.start())
        fields = {"key": key}
        for field in ("label", "note", "group", "experimental"):
            found = re.search(r"\." + field + r"\s*=\s*", row)
            fields[field], _ = read_value(row, found.end()) if found is not None else (None, 0)
            if field == "group" and fields[field] == "":
                fields[field] = ""  # the plain separator: no line to translate
        rows.append(fields)
    return rows


def source_strings(problems):
    """Every key the host sources ask for, with the English they carry beside it."""
    found = {}
    pattern = re.compile(r"bw_lang::(text|fill)\(\s*\"")
    for path in SCANNED:
        full = os.path.join(ROOT, path)
        if not os.path.exists(full):
            continue
        code = strip_code(open(full, encoding="utf-8").read())
        for match in pattern.finditer(code):
            key, pos = read_value(code, match.end() - 1)
            while pos < len(code) and code[pos] in " \t\r\n":
                pos += 1
            if pos >= len(code) or code[pos] != ",":
                fail(problems, "%s: %s(\"%s\", ...) has no English beside it" % (path, match.group(1), key))
                continue
            english, _ = read_value(code, pos + 1)
            if english is None:
                fail(problems, "%s: %s(\"%s\", ...) has no English string" % (path, match.group(1), key))
                continue
            if key in found and found[key] != english:
                fail(problems, "%s: \"%s\" is asked for twice with different English" % (path, key))
            found[key] = english
    return found


def wanted(problems):
    """key -> English, from the table and the sources: everything the menu can say."""
    strings = {}
    for row in settings_rows():
        key = row["key"]
        for field in ("label", "note", "group", "experimental"):
            value = row[field]
            if value is None or value == "":
                continue
            strings["setting.%s.%s" % (key, field)] = value
    for key, english in source_strings(problems).items():
        strings[key] = english
    return strings


def escape(text):
    return text.replace("\\", "\\\\").replace("\n", "\\n").replace("\t", "\\t")


def read_catalog(path, problems):
    """key -> text, and the file's own `language.name` if it has one."""
    strings, name = {}, None
    for number, line in enumerate(open(path, encoding="utf-8").read().splitlines(), 1):
        text = line.strip()
        if not text or text[0] in "#;":
            continue
        if "=" not in text:
            fail(problems, "%s:%d: no `key = text`" % (path, number))
            continue
        key, value = text.split("=", 1)
        key, value = key.strip(), value.strip()
        value = (value.replace("\\n", "\n").replace("\\t", "\t").replace("\\\\", "\\"))
        if not key:
            fail(problems, "%s:%d: no key before the `=`" % (path, number))
        if key in strings:
            fail(problems, "%s:%d: `%s` is here twice" % (path, number, key))
        strings[key] = value
        if key == "language.name":
            name = value
    return strings, name


def check(problems):
    strings = wanted(problems)
    if not os.path.exists(ENGLISH):
        fail(problems, "languages/en.txt is missing (run --write)")
        return problems
    english, _ = read_catalog(ENGLISH, problems)

    missing = sorted(set(strings) - set(english))
    extra = sorted(set(english) - set(strings))
    for key in missing:
        fail(problems, "languages/en.txt has no `%s` (%r)" % (key, strings[key]))
    for key in extra:
        fail(problems, "languages/en.txt has `%s`, which no row or source asks for" % key)
    for key in sorted(set(strings) & set(english)):
        if strings[key] != english[key]:
            fail(problems, "languages/en.txt: `%s` reads %r, the source says %r"
                 % (key, english[key], strings[key]))

    # A translation may leave lines out; what it has must be a line English
    # has, saying the same numbers in the same slots.
    for path in sorted(glob.glob(os.path.join(CATALOGS, "*.txt"))):
        if os.path.abspath(path) == os.path.abspath(ENGLISH):
            continue
        catalog, name = read_catalog(path, problems)
        if name is None:
            fail(problems, "%s: no `language.name` line, so the menu would list it as its file name" % path)
        for key in sorted(set(catalog) - set(english)):
            fail(problems, "%s: `%s` is not a key English has" % (path, key))
        for key in sorted(set(catalog) & set(english)):
            theirs = sorted(SPEC.findall(catalog[key]))
            ours = sorted(SPEC.findall(english[key]))
            if theirs != ours:
                fail(problems, "%s: `%s` has %s where English has %s" % (path, key, theirs or "no %", ours or "no %"))
    return problems


def write():
    strings = wanted([])
    order = [row["key"] for row in settings_rows()]
    lines = [
        "# The settings menu, in English: the file a translator starts from.",
        "#",
        "# Copy it into the languages folder beside the settings file under the name of",
        "# your language (es.txt, de.txt, ...), translate the text after each `=`, and",
        "# pick that name in F1 > Config > Language. A line you leave out stays English.",
        "#",
        "# Keep every %s and %d as it is, spaces either side of `=` are trimmed, `#`",
        "# starts a comment, and `language.name` is what the menu's list will call your",
        "# language. Nothing here is written back: your file is only ever read.",
        "",
        "# The menu's own text.",
    ]
    for key in sorted(k for k in strings if not k.startswith("setting.")):
        lines.append("%s = %s" % (key, escape(strings[key])))
    lines.append("")
    lines.append("# Every setting, in the order the menu draws it.")
    for key in order:
        row_keys = [k for k in strings if k.startswith("setting." + key + ".")]
        if not row_keys:
            continue
        lines.append("")
        for k in sorted(row_keys, key=lambda s: ["label", "note", "group", "experimental"].index(s.rsplit(".", 1)[1])):
            lines.append("%s = %s" % (k, escape(strings[k])))
    os.makedirs(CATALOGS, exist_ok=True)
    with open(ENGLISH, "w", encoding="utf-8") as out:
        out.write("\n".join(lines).rstrip("\n") + "\n")
    print("wrote %s: %d strings" % (os.path.relpath(ENGLISH, ROOT), len(strings)))


def main():
    if "--write" in sys.argv[1:]:
        write()
        return 0
    problems = []
    check(problems)
    for message in problems:
        print("test_i18n_strings: " + message)
    if problems:
        print("test_i18n_strings: %d problem(s)" % len(problems))
        return 1
    print("test_i18n_strings: the catalogs and the sources agree")
    return 0


if __name__ == "__main__":
    sys.exit(main())
