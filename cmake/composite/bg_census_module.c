/* See bg_census_module.h. A block's entry label can be reached twice for one
 * call when the turn ends right at it (the next turn resumes there), so the
 * counts are a hair high; a census, not an exact tally. */
#include "bg_census_module.h"

#include <stdio.h>

#if defined(_WIN32)
#define BW_BG_CENSUS_EXPORT __declspec(dllexport)
#else
#define BW_BG_CENSUS_EXPORT __attribute__((visibility("default")))
#endif

enum { kGroundCross, kGrpRp, kGrpThrough, kRp, kKinds };
static const char* const kNames[kKinds] = {"GroundCross", "GroundCrossGrpRp", "ChkGrpThrough", "GroundCrossRp"};
enum { kDepths = 8, kBins = 10 };
/* Per ground check: 0, 1, 2-3, 4-7, ..., 128-255, 256+. */
static const char* const kBinNames[kBins] = {"0", "1", "2-3", "4-7", "8-15", "16-31", "32-63", "64-127", "128-255", ">=256"};

static unsigned long long s_entries[kKinds], s_depth[kDepths];
static unsigned long long s_group_bins[kBins], s_through_bins[kBins], s_rp_bins[kBins];
static unsigned long long s_group_max, s_through_max, s_rp_max;
static unsigned s_groups, s_through, s_rp;
static int s_open;
/* Per top-level walk (a GroundCrossGrpRp call at depth 1): its subtree's
 * visits, whether it reaches a polygon tree, and the visits before the first. */
static unsigned long long s_top_calls, s_top_with_tree, s_top_visits, s_top_visits_before_tree;
static unsigned long long s_top_size_bins[kBins], s_top_tree_size_bins[kBins], s_top_before_bins[kBins];
static unsigned s_top_size, s_top_before;
static int s_top_open, s_top_tree;

static unsigned bin(unsigned n);

static void close_top(void) {
    if (!s_top_open)
        return;
    s_top_calls++;
    s_top_visits += s_top_size;
    s_top_size_bins[bin(s_top_size)]++;
    if (s_top_tree) {
        s_top_with_tree++;
        s_top_visits_before_tree += s_top_before;
        s_top_tree_size_bins[bin(s_top_size)]++;
        s_top_before_bins[bin(s_top_before)]++;
    }
    s_top_open = s_top_tree = 0;
    s_top_size = s_top_before = 0;
}

static unsigned bin(unsigned n) {
    unsigned b = 0;
    for (unsigned limit = 1u; b + 1u < kBins && n >= limit; limit <<= 1)
        ++b;
    return b;
}

static void close_check(void) {
    if (!s_open)
        return;
    s_group_bins[bin(s_groups)]++;
    s_through_bins[bin(s_through)]++;
    s_rp_bins[bin(s_rp)]++;
    if (s_groups > s_group_max) s_group_max = s_groups;
    if (s_through > s_through_max) s_through_max = s_through;
    if (s_rp > s_rp_max) s_rp_max = s_rp;
    s_groups = s_through = s_rp = 0;
}

void bluewake_bg_census_module(const CPUState* cpu, unsigned int address) {
    switch (address) {
    case BLUEWAKE_BG_CENSUS_MODULE_GROUND_CROSS:
        close_top();
        close_check();
        s_open = 1;
        s_entries[kGroundCross]++;
        break;
    case BLUEWAKE_BG_CENSUS_MODULE_GRP_RP: {
        const unsigned depth = cpu != NULL ? cpu->gpr[6] : 0u;
        s_entries[kGrpRp]++;
        s_depth[depth < kDepths ? depth : kDepths - 1u]++;
        s_groups++;
        if (depth <= 1u) {
            close_top();
            s_top_open = 1;
        }
        s_top_size++;
        break;
    }
    case BLUEWAKE_BG_CENSUS_MODULE_GRP_THROUGH: s_entries[kGrpThrough]++; s_through++; break;
    case BLUEWAKE_BG_CENSUS_MODULE_RP:
        s_entries[kRp]++;
        s_rp++;
        if (s_top_open && !s_top_tree) {
            s_top_tree = 1;
            s_top_before = s_top_size; /* visits so far, the one with the tree included */
        }
        break;
    default: break;
    }
}

BW_BG_CENSUS_EXPORT void bluewake_bg_census_module_report(void) {
    close_top();
    close_check();
    for (unsigned k = 0; k < kKinds; ++k)
        fprintf(stderr, "[bg-census-module] entries %s=%llu\n", kNames[k], s_entries[k]);
    for (unsigned d = 0; d < kDepths; ++d)
        fprintf(stderr, "[bg-census-module] GroundCrossGrpRp depth %u%s=%llu\n", d, d + 1u == kDepths ? "+" : "",
                s_depth[d]);
    for (unsigned b = 0; b < kBins; ++b)
        fprintf(stderr, "[bg-census-module] per ground check %s: groups=%llu filters=%llu tree_tests=%llu\n",
                kBinNames[b], s_group_bins[b], s_through_bins[b], s_rp_bins[b]);
    fprintf(stderr, "[bg-census-module] per ground check max: groups=%llu filters=%llu tree_tests=%llu\n",
            s_group_max, s_through_max, s_rp_max);
    fprintf(stderr, "[bg-census-module] top-level walks=%llu visits=%llu with_tree=%llu visits_before_first_tree=%llu\n",
            s_top_calls, s_top_visits, s_top_with_tree, s_top_visits_before_tree);
    for (unsigned b = 0; b < kBins; ++b)
        fprintf(stderr, "[bg-census-module] top-level walk size %s: all=%llu with_tree=%llu; visits before first tree %s: %llu\n",
                kBinNames[b], s_top_size_bins[b], s_top_tree_size_bins[b], kBinNames[b], s_top_before_bins[b]);
}
