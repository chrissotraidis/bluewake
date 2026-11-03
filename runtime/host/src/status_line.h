#ifndef BLUEWAKE_STATUS_LINE_H
#define BLUEWAKE_STATUS_LINE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// The desktop overlays' status line: a few words at the bottom left after an
// F-key ("State saved", "Fullscreen on"...). Post from any thread (main.c
// keeps it); bluewake_status_read copies the latest out and returns its
// number, which changes with every new message (0: none yet, or mid-write).
void bluewake_status_post(const char* text);
unsigned bluewake_status_read(char* text, size_t size);

#ifdef __cplusplus
}
#endif

#endif
