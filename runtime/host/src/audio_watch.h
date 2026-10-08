#ifndef BLUEWAKE_AUDIO_WATCH_H
#define BLUEWAKE_AUDIO_WATCH_H

#include "core/cpu.h"
#include <stdbool.h>

// Cutscene sound in the session log (#65, #97), in every build. It only reads
// guest memory; it never changes the game.
//
//   [demo] start ...      a cutscene (a JStudio demo) begins: stage and event
//   [demo] end ...        how many sound cues it gave, how many got a sound,
//                         and how long its output was silent
//   [demo-sound] ...      a cue that got no sound (the first few per cutscene)
//   [audio-lost] ...      a cutscene's output, or the output while streamed
//                         music plays (the intro story), has been silent for
//                         6 seconds, and when sound comes back
//   BLUEWAKE_AUDIO_WATCH_TRACE=1 also logs every cue.
//
// GZLE01 revision 0: the cue is JStudio_JAudio::TAdaptor_sound's fade-in
// output, which starts each sound a cutscene asks for.
#define BLUEWAKE_AUDIO_WATCH_CUE 0x8027908Cu

void bluewake_audio_watch_cue(CPUState* cpu);
// Once per retrace, after the game's code for it has run.
void bluewake_audio_watch_retrace(CPUState* cpu, unsigned long long retrace);

static inline bool bluewake_audio_watch_observes(u32 address) {
    return address == BLUEWAKE_AUDIO_WATCH_CUE;
}

static inline void bluewake_audio_watch_dispatch(CPUState* cpu, u32 address) {
    if (__builtin_expect(address == BLUEWAKE_AUDIO_WATCH_CUE, 0))
        bluewake_audio_watch_cue(cpu);
}

#endif
