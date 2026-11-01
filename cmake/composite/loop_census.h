#ifndef BLUEWAKE_COMPOSITE_LOOP_CENSUS_H
#define BLUEWAKE_COMPOSITE_LOOP_CENSUS_H

/* BLUEWAKE_LOOP_CENSUS (compile-time, diagnostic builds only): what goes round
 * the chassis loop (dispatch_loop.h). It counts the host's entries, each
 * boundary the loop takes (by target address, and whether the edge service was
 * skipped or asked) and why the loop returned. It only counts: the loop takes
 * the same path with it as without it (#530). A report goes to stderr when the
 * module unloads or the process exits.
 *
 * No identifier here may be `ctx`: the chunks define it as a macro. */

#if defined(BLUEWAKE_LOOP_CENSUS)
#include <stdio.h>
#include <stdlib.h>

enum {
    BW_LOOP_EXIT_FIRST_MISS,      /* the first dispatch found nothing */
    BW_LOOP_EXIT_FIRST_ONLY,      /* no edge service, or the first block charged nothing */
    BW_LOOP_EXIT_STOP,            /* an exception or the turn's budget */
    BW_LOOP_EXIT_SERVICE,         /* the edge service asked for the host */
    BW_LOOP_EXIT_MISS,            /* a successor with no translation */
    BW_LOOP_EXIT_NO_PROGRESS,     /* the ninth non-advancing successor */
    BW_LOOP_EXITS
};

#define BW_LOOP_CENSUS_SLOTS 65536u

static unsigned long long bw_loop_entries;
static unsigned long long bw_loop_boundaries;
static unsigned long long bw_loop_skipped;
static unsigned long long bw_loop_exits[BW_LOOP_EXITS];
static unsigned long long bw_loop_unrecorded; /* targets beyond the table */
static unsigned bw_loop_address[BW_LOOP_CENSUS_SLOTS];
static unsigned long long bw_loop_count[BW_LOOP_CENSUS_SLOTS];
static unsigned long long bw_loop_count_skipped[BW_LOOP_CENSUS_SLOTS];

static inline void bw_loop_census_boundary(unsigned address, bool skipped) {
    bw_loop_boundaries++;
    bw_loop_skipped += skipped;
    unsigned slot = (address * 0x9E3779B1u) >> 16;
    for (unsigned probes = 0; probes < BW_LOOP_CENSUS_SLOTS; ++probes) {
        if (bw_loop_count[slot] == 0u)
            bw_loop_address[slot] = address;
        if (bw_loop_address[slot] == address) {
            bw_loop_count[slot]++;
            bw_loop_count_skipped[slot] += skipped;
            return;
        }
        slot = (slot + 1u) & (BW_LOOP_CENSUS_SLOTS - 1u);
    }
    bw_loop_unrecorded++;
}

static inline int bw_loop_census_exit(int reason, int result) {
    bw_loop_exits[reason]++;
    return result;
}

/* Every target, one line each, so a script can sort and map them. */
__attribute__((destructor)) static void bw_loop_census_report(void) {
    if (bw_loop_entries == 0u)
        return;
    fprintf(stderr,
            "[loop-census] entries=%llu boundaries=%llu skipped=%llu unrecorded=%llu "
            "exit_first_miss=%llu exit_first_only=%llu exit_stop=%llu exit_service=%llu "
            "exit_miss=%llu exit_no_progress=%llu\n",
            bw_loop_entries, bw_loop_boundaries, bw_loop_skipped, bw_loop_unrecorded,
            bw_loop_exits[BW_LOOP_EXIT_FIRST_MISS], bw_loop_exits[BW_LOOP_EXIT_FIRST_ONLY],
            bw_loop_exits[BW_LOOP_EXIT_STOP], bw_loop_exits[BW_LOOP_EXIT_SERVICE],
            bw_loop_exits[BW_LOOP_EXIT_MISS], bw_loop_exits[BW_LOOP_EXIT_NO_PROGRESS]);
    for (unsigned i = 0; i < BW_LOOP_CENSUS_SLOTS; ++i)
        if (bw_loop_count[i] != 0u)
            fprintf(stderr, "[loop-census] target=0x%08X count=%llu skipped=%llu\n",
                    bw_loop_address[i], bw_loop_count[i], bw_loop_count_skipped[i]);
}

#define BW_LOOP_CENSUS_ENTRY() (bw_loop_entries++)
#define BW_LOOP_CENSUS_BOUNDARY(address, skipped) bw_loop_census_boundary((address), (skipped))
#define BW_LOOP_CENSUS_EXIT(reason, result) bw_loop_census_exit((reason), (result))
#else
#define BW_LOOP_CENSUS_ENTRY() ((void)0)
#define BW_LOOP_CENSUS_BOUNDARY(address, skipped) ((void)0)
#define BW_LOOP_CENSUS_EXIT(reason, result) (result)
#endif

#endif
