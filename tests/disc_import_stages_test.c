// The first run's stage list: pure decisions, checked without a window, so every
// platform that renders it agrees on the order and the wording.
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "disc_import_stages.h"

int main(void) {
    // Every stage has a name, and the count matches the enum.
    for (int stage = 0; stage < BW_IMPORT_STAGE_COUNT; ++stage) {
        const char* name = bw_import_stage_name(stage);
        assert(name != NULL && name[0] != '\0');
    }
    assert(bw_import_stage_name(-1) == NULL);
    assert(bw_import_stage_name(BW_IMPORT_STAGE_COUNT) == NULL);

    // The importer's own stage name decides, whatever fraction comes with it.
    assert(bw_import_stage_for_report("Reading the disc", 0.0) == BW_IMPORT_STAGE_READ);
    assert(bw_import_stage_for_report("Unpacking game modules", 0.0) == BW_IMPORT_STAGE_UNPACK);
    assert(bw_import_stage_for_report("Ready", 1.0) == BW_IMPORT_STAGE_UNPACK);
    // An unnamed report falls back to its fraction: reading below 0.3, the
    // modules from 0.3 on.
    assert(bw_import_stage_for_report(NULL, 0.0) == BW_IMPORT_STAGE_READ);
    assert(bw_import_stage_for_report(NULL, 0.29) == BW_IMPORT_STAGE_READ);
    assert(bw_import_stage_for_report(NULL, 0.3) == BW_IMPORT_STAGE_UNPACK);
    assert(bw_import_stage_for_report("", 0.9) == BW_IMPORT_STAGE_UNPACK);
    // The checking and saving stages belong to the UI: an importer report must
    // never claim one, or the list would show them before they ran.
    const char* reports[] = {NULL, "", "Reading the disc", "Unpacking game modules", "Ready",
                             "something new"};
    for (int report = 0; report < (int)(sizeof reports / sizeof reports[0]); ++report)
        for (int step = 0; step <= 10; ++step) {
            const int stage = bw_import_stage_for_report(reports[report], step / 10.0);
            assert(stage == BW_IMPORT_STAGE_READ || stage == BW_IMPORT_STAGE_UNPACK);
        }

    // One bar across all four stages: 0 at the start, 1 at the end, never
    // backwards across a stage boundary.
    assert(bw_import_overall_fraction(BW_IMPORT_STAGE_CHECK, 0.0) == 0.0);
    assert(bw_import_overall_fraction(BW_IMPORT_STAGE_SAVE, 1.0) == 1.0);
    double previous = -1.0;
    for (int stage = 0; stage < BW_IMPORT_STAGE_COUNT; ++stage)
        for (int step = 0; step <= 20; ++step) {
            const double value = bw_import_overall_fraction(stage, step / 20.0);
            assert(value >= previous && value >= 0.0 && value <= 1.0);
            previous = value;
        }
    // The slices meet: the end of one stage is the start of the next, so the
    // bar has no jumps and no stage is given no room.
    for (int stage = 0; stage + 1 < BW_IMPORT_STAGE_COUNT; ++stage) {
        const double end = bw_import_overall_fraction(stage, 1.0);
        const double next = bw_import_overall_fraction(stage + 1, 0.0);
        assert(end == next);
        assert(end > bw_import_overall_fraction(stage, 0.0));
    }
    // Out-of-range input is clamped rather than trusted.
    assert(bw_import_overall_fraction(-5, 0.5) == bw_import_overall_fraction(BW_IMPORT_STAGE_CHECK, 0.5));
    assert(bw_import_overall_fraction(99, 0.5) ==
           bw_import_overall_fraction(BW_IMPORT_STAGE_COUNT - 1, 0.5));
    assert(bw_import_overall_fraction(1, -2.0) == bw_import_overall_fraction(1, 0.0));
    assert(bw_import_overall_fraction(2, 5.0) == bw_import_overall_fraction(2, 1.0));

    // The state each stage is in, as every platform's list draws it.
    assert(bw_import_stage_state(0, 1, 0, 0) == BW_IMPORT_DONE);
    assert(bw_import_stage_state(1, 1, 0, 0) == BW_IMPORT_RUNNING);
    assert(bw_import_stage_state(2, 1, 0, 0) == BW_IMPORT_PENDING);
    assert(bw_import_stage_state(1, 1, 1, 0) == BW_IMPORT_FAILED);   // the failing stage
    assert(bw_import_stage_state(0, 1, 1, 0) == BW_IMPORT_DONE);     // earlier stages stay done
    assert(bw_import_stage_state(2, 1, 1, 0) == BW_IMPORT_PENDING);  // later ones stay pending
    for (int stage = 0; stage < BW_IMPORT_STAGE_COUNT; ++stage)
        assert(bw_import_stage_state(stage, 3, 0, 1) == BW_IMPORT_DONE);   // finished: all done

    // Marks are distinct and short enough for a text label.
    assert(strcmp(bw_import_stage_mark(BW_IMPORT_DONE), "[x]") == 0);
    assert(strcmp(bw_import_stage_mark(BW_IMPORT_RUNNING), "[>]") == 0);
    assert(strcmp(bw_import_stage_mark(BW_IMPORT_PENDING), "[ ]") == 0);
    assert(strcmp(bw_import_stage_mark(BW_IMPORT_FAILED), "[!]") == 0);

    // The text UI's four lines: one mark and one name per stage, in order.
    char lines[1024];
    bw_import_stage_lines(1, 0, 0, lines, sizeof lines);
    const char* expected =
        "[x] Checking the disc image and the data folder\n"
        "[>] Reading the disc\n"
        "[ ] Unpacking the game modules\n"
        "[ ] Saving the settings and the shortcuts\n";
    assert(strcmp(lines, expected) == 0);
    bw_import_stage_lines(1, 1, 0, lines, sizeof lines);   // the running stage failed
    assert(strstr(lines, "[!] Reading the disc") != NULL);
    bw_import_stage_lines(3, 0, 1, lines, sizeof lines);   // finished
    assert(strstr(lines, "[x] Saving the settings and the shortcuts") != NULL);
    // A short buffer is truncated, never overrun.
    char small[24];
    const size_t used = bw_import_stage_lines(1, 0, 0, small, sizeof small);
    assert(used < sizeof small && strlen(small) == used);
    assert(bw_import_stage_lines(1, 0, 0, NULL, 0) == 0);
    printf("disc_import_stages_test: OK\n");
    return 0;
}
