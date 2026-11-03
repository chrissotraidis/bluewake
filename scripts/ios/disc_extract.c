// Extract main.dol and rels/ from a user-owned GZLE01 disc image on the Mac.
//
// A command-line wrapper around the iOS app's own first-run importer
// (apple/ios/src/disc_import.c), so the Mac preparation and the on-device
// preparation are the same code: it checks the disc id, verifies main.dol's
// SHA-1 against USA revision 0 and writes OUT/main.dol and OUT/rels/*.rel.
//
// Build and run (scripts/ios/build_device.sh does this):
//   clang -O2 -o disc_extract scripts/ios/disc_extract.c apple/ios/src/disc_import.c \
//       -Iapple/ios/src
//   ./disc_extract "GZLE01.iso" OUT
#include "disc_import.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static void progress(void* context, double fraction, const char* stage) {
    (void)context;
    fprintf(stderr, "\rdisc_extract: %3d%% %s          ", (int)(fraction * 100.0), stage ? stage : "");
}

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: disc_extract ISO OUT_DIR\n");
        return 2;
    }
    char error[512] = {0};
    if (mkdir(argv[2], 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "disc_extract: cannot create %s: %s\n", argv[2], strerror(errno));
        return 1;
    }
    if (bluewake_disc_check(argv[1], error, sizeof error) != 0) {
        fprintf(stderr, "disc_extract: %s\n", error);
        return 1;
    }
    if (bluewake_disc_prepare(argv[1], argv[2], progress, NULL, error, sizeof error) != 0) {
        fprintf(stderr, "\ndisc_extract: %s\n", error);
        return 1;
    }
    fprintf(stderr, "\ndisc_extract: wrote %s/main.dol and %s/rels\n", argv[2], argv[2]);
    return 0;
}
