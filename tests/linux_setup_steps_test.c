// The setup window's stage list: pure decisions, checked without a window.
#include <assert.h>
#include <stdio.h>

#include "linux_setup_steps.h"

int main(void) {
    // The importer's own stage name decides, whatever fraction comes with it.
    assert(bw_setup_stage_for_report("Reading the disc", 0.0) == BW_SETUP_STAGE_READ);
    assert(bw_setup_stage_for_report("Unpacking game modules", 0.0) == BW_SETUP_STAGE_UNPACK);
    assert(bw_setup_stage_for_report("Ready", 1.0) == BW_SETUP_STAGE_UNPACK);
    // An unnamed report falls back to its fraction: reading below 0.3, the
    // modules from 0.3 on.
    assert(bw_setup_stage_for_report(NULL, 0.0) == BW_SETUP_STAGE_READ);
    assert(bw_setup_stage_for_report(NULL, 0.29) == BW_SETUP_STAGE_READ);
    assert(bw_setup_stage_for_report(NULL, 0.3) == BW_SETUP_STAGE_UNPACK);
    assert(bw_setup_stage_for_report(NULL, 1.0) == BW_SETUP_STAGE_UNPACK);
    assert(bw_setup_stage_for_report("", 0.9) == BW_SETUP_STAGE_UNPACK);
    assert(bw_setup_stage_for_report("something new", 0.1) == BW_SETUP_STAGE_READ);
    // The checking and saving stages belong to the window: an importer report
    // must never claim one, or the checklist would show them before they ran.
    const char* names[] = {NULL, "", "Reading the disc", "Unpacking game modules", "Ready",
                           "something new"};
    for (int name = 0; name < (int)(sizeof names / sizeof names[0]); ++name)
        for (int step = 0; step <= 10; ++step) {
            const int stage = bw_setup_stage_for_report(names[name], step / 10.0);
            assert(stage == BW_SETUP_STAGE_READ || stage == BW_SETUP_STAGE_UNPACK);
        }
    // One bar across all four stages: 0 at the start, 1 at the end, and never
    // backwards across a stage boundary.
    assert(bw_setup_overall_fraction(BW_SETUP_STAGE_CHECK, 0.0) == 0.0);
    assert(bw_setup_overall_fraction(BW_SETUP_STAGE_SAVE, 1.0) == 1.0);
    double previous = -1.0;
    for (int stage = 0; stage < BW_SETUP_STAGE_COUNT; ++stage)
        for (int step = 0; step <= 20; ++step) {
            const double value = bw_setup_overall_fraction(stage, step / 20.0);
            assert(value >= previous);
            assert(value >= 0.0 && value <= 1.0);
            previous = value;
        }
    // Out-of-range input is clamped rather than trusted.
    assert(bw_setup_overall_fraction(-5, 0.5) == bw_setup_overall_fraction(BW_SETUP_STAGE_CHECK, 0.5));
    assert(bw_setup_overall_fraction(99, 0.5) ==
           bw_setup_overall_fraction(BW_SETUP_STAGE_COUNT - 1, 0.5));
    assert(bw_setup_overall_fraction(1, -2.0) == bw_setup_overall_fraction(1, 0.0));
    assert(bw_setup_overall_fraction(2, 5.0) == bw_setup_overall_fraction(2, 1.0));
    printf("linux_setup_steps_test: OK\n");
    return 0;
}
