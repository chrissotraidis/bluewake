#ifndef BLUEWAKE_BG_CENSUS_MODULE_H
#define BLUEWAKE_BG_CENSUS_MODULE_H

/* The ground collision walk's shape, counted inside the module (#542 M0;
 * --bg-census diagnostic builds only). scripts/windows/bg_census.py calls
 * this at the entry label of each function below, so it sees every call,
 * however the chunk reaches it. It only reads registers. No identifier here
 * may be `ctx`. */

#include "core/cpu.h"

#define BLUEWAKE_BG_CENSUS_MODULE_GROUND_CROSS 0x802468E4u /* cBgS::GroundCross */
#define BLUEWAKE_BG_CENSUS_MODULE_GRP_RP 0x802491F4u       /* cBgW::GroundCrossGrpRp (r6: depth) */
#define BLUEWAKE_BG_CENSUS_MODULE_GRP_THROUGH 0x800A9684u  /* dBgW::ChkGrpThrough */
#define BLUEWAKE_BG_CENSUS_MODULE_RP 0x80248C38u           /* cBgW::GroundCrossRp */

void bluewake_bg_census_module(const CPUState* cpu, unsigned int address);
void bluewake_bg_census_module_report(void);

#endif
