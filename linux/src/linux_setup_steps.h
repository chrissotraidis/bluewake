// BlueWake's Linux setup window: the stages it shows and how the importer's
// own progress reports map onto them. Kept free of ImGui/SDL so a headless
// test (tests/linux_setup_steps_test.c) can check the mapping, the same split
// the TotK installer makes between guilogic.py and gui.py.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

enum {
    BW_SETUP_STAGE_CHECK = 0,  // the disc image is checked (id, main.dol SHA-1)
    BW_SETUP_STAGE_READ,       // the disc header and main.dol are read
    BW_SETUP_STAGE_UNPACK,     // the 415 game modules are unpacked and swapped in
    BW_SETUP_STAGE_SAVE,       // settings, shortcuts and the remembered disc
    BW_SETUP_STAGE_COUNT
};

// Which stage an importer report belongs to: by its own stage name when it has
// one ("Reading the disc", "Unpacking game modules", "Ready"), otherwise by the
// fraction it reports (below 0.3 is reading; from 0.3 the modules). Returns
// BW_SETUP_STAGE_READ or BW_SETUP_STAGE_UNPACK only: the checking and saving
// stages belong to the window, not to the importer.
int bw_setup_stage_for_report(const char* stage, double fraction);

// One overall 0..1 value for the progress bar: each stage owns a slice, so the
// bar keeps moving across all four instead of restarting per stage.
double bw_setup_overall_fraction(int stage, double stage_fraction);

#ifdef __cplusplus
}
#endif
