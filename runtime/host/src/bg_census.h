#ifndef BLUEWAKE_BG_CENSUS_H
#define BLUEWAKE_BG_CENSUS_H

#include "core/cpu.h"
#include <stdbool.h>

// The ground collision walk's shape in play (#542 M0), for a local
// measurement only. BLUEWAKE_BG_CENSUS=1 counts every entry to the ground
// check's functions and, per ground check, how many groups, polygon-tree tests
// and polygon checks it led to; the counts go to the log at exit. It only reads
// registers and never changes the game.
//
// It needs every call to pass the host's edge service: run it with
// BLUEWAKE_DIRECT_CALLS=0. The addresses are kept off the module's watch list
// (the markers below), so normal builds keep their direct calls here.
//
// GZLE01 revision 0 (tww decompilation, config/GZLE01/symbols.txt):
// bluewake-unwatched-begin
#define BLUEWAKE_BG_CENSUS_GROUND_CROSS 0x802468E4u    /* cBgS::GroundCross */
#define BLUEWAKE_BG_CENSUS_GRP_RP 0x802491F4u          /* cBgW::GroundCrossGrpRp (r6: depth) */
#define BLUEWAKE_BG_CENSUS_GRP_THROUGH 0x800A9684u     /* dBgW::ChkGrpThrough */
#define BLUEWAKE_BG_CENSUS_RP 0x80248C38u              /* cBgW::GroundCrossRp */
#define BLUEWAKE_BG_CENSUS_CHECK_GND 0x80248AB8u       /* cBgW::RwgGroundCheckGnd */
#define BLUEWAKE_BG_CENSUS_CHECK_WALL 0x80248B68u      /* cBgW::RwgGroundCheckWall */
#define BLUEWAKE_BG_CENSUS_CHECK_COMMON 0x8024898Cu    /* cBgW::RwgGroundCheckCommon */
#define BLUEWAKE_BG_CENSUS_TRI 0x8024C370u             /* cM3d_CrossY_Tri_Front */
// bluewake-unwatched-end

extern bool bluewake_bg_census_on;

void bluewake_bg_census_init(void);
void bluewake_bg_census_enter(const CPUState* cpu, u32 address);
void bluewake_bg_census_report(void);

static inline bool bluewake_bg_census_observes(u32 address) {
    return __builtin_expect(bluewake_bg_census_on, 0) &&
           (address == BLUEWAKE_BG_CENSUS_GROUND_CROSS || address == BLUEWAKE_BG_CENSUS_GRP_RP ||
            address == BLUEWAKE_BG_CENSUS_GRP_THROUGH || address == BLUEWAKE_BG_CENSUS_RP ||
            address == BLUEWAKE_BG_CENSUS_CHECK_GND || address == BLUEWAKE_BG_CENSUS_CHECK_WALL ||
            address == BLUEWAKE_BG_CENSUS_CHECK_COMMON || address == BLUEWAKE_BG_CENSUS_TRI);
}

static inline void bluewake_bg_census_dispatch(const CPUState* cpu, u32 address) {
    if (__builtin_expect(bluewake_bg_census_on, 0))
        bluewake_bg_census_enter(cpu, address);
}

#endif
