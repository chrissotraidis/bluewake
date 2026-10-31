// SPDX-License-Identifier: GPL-3.0-or-later
// The first-run importer's free-space verdict (#479). A disc image is not
// needed: the verdict is the comparison and the sentence the player reads, and
// that is what this checks.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "disc_import.h"

#define MB (1024 * 1024)
#define GB (1024ll * 1024ll * 1024ll)

int main(void) {
    char error[512];

    // Room to spare: the import goes ahead and says nothing.
    error[0] = '\0';
    assert(bluewake_disc_space_verdict("/data/bluewake", 11 * MB, 229 * MB,
                                      error, sizeof error) == 0);
    assert(error[0] == '\0');

    // Exactly enough is enough, and a platform that cannot report free space
    // (statvfs/GetDiskFreeSpaceEx failed) never refuses the import.
    assert(bluewake_disc_space_verdict("/data/bluewake", 11 * MB, 11 * MB,
                                      error, sizeof error) == 0);
    assert(bluewake_disc_space_verdict("/data/bluewake", 11 * MB, -1, error,
                                      sizeof error) == 0);

    // Short of room: refused, naming the folder, what is needed and what is free
    // — the three things the report said were missing (#479).
    error[0] = '\0';
    assert(bluewake_disc_space_verdict("/home/deck/.local/share/BlueWake",
                                      12 * MB, 229 * 1024, error,
                                      sizeof error) == -1);
    assert(strstr(error, "/home/deck/.local/share/BlueWake") != NULL);
    assert(strstr(error, "12 MB") != NULL);
    assert(strstr(error, "229 KB") != NULL);

    // Sizes read the way a file manager shows them: GB with one decimal, MB
    // rounded, KB below a megabyte.
    assert(bluewake_disc_space_verdict("/d", 5 * GB, 229 * MB, error,
                                      sizeof error) == -1);
    assert(strstr(error, "5.0 GB") != NULL);
    assert(strstr(error, "229 MB") != NULL);

    printf("disc space verdict: ok\n");
    return 0;
}
