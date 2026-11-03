#include "bg_census.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool bluewake_bg_census_on;

enum { kGroundCross, kGrpRp, kGrpThrough, kRp, kCheckGnd, kCheckWall, kCheckCommon, kTri, kKinds };
static const char* const kNames[kKinds] = {
    "GroundCross", "GroundCrossGrpRp", "ChkGrpThrough", "GroundCrossRp",
    "RwgGroundCheckGnd", "RwgGroundCheckWall", "RwgGroundCheckCommon", "cM3d_CrossY_Tri_Front",
};
enum { kDepths = 8, kBins = 8 };
// Per ground check, groups / tree tests / polygon checks in bins 0, 1, 2-3,
// 4-7, 8-15, 16-31, 32-63, 64+.
static const char* const kBinNames[kBins] = {"0", "1", "2-3", "4-7", "8-15", "16-31", "32-63", ">=64"};

static unsigned long long s_entries[kKinds], s_depth[kDepths];
static unsigned long long s_groups_bins[kBins], s_rp_bins[kBins], s_polygon_bins[kBins];
static unsigned long long s_max_groups, s_max_rp, s_max_polygons;
static unsigned s_groups, s_rp, s_polygons; // since the current ground check began
static bool s_open;

static unsigned bin(unsigned n) {
    unsigned b = 0;
    for (unsigned limit = 1u; b + 1u < kBins && n >= limit; limit <<= 1)
        ++b;
    return b;
}

static void close_check(void) {
    if (!s_open)
        return;
    s_groups_bins[bin(s_groups)]++;
    s_rp_bins[bin(s_rp)]++;
    s_polygon_bins[bin(s_polygons)]++;
    if (s_groups > s_max_groups) s_max_groups = s_groups;
    if (s_rp > s_max_rp) s_max_rp = s_rp;
    if (s_polygons > s_max_polygons) s_max_polygons = s_polygons;
    s_groups = s_rp = s_polygons = 0;
}

void bluewake_bg_census_init(void) {
    const char* value = getenv("BLUEWAKE_BG_CENSUS");
    bluewake_bg_census_on = value != NULL && strcmp(value, "1") == 0;
    if (bluewake_bg_census_on)
        fprintf(stderr, "[bg-census] on (needs BLUEWAKE_DIRECT_CALLS=0 to see every call)\n");
}

void bluewake_bg_census_enter(const CPUState* cpu, u32 address) {
    switch (address) {
    case BLUEWAKE_BG_CENSUS_GROUND_CROSS:
        close_check();
        s_open = true;
        s_entries[kGroundCross]++;
        break;
    case BLUEWAKE_BG_CENSUS_GRP_RP: {
        s_entries[kGrpRp]++;
        const u32 depth = cpu != NULL ? cpu->gpr[6] : 0u;
        s_depth[depth < kDepths ? depth : kDepths - 1u]++;
        s_groups++;
        break;
    }
    case BLUEWAKE_BG_CENSUS_GRP_THROUGH: s_entries[kGrpThrough]++; break;
    case BLUEWAKE_BG_CENSUS_RP: s_entries[kRp]++; s_rp++; break;
    case BLUEWAKE_BG_CENSUS_CHECK_GND: s_entries[kCheckGnd]++; s_polygons++; break;
    case BLUEWAKE_BG_CENSUS_CHECK_WALL: s_entries[kCheckWall]++; s_polygons++; break;
    case BLUEWAKE_BG_CENSUS_CHECK_COMMON: s_entries[kCheckCommon]++; break;
    case BLUEWAKE_BG_CENSUS_TRI: s_entries[kTri]++; break;
    default: break;
    }
}

void bluewake_bg_census_report(void) {
    if (!bluewake_bg_census_on)
        return;
    close_check();
    for (unsigned k = 0; k < kKinds; ++k)
        fprintf(stderr, "[bg-census] entries %s=%llu\n", kNames[k], s_entries[k]);
    for (unsigned d = 0; d < kDepths; ++d)
        fprintf(stderr, "[bg-census] GroundCrossGrpRp depth %u%s=%llu\n", d, d + 1u == kDepths ? "+" : "",
                s_depth[d]);
    for (unsigned b = 0; b < kBins; ++b)
        fprintf(stderr, "[bg-census] per ground check %s: groups=%llu tree_tests=%llu polygon_checks=%llu\n",
                kBinNames[b], s_groups_bins[b], s_rp_bins[b], s_polygon_bins[b]);
    fprintf(stderr, "[bg-census] per ground check max: groups=%llu tree_tests=%llu polygon_checks=%llu\n",
            s_max_groups, s_max_rp, s_max_polygons);
}
