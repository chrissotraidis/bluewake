#ifndef BLUEWAKE_COMPOSITE_GATHER_PIPE_BATCH_H
#define BLUEWAKE_COMPOSITE_GATHER_PIPE_BATCH_H

/* The chunks' batch of gather-pipe bytes (gather_pipe.h), for everything that
 * must hand it to the host before the host looks: the stores and loads that
 * reach the hardware, and the chassis loop wherever it asks the host about a
 * boundary or returns to it (dispatch_loop.h).
 *
 * The words collect in guest byte order and reach the host a batch at a time,
 * a call per batch where there was a call per word. NULL sends each word as
 * it is written. */

#include "core/types.h"

typedef void (*BwGatherPipeBytes)(const u8* bytes, u32 size);

#define BW_GATHER_PIPE_BATCH 256u
extern BwGatherPipeBytes bw_gather_pipe_bytes;
extern u8 bw_gather_pipe_buffer[BW_GATHER_PIPE_BATCH + 8u];
extern u32 bw_gather_pipe_length;
void bw_gather_pipe_flush(void);

static inline void bw_gather_pipe_drain(void) {
    if (__builtin_expect(bw_gather_pipe_length != 0u, 0))
        bw_gather_pipe_flush();
}

#endif
