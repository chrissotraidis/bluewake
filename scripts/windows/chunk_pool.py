"""Rewrite the composite source's chunks on every core.

The preparation steps (fast_blocks.py, direct_calls.py, global_guest_cpu.py,
inline_save_restore_gpr.py, return_ranges.py) each rewrite every chunk on its
own, so the chunks can go to separate processes: the result is the same file
for file, in a fraction of the time. BLUEWAKE_PREP_JOBS sets how many (the builders pass
their --jobs; 1 runs in this process, as before).
"""
from concurrent.futures import ProcessPoolExecutor
import os


def jobs():
    try:
        wanted = int(os.environ.get("BLUEWAKE_PREP_JOBS", "0"))
    except ValueError:
        wanted = 0
    return max(1, wanted or os.cpu_count() or 1)


def rewrite(path, transform, changed=None):
    """Read one chunk, transform it, and replace it only when it changed.
    `transform(text)` returns (text, result); `changed(result)` says whether to
    write (by default, whether the text differs). Returns the result."""
    with open(path, encoding="utf-8", newline="") as file:
        original = file.read()
    converted, result = transform(original)
    if changed(result) if changed is not None else converted != original:
        temporary = path.with_suffix(".c.tmp")
        with open(temporary, "w", encoding="utf-8", newline="") as file:
            file.write(converted)
        temporary.replace(path)
    return result


def map_chunks(function, chunks, initializer=None, initargs=()):
    """function(path) for every chunk, in chunk order, on jobs() processes.
    `initializer(*initargs)` sets each process up (what every chunk shares)."""
    chunks = list(chunks)
    count = min(jobs(), len(chunks))
    if count <= 1:
        if initializer is not None:
            initializer(*initargs)
        return [function(path) for path in chunks]
    with ProcessPoolExecutor(max_workers=count, initializer=initializer, initargs=initargs) as pool:
        return list(pool.map(function, chunks, chunksize=1))
