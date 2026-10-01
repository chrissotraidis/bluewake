#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include "launch_marker.h"
int main(void) {
    char marker[256], settings[256];
    snprintf(marker, sizeof marker, "launch-test-%lu.pending", (unsigned long)getpid());
    snprintf(settings, sizeof settings, "launch-test-%lu.ini", (unsigned long)getpid());
    FILE* file = fopen(settings, "wb");
    assert(file && fputs("lle_audio=1\nbetterww=1\n", file) >= 0 && fclose(file) == 0);
    assert(!bw_launch_pending(marker));
    assert(bw_launch_begin(marker));
    assert(bw_launch_pending(marker));
    assert(bw_launch_backup(settings));
    char backup[400];
    snprintf(backup, sizeof backup, "%s.before-safe-mode-%lld-%lu", settings,
        (long long)time(NULL), (unsigned long)getpid());
    file = fopen(backup, "rb");
    char bytes[100] = {0};
    assert(file && fread(bytes, 1, sizeof bytes, file) == 23);
    fclose(file);
    assert(strcmp(bytes, "lle_audio=1\nbetterww=1\n") == 0);
    assert(bw_launch_clear(marker) && !bw_launch_pending(marker));
    assert(bw_launch_clear(marker));
    remove(settings); remove(backup);
}
