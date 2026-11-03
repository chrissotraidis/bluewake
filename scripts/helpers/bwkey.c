// BlueWake key injector: post a key down/up straight to a pid, bypassing focus.
//
// This source lives in the repository for the same reason bwfocus.m does: the
// acceptance test needs it and a binary in /tmp cannot be rebuilt.
#include <ApplicationServices/ApplicationServices.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: bwkey PID KEYCODE [hold_ms]\n");
        return 2;
    }
    pid_t pid = (pid_t)atoi(argv[1]);
    CGKeyCode code = (CGKeyCode)atoi(argv[2]);
    int hold_ms = argc > 3 ? atoi(argv[3]) : 200;
    CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
    CGEventRef down = CGEventCreateKeyboardEvent(source, code, true);
    CGEventRef up = CGEventCreateKeyboardEvent(source, code, false);
    if (down == NULL || up == NULL) {
        fprintf(stderr, "bwkey: could not create events\n");
        return 1;
    }
    // pid 0 means post to the HID tap (whichever window the window server has key).
    if (pid == 0) {
        CGEventPost(kCGHIDEventTap, down);
    } else {
        CGEventPostToPid(pid, down);
    }
    usleep((useconds_t)hold_ms * 1000u);
    if (pid == 0) {
        CGEventPost(kCGHIDEventTap, up);
    } else {
        CGEventPostToPid(pid, up);
    }
    CFRelease(down);
    CFRelease(up);
    if (source != NULL) CFRelease(source);
    return 0;
}
