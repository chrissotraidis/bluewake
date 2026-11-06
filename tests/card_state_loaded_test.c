// SPDX-License-Identifier: GPL-3.0-or-later
// Saving after a save state is loaded (bluewake_card_runtime_state_loaded):
// a state taken with saving off turns it off in the run that loads it, from
// main.c's HOSTVARS field or, for an older state, from the game's messages the
// state holds as "saving is off"; a load never turns it back on; and
// BLUEWAKE_STAGE_SELECT_SAVES keeps it on. Each case runs in a process of its
// own (argv[1]), since saving stays off for the rest of a run.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include "../runtime/host/src/card_runtime.c"

static void put_text(CPUState* cpu, u32 address, const char* text) {
    for (size_t i = 0; i <= strlen(text); ++i)
        mem_write8(cpu, address + (u32)i, (u8)text[i]);
}

static bool text_at(CPUState* cpu, u32 address, const char* text) {
    for (size_t i = 0; i <= strlen(text); ++i)
        if (mem_read8(cpu, address + (u32)i) != (u8)text[i])
            return false;
    return true;
}

int main(int argc, char** argv) {
    const char* which = argc > 1 ? argv[1] : "marker";
    CPUState cpu;
    assert(cpu_init(&cpu));
    // The game's no-card message where every run has had it.
    put_text(&cpu, k_no_card_texts[1].seen_at, k_no_card_texts[1].text);
    if (strcmp(which, "marker") == 0) {
        // A clean game: saving stays on.
        bluewake_card_runtime_state_loaded(&cpu, false);
        assert(!bluewake_card_runtime_writes_refused());
        // An older state from a session with saving off: its game holds the
        // message as "saving is off", somewhere else in MEM1.
        put_text(&cpu, 0x80400000u, k_no_card_texts[0].instead);
        bluewake_card_runtime_state_loaded(&cpu, false);
        assert(bluewake_card_runtime_writes_refused());
        assert(text_at(&cpu, k_no_card_texts[1].seen_at, k_no_card_texts[1].instead));
        // A clean state loaded afterwards leaves it off.
        put_text(&cpu, 0x80400000u, "");
        put_text(&cpu, k_no_card_texts[1].seen_at, k_no_card_texts[1].text);
        bluewake_card_runtime_state_loaded(&cpu, false);
        assert(bluewake_card_runtime_writes_refused());
    } else if (strcmp(which, "saved") == 0) {
        // The state says saving was off where it was taken, its game's
        // messages untouched (taken before they were written).
        bluewake_card_runtime_state_loaded(&cpu, true);
        assert(bluewake_card_runtime_writes_refused());
        assert(text_at(&cpu, k_no_card_texts[1].seen_at, k_no_card_texts[1].instead));
        // The card answers as one taken out.
        cpu.pc = 0x8031DC9Cu;
        cpu.lr = 0x80003140u;
        assert(bluewake_card_runtime_dispatch(&cpu));
        assert((s32)cpu.gpr[3] == DOL_CARD_RESULT_NO_CARD);
    } else if (strcmp(which, "kept") == 0) {
        // The player keeps saving on whatever the stage select does.
        setenv("BLUEWAKE_STAGE_SELECT_SAVES", "1", 1);
        bluewake_card_runtime_state_loaded(&cpu, true);
        assert(!bluewake_card_runtime_writes_refused());
    } else {
        return 2;
    }
    cpu_free(&cpu);
    printf("card state loaded (%s) passed\n", which);
    return 0;
}
