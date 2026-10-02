/* The host's GX writers for the chunks' gather-pipe stores (gather_pipe.h),
 * and the chunks' batch of pipe bytes (gather_pipe_batch.h). */
#include "gather_pipe.h"

#if defined(_WIN32)
#define BW_GATHER_PIPE_EXPORT __declspec(dllexport)
#else
#define BW_GATHER_PIPE_EXPORT __attribute__((visibility("default")))
#endif

BwGatherPipeWrite bw_gather_pipe_write;
BwGatherPipeBytes bw_gather_pipe_bytes;
u8 bw_gather_pipe_buffer[BW_GATHER_PIPE_BATCH + 8u];
u32 bw_gather_pipe_length;

void bw_gather_pipe_flush(void) {
    const u32 length = bw_gather_pipe_length;
    bw_gather_pipe_length = 0u;
    if (length != 0u && bw_gather_pipe_bytes != NULL)
        bw_gather_pipe_bytes(bw_gather_pipe_buffer, length);
}

/* The function the host's MMIO handler calls for a pipe store, or NULL to
 * send pipe stores through that handler again. */
BW_GATHER_PIPE_EXPORT void bluewake_composite_set_gather_pipe(BwGatherPipeWrite write) {
    bw_gather_pipe_flush();
    bw_gather_pipe_write = write;
}

/* The host's writer for a run of pipe bytes in guest order, where its GX
 * backend takes the FIFO as a byte stream; NULL sends each store on its own. */
BW_GATHER_PIPE_EXPORT void bluewake_composite_set_gather_pipe_bytes(BwGatherPipeBytes bytes) {
    bw_gather_pipe_flush();
    bw_gather_pipe_bytes = bytes;
}
