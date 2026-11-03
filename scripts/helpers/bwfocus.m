// BlueWake focus helper: activate a bare-binary app by pid so it can take keys.
//
// This source lives in the repository because the acceptance test cannot be
// reproduced from a clean checkout without it. The binary it used to be built
// into existed only under /tmp, so a reboot or a tmp cleaner left the harness
// unable to run with no way to rebuild it.
#import <AppKit/AppKit.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: bwfocus PID [settle_ms]\n");
        return 2;
    }
    pid_t pid = (pid_t)atoi(argv[1]);
    int settle_ms = argc > 2 ? atoi(argv[2]) : 400;
    NSRunningApplication* app =
        [NSRunningApplication runningApplicationWithProcessIdentifier:pid];
    if (app == nil) {
        fprintf(stderr, "bwfocus: no running application for pid %d\n", (int)pid);
        return 1;
    }
    printf("bwfocus: policy=%ld name=%s\n", (long)[app activationPolicy],
           [[app localizedName] UTF8String]);
    // macOS 14 deprecated NSApplicationActivateIgnoringOtherApps and made it a
    // no-op, so the old call here reported activate=1 while the app never held a
    // key window and no posted key ever reached it. That is the "another window
    // swallowed the keys" failure the harness kept recording. The cooperative
    // call is the supported replacement on 14 and later.
    BOOL requested;
    if (@available(macOS 14.0, *)) {
        requested = [app activateFromApplication:[NSRunningApplication currentApplication]
                                        options:NSApplicationActivateAllWindows];
        printf("bwfocus: cooperative-activate=%d\n", (int)requested);
    } else {
        requested = [app activateWithOptions:NSApplicationActivateIgnoringOtherApps];
    }
    printf("bwfocus: activate=%d\n", (int)requested);
    usleep((useconds_t)settle_ms * 1000u);
    // The request's return value is not the answer. The window server grants or
    // refuses activation, and only the app's own isActive says which happened: a
    // caller that trusts the request reports success while no key can land.
    BOOL active = [app isActive];
    printf("bwfocus: active=%d\n", (int)active);
    return active ? 0 : 1;
}
