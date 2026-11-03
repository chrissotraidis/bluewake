#!/usr/bin/env python3
"""Write an instrumented copy of the composite dispatch loop.

Why this exists. scripts/chunk_bench.sh measures the emitted body's cost per
guest cycle, but it drives that measurement from a synthetic CPUState, which
cannot reproduce a real trajectory. To measure the real thing the benchmark
needs a real in-game CPUState, and the runtime has no way to write one.

This emits a copy of cmake/composite/dispatch_loop.c with a dump hook, so that
scripts/dump_chunk_state.sh can relink an instrumented composite in a temp
directory. The tracked source stays untouched and the certified dylib stays
byte-identical, which matters: the workstream depends on that artifact being
reproducible.

The hook is inert unless BLUEWAKE_DUMP_CHUNK_STATE is set, so an instrumented
build behaves exactly like the shipping one when the variable is absent.

Usage: make_chunk_dump_loop.py <source> <output>
"""

import sys

HOOK = r'''
/* --- BlueWake chunk-state dump (research instrument) -------------------
 * BLUEWAKE_DUMP_CHUNK_STATE is a comma-separated list of "LO:HI:PATH"
 * entries with hex addresses. The first time execution is dispatched into
 * one of those ranges, the CPUState and the MEM1 image are written to PATH
 * as a 6-word u32 header {magic, version, sizeof(CPUState), ram_size, pc,
 * reserved}, then the CPUState bytes, then the MEM1 bytes. Nothing happens
 * when the variable is unset.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BW_DUMP_MAX 16

static int bw_dump_parsed = 0;
static int bw_dump_count = 0;
static double bw_dump_t0 = -1.0;
static double bw_dump_after_ms = -1.0;
static struct {
    u32 lo, hi;
    int done;
    char path[512];
} bw_dump_spec[BW_DUMP_MAX];

static void bw_dump_parse(void) {
    bw_dump_parsed = 1;
    const char* spec = getenv("BLUEWAKE_DUMP_CHUNK_STATE");
    if (spec == NULL || spec[0] == 0)
        return;
    char buf[8192];
    snprintf(buf, sizeof(buf), "%s", spec);
    char* save = NULL;
    for (char* tok = strtok_r(buf, ",", &save);
         tok != NULL && bw_dump_count < BW_DUMP_MAX;
         tok = strtok_r(NULL, ",", &save)) {
        unsigned lo = 0u, hi = 0u;
        char path[512];
        if (sscanf(tok, "%x:%x:%511s", &lo, &hi, path) != 3)
            continue;
        bw_dump_spec[bw_dump_count].lo = (u32)lo;
        bw_dump_spec[bw_dump_count].hi = (u32)hi;
        bw_dump_spec[bw_dump_count].done = 0;
        snprintf(bw_dump_spec[bw_dump_count].path,
                 sizeof(bw_dump_spec[bw_dump_count].path), "%s", path);
        ++bw_dump_count;
    }
}

static void bw_dump_maybe(CPUState* cpu, u32 address) {
    if (!bw_dump_parsed)
        bw_dump_parse();
    if (bw_dump_count == 0)
        return;
    /* Optional arm delay. The boot executes these chunks too, so a snapshot
     * taken at the first entry is a real state but not a play-scene one.
     * BLUEWAKE_DUMP_AFTER_MS holds the hook off until the route has been
     * running long enough to be inside the play window. */
    if (bw_dump_after_ms < 0.0) {
        const char* after = getenv("BLUEWAKE_DUMP_AFTER_MS");
        bw_dump_after_ms = (after != NULL) ? atof(after) : 0.0;
    }
    if (bw_dump_after_ms > 0.0) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        double now = (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
        if (bw_dump_t0 < 0.0)
            bw_dump_t0 = now;
        if (now - bw_dump_t0 < bw_dump_after_ms)
            return;
    }
    for (int i = 0; i < bw_dump_count; ++i) {
        if (bw_dump_spec[i].done)
            continue;
        if (address < bw_dump_spec[i].lo || address > bw_dump_spec[i].hi)
            continue;
        FILE* f = fopen(bw_dump_spec[i].path, "wb");
        if (f == NULL) {
            bw_dump_spec[i].done = 1;
            continue;
        }
        u32 header[6];
        header[0] = 0x31574442u; /* BWD1 in little-endian byte order */
        header[1] = 1u;
        header[2] = (u32)sizeof(CPUState);
        header[3] = cpu->ram_size;
        header[4] = address;
        header[5] = 0u;
        fwrite(header, sizeof(header), 1, f);
        fwrite(cpu, sizeof(CPUState), 1, f);
        if (cpu->ram != NULL && cpu->ram_size != 0u)
            fwrite(cpu->ram, 1, cpu->ram_size, f);
        fclose(f);
        bw_dump_spec[i].done = 1;
        fprintf(stderr, "[dump-chunk-state] pc=%08X ram=%u -> %s", address,
                (unsigned)cpu->ram_size, bw_dump_spec[i].path);
        fputc(10, stderr);
    }
}
/* --- end chunk-state dump --------------------------------------------- */
'''


def main() -> int:
    if len(sys.argv) != 3:
        sys.stderr.write(__doc__)
        return 2
    source_path, output_path = sys.argv[1], sys.argv[2]
    with open(source_path, "r", encoding="utf-8") as handle:
        text = handle.read()

    anchor = '#include "dispatch_loop.h"' + chr(10)
    if anchor not in text:
        sys.stderr.write("make_chunk_dump_loop: include anchor not found" + chr(10))
        return 1
    text = text.replace(anchor, anchor + HOOK, 1)

    needle_a = "    int dispatched = dispatch(ctx, address);" + chr(10)
    repl_a = "    bw_dump_maybe(ctx, address);" + chr(10) + needle_a
    needle_b = "        dispatched = dispatch(ctx, address);" + chr(10)
    repl_b = "        bw_dump_maybe(ctx, address);" + chr(10) + needle_b
    for needle, replacement in ((needle_a, repl_a), (needle_b, repl_b)):
        if needle not in text:
            sys.stderr.write(
                "make_chunk_dump_loop: dispatch call site not found: "
                + repr(needle) + chr(10)
            )
            return 1
        text = text.replace(needle, replacement, 1)

    with open(output_path, "w", encoding="utf-8") as handle:
        handle.write(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
