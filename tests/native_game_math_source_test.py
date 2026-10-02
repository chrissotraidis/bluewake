#!/usr/bin/env python3
"""Test hooks against a WORKTREE COPY of the comparison DLL's C chunks.

  python tests/native_game_math_source_test.py --reference E:/Github/Wind-Waker-Recomp/build/windows/composite-src --copy .native-study/hooks --diff tests/native_game_math_hooks.diff

Verifies every certificate, the cross-chunk subtraction dependency, rejection
of changed bodies/mod variants/watched PCs/missing dependencies, removal of
obsolete hooks, byte-identical idempotence, and pre/post-fast-block hashes.
Only the copy/diff paths under this worktree may be written. Never runs builds
or the app. The emitted diff contains only the small entry-hook additions.
"""
import argparse
import contextlib
import difflib
import hashlib
import importlib.util
import io
from pathlib import Path
import re
import shutil
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts/windows"))
import native_game_math as native
import fast_blocks


def before_fast_blocks(text):
    """Undo only the added copies/jumps in the read-only reference's copy."""
    heads = list(re.finditer(r"^(?:static )?void \w+\(CPUState\* ctx_param\) \{$", text, re.M))
    out, last = [], 0
    for i, head in enumerate(heads):
        end = heads[i+1].start() if i+1 < len(heads) else len(text)
        body = text[head.start():end]
        fast = body.find("\nbwfast_")
        if fast >= 0:
            close = body.find("\n}", fast)
            assert close >= 0
            original = body[:fast]
            assert original.endswith("    return;")
            body = original[:-len("    return;")] + body[close:]
        out.append(text[last:head.start()])
        out.append(body)
        last = end
    out.append(text[last:])
    text = "".join(out).replace(fast_blocks.MARK, "")
    text = re.sub(r"^    if \(cycle_block_prepaid\) goto bwfast_\w+;\n", "", text, flags=re.M)
    return re.sub(r"^(?:bwslow|bwend)_\w+: ;\n", "", text, flags=re.M)


def inside(path):
    path = path.resolve()
    if not path.is_relative_to(ROOT) or path == ROOT:
        raise ValueError(f"test output must be inside {ROOT}: {path}")
    return path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--reference", type=Path, required=True)
    parser.add_argument("--copy", type=Path, required=True)
    parser.add_argument("--diff", type=Path, required=True)
    args = parser.parse_args()
    copy, diff = inside(args.copy), inside(args.diff)
    copy.mkdir(parents=True, exist_ok=True)
    starts = {spec[0] for spec in native.FRAGMENTS.values()}
    originals = {}
    for start in starts:
        found = sorted(args.reference.glob(f"chunks_*/*_{start:08X}.c"))
        assert found, f"missing comparison source chunk {start:08X}"
        for source in found:
            target = copy / source.relative_to(args.reference)
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, target)
            originals[target] = target.read_text(encoding="utf-8")
    shutil.copyfile(args.reference / "generated.h", copy / "generated.h")
    chunks = sorted(copy.glob("chunks_*/*.c"))
    entries = native.certify(chunks, native.watched_addresses())
    assert entries == set(native.ENTRIES), f"uncertified entries: {set(native.ENTRIES)-entries}"
    total = 0
    patch = []
    for path in chunks:
        own = int(path.stem[-8:], 16)
        converted, count = native.transform(originals[path], own, entries)
        again, repeated = native.transform(converted, own, entries)
        assert converted == again and repeated == 0
        path.write_text(converted, encoding="utf-8", newline="\n")
        total += count
        relative = path.relative_to(copy).as_posix()
        patch.extend(difflib.unified_diff(originals[path].splitlines(True), converted.splitlines(True),
                                        fromfile="a/"+relative, tofile="b/"+relative, n=2))
    assert total == len(entries), f"unexpected hooks: {total}"
    assert native.certify(chunks, set()) == entries
    # Exercise the ACTUAL build order on the copy: game hooks before fast
    # blocks, then recertify the resulting original bodies and loop helper.
    for path in chunks:
        text = before_fast_blocks(originals[path])
        hooked, _ = native.transform(text, int(path.stem[-8:], 16), entries)
        prepared, _ = fast_blocks.transform(hooked)
        assert native.transform(prepared, int(path.stem[-8:], 16), entries)[0] == prepared
        path.write_text(prepared, encoding="utf-8", newline="\n")
    assert native.certify(chunks, set()) == entries
    # Normalize the source back to before fast_blocks.py: its added control
    # labels/jumps and copies must be irrelevant to certification.
    for name, (chunk, start, end, digest) in native.FRAGMENTS.items():
        path = next(p for p in chunks if p.parent.name == "chunks_dol" and p.stem.endswith(f"{chunk:08X}"))
        body = native.fragment(path.read_text(), chunk, start, end)
        assert hashlib.sha256(body.encode()).hexdigest() == digest, name
    with contextlib.redirect_stdout(io.StringIO()):
        # One changed SDK instruction disables ONLY the wrappers that emulate it.
        sdk = next(p for p in chunks if p.stem.endswith("8030D6E0"))
        saved = sdk.read_text()
        changed = saved.replace("// 8030DCE0:", "// 8030DCE1:", 1)
        assert changed != saved
        sdk.write_text(changed)
        rejected = native.certify(chunks, set())
        assert rejected == entries - {0x80245674}
        add_chunk = next(p for p in chunks if p.stem.endswith("802416E0"))
        reverted, _ = native.transform(add_chunk.read_text(), 0x802416E0, rejected)
        assert "bluewake_native_game_math(ctx, 0x80245674u)" not in reverted
        sdk.write_text(saved)
        # Losing the tail of the split subtraction must disable its entry hook.
        tail = next(p for p in chunks if p.stem.endswith("802456E0"))
        missing = native.certify([p for p in chunks if p != tail], set())
        assert 0x802456C4 not in missing
        # A changed mod variant invalidates natives even if its base matches.
        variant = copy / "chunks_mod_test" / "variant_8030D6E0.c"
        variant.parent.mkdir(exist_ok=True)
        variant.write_text(changed)
        assert 0x80245674 not in native.certify(chunks+[variant], set())
        variant.unlink()
        # Host-visible internal boundaries prohibit replacing the whole routine.
        watched = native.certify(chunks, {0x8024AEC8})
        assert 0x8024AE3C not in watched and len(watched) == len(entries)-1
    diff.parent.mkdir(parents=True, exist_ok=True)
    diff.write_text("".join(patch), encoding="utf-8", newline="\n")
    print(f"{len(native.FRAGMENTS)} fragment certificates, {total} hooks, build order, idempotence and all rejection checks passed")
    print(f"Copy: {copy}; diff: {diff}")


if __name__ == "__main__":
    main()
