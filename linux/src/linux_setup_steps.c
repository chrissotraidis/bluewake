// The setup window's stage list. See linux_setup_steps.h.
#include "linux_setup_steps.h"

#include <string.h>

int bw_setup_stage_for_report(const char* stage, double fraction) {
    if (stage != NULL) {
        if (strcmp(stage, "Reading the disc") == 0) return BW_SETUP_STAGE_READ;
        if (strcmp(stage, "Unpacking game modules") == 0) return BW_SETUP_STAGE_UNPACK;
        // "Ready" arrives with fraction 1.0 once the outputs are swapped in.
        if (strcmp(stage, "Ready") == 0) return BW_SETUP_STAGE_UNPACK;
    }
    return fraction < 0.3 ? BW_SETUP_STAGE_READ : BW_SETUP_STAGE_UNPACK;
}

double bw_setup_overall_fraction(int stage, double stage_fraction) {
    static const double first[] = {0.00, 0.03, 0.10, 0.95};
    static const double last[] = {0.03, 0.10, 0.95, 1.00};
    if (stage < BW_SETUP_STAGE_CHECK) stage = BW_SETUP_STAGE_CHECK;
    if (stage >= BW_SETUP_STAGE_COUNT) stage = BW_SETUP_STAGE_COUNT - 1;
    if (!(stage_fraction > 0.0)) stage_fraction = 0.0;   // also catches NaN
    if (stage_fraction > 1.0) stage_fraction = 1.0;
    return first[stage] + (last[stage] - first[stage]) * stage_fraction;
}
