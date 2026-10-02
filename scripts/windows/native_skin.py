#!/usr/bin/env python3
"""Call J3DModel::calcWeightEnvelopeMtx natively (cmake/composite/native_skin.c).

  native_skin.py COMPOSITE_SRC

The function's one caller, J3DModel::calc, is in the same chunk: its bl at
0x802EE9D8 is a goto. This makes it try bluewake_native_skin first and carry
on at the return address when that ran the whole function; otherwise, and
whenever the native declines, the translation runs as before.

Only where the translated body is the one tests/native_skin_test.c compared
the native against, every register and byte: its hash as the Windows steps
before this one leave it (whitespace aside). A different body (another
translator, another step) is left alone and the step says so.

The change is repeatable (a prepared chunk is left as it is) and keeps LF line
ends. Run it after direct_calls.py and before prepare_native_math.py, whose
manifest hashes the chunks as they finally are.
"""
import hashlib
import sys
from pathlib import Path

MARK = "/* bluewake: calcWeightEnvelopeMtx natively (cmake/composite/native_skin.c) */\n"
INCLUDE = '#include "../generated.h"\n'
START, END = "\nlabel_802EE67C:", "\nlabel_802EE874:"
BODY_SHA256 = "401b6d74be3c73babb47e9a329fb6173007fe0c907b94780388a463ab09b6013"
CALL = (
    "    // 802EE9D8: bl      0x802EE67C\n"
    "    {\n"
    "            ctx->lr = 0x802EE9DCu;\n"
    "            if (ctx->downcount <= -(s64)DOLRECOMP_C_LOOP_CYCLE_BUDGET) {\n"
    "                ctx->pc = 0x802EE67Cu;\n"
    "                return;\n"
    "            }\n"
    "            goto label_802EE67C;\n"
    "    }\n")
NATIVE_CALL = CALL.replace(
    "            goto label_802EE67C;\n",
    "            if (bluewake_native_skin(ctx))\n"
    "                goto label_802EE9DC;\n"
    "            goto label_802EE67C;\n")


def body_hash(text):
    begin, end = text.find(START), text.find(END)
    if begin < 0 or end <= begin:
        return None
    return hashlib.sha256(" ".join(text[begin:end].split()).encode()).hexdigest()


def main():
    root = Path(sys.argv[1])
    done = kept = 0
    for path in sorted(root.glob("chunks_*/*802ED6E0*.c")):
        with open(path, encoding="utf-8", newline="") as file:
            text = file.read()
        if MARK in text:
            kept += 1
            continue
        if body_hash(text) != BODY_SHA256 or text.count(CALL) != 1 or INCLUDE not in text:
            print(f"{path.relative_to(root)}: not the verified calcWeightEnvelopeMtx; left alone")
            continue
        text = text.replace(CALL, NATIVE_CALL).replace(INCLUDE, INCLUDE + MARK + '#include "native_skin.h"\n', 1)
        temporary = path.with_suffix(".c.tmp")
        with open(temporary, "w", encoding="utf-8", newline="") as file:
            file.write(text)
        temporary.replace(path)
        done += 1
    print(f"native calcWeightEnvelopeMtx: {done} call sites (already {kept})")


if __name__ == "__main__":
    main()
