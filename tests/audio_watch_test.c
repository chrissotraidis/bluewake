#ifdef NDEBUG
#undef NDEBUG
#endif
#include "audio_watch.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The cutscene audio watch against a fake game: a cutscene with one cue that
// got a sound and one that got none, six seconds of silence, sound coming
// back, the cutscene's summary, and silent streamed music outside a cutscene.
enum {
    kDemoManager = 0x803CA6D0u, kStageName = 0x803C9D3Cu, kEventId = 0x803C9EB8u, kLastDac = 0x803F7404u,
    kManager = 0x80100000u, kDac = 0x80300000u, kAdaptor = 0x80400000u, kAdaptor2 = 0x80400200u,
    kSound = 0x80500000u, kStreamObject = 0x80600000u, kR13 = 0x80700000u,
};

static unsigned long long g_retrace;

static void retraces(CPUState* cpu, unsigned count) {
    for (unsigned i = 0; i < count; i++)
        bluewake_audio_watch_retrace(cpu, ++g_retrace);
}

int main(void) {
    char path[] = "/tmp/bluewake-audio-watch-XXXXXX";
    const int fd = mkstemp(path);
    assert(fd >= 0);
    assert(freopen(path, "w", stderr) != NULL);

    CPUState cpu = {0};
    cpu.ram_size = 0x01800000u;
    cpu.ram = calloc(1, cpu.ram_size);
    assert(cpu.ram != NULL);
    cpu.gpr[13] = kR13;
    mem_write32(&cpu, kDemoManager, kManager);
    mem_write32(&cpu, kLastDac, kDac);
    mem_write8(&cpu, kStageName, 's'); mem_write8(&cpu, kStageName + 1, 'e'); mem_write8(&cpu, kStageName + 2, 'a');
    mem_write16(&cpu, kEventId, 38);

    retraces(&cpu, 1);  // no cutscene: nothing
    mem_write32(&cpu, kManager + 0xD0u, 0x80200000u);  // a demo file: the cutscene runs
    mem_write32(&cpu, kManager + 0xD4u, 10u);
    retraces(&cpu, 1);
    mem_write32(&cpu, kAdaptor + 0xF0u, 0x1880u);  // a cue that got no sound
    cpu.gpr[4] = kAdaptor;
    bluewake_audio_watch_dispatch(&cpu, BLUEWAKE_AUDIO_WATCH_CUE);
    mem_write32(&cpu, kAdaptor2 + 0xF0u, 0x1881u);  // and one that did
    mem_write32(&cpu, kAdaptor2 + 0xECu, kSound);
    cpu.gpr[4] = kAdaptor2;
    bluewake_audio_watch_dispatch(&cpu, BLUEWAKE_AUDIO_WATCH_CUE);
    bluewake_audio_watch_dispatch(&cpu, BLUEWAKE_AUDIO_WATCH_CUE + 4u);  // not the cue: ignored
    retraces(&cpu, 365);  // silent output
    mem_write16(&cpu, kDac + 100u, 0x4000u);  // sound again
    retraces(&cpu, 3);
    mem_write32(&cpu, kManager + 0xD0u, 0u);  // the cutscene ends
    retraces(&cpu, 1);

    mem_write16(&cpu, kDac + 100u, 0u);  // streamed music playing, output silent
    mem_write32(&cpu, kR13 - 27088u, kStreamObject);
    mem_write32(&cpu, kStreamObject + 0x70u, kSound);
    mem_write8(&cpu, kSound + 5u, 4u);
    retraces(&cpu, 361);

    fflush(stderr);
    FILE* log = fopen(path, "r");
    assert(log != NULL);
    static char text[8192];
    const size_t size = fread(text, 1, sizeof text - 1, log);
    text[size] = '\0';
    fclose(log);
    remove(path);
    assert(strstr(text, "[demo] start stage=sea event=38") != NULL);
    assert(strstr(text, "[demo-sound] stage=sea event=38 frame=10 id=0x00001880 no sound") != NULL);
    assert(strstr(text, "id=0x00001881") == NULL);  // a cue that played is only counted
    assert(strstr(text, "output silent for 6 s in a cutscene (cues=2 sounds=1 missing=1") != NULL);
    assert(strstr(text, "sound back after 6.1 s") != NULL);
    assert(strstr(text, "[demo] end stage=sea event=38 frames=10 cues=2 sounds=1 missing=1 silent=6.1s of ") != NULL);
    assert(strstr(text, "[audio-lost] stage=sea streamed music playing but the output silent for 6 s") != NULL);
    free(cpu.ram);
    printf("audio watch: cutscene cues, silence, recovery and streamed music are reported\n");
    return 0;
}
