#ifndef BLUEWAKE_GATHER_PIPE_BRIDGE_H
#define BLUEWAKE_GATHER_PIPE_BRIDGE_H

#include "core/types.h"
#include <string.h>

typedef void (*BluewakeGatherWord)(u64, u8);
typedef void (*BluewakeGatherBytes)(const u8*, u32);
typedef void (*BluewakeSetGatherWord)(BluewakeGatherWord);
typedef void (*BluewakeSetGatherBytes)(BluewakeGatherBytes);
typedef enum BluewakeGatherMode {
    BLUEWAKE_GATHER_OFF,
    BLUEWAKE_GATHER_DIRECT,
    BLUEWAKE_GATHER_BATCH
} BluewakeGatherMode;

/* Disable the old writers first: their setters drain through the previous
 * callback before replacing it. Missing exports keep older modules usable.
 * Diagnostic MMIO observers must see the ordinary per-store handler. */
static inline BluewakeGatherMode bluewake_gather_pipe_configure(
    BluewakeSetGatherWord set_word, BluewakeSetGatherBytes set_bytes,
    BluewakeGatherWord word, BluewakeGatherBytes bytes,
    const char* direct_option, const char* batch_option, bool diagnostic_mmio) {
    if (set_bytes != NULL)
        set_bytes(NULL);
    if (set_word != NULL)
        set_word(NULL);
    if (diagnostic_mmio || set_word == NULL || word == NULL ||
        direct_option == NULL || strcmp(direct_option, "1") != 0)
        return BLUEWAKE_GATHER_OFF;
    set_word(word);
    if (set_bytes != NULL && bytes != NULL && batch_option != NULL &&
        strcmp(batch_option, "1") == 0) {
        set_bytes(bytes);
        return BLUEWAKE_GATHER_BATCH;
    }
    return BLUEWAKE_GATHER_DIRECT;
}

#endif
