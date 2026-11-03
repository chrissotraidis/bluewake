#ifndef BLUEWAKE_RETURN_CENSUS_H
#define BLUEWAKE_RETURN_CENSUS_H

#include "StaticRecompABI.h"
#include <stdbool.h>
#include <stdio.h>

#define BLUEWAKE_RETURN_CENSUS_EDGE_CAPACITY 4096u
#define BLUEWAKE_RETURN_CENSUS_REASON_COUNT 6u

typedef enum BluewakeReturnReason {
    BLUEWAKE_RETURN_MISS,
    BLUEWAKE_RETURN_EXCEPTION,
    BLUEWAKE_RETURN_BUDGET,
    BLUEWAKE_RETURN_SAME_CHUNK,
    BLUEWAKE_RETURN_CROSS_CHUNK,
    BLUEWAKE_RETURN_OUTSIDE_CODE,
} BluewakeReturnReason;

typedef struct BluewakeReturnEdge {
    u32 input_pc;
    u32 output_pc;
    u64 count;
    u64 cycles;
} BluewakeReturnEdge;

typedef struct BluewakeReturnCensus {
    u64 reasons[BLUEWAKE_RETURN_CENSUS_REASON_COUNT];
    u64 eligible_chunk_entries;
    u64 dropped_edges;
    BluewakeReturnEdge edges[BLUEWAKE_RETURN_CENSUS_EDGE_CAPACITY];
    // Edge attribution is off by default: the bounded table saturates on a
    // long route, and once it is full every dispatch pays a full 4,096-slot
    // probe for no usable detail. Reason totals stay exact either way.
    bool edges_enabled;
} BluewakeReturnCensus;

void bluewake_return_census_init(BluewakeReturnCensus* census);
void bluewake_return_census_record(BluewakeReturnCensus* census,
                                   const StaticRecompModuleDesc* module,
                                   u32 input_pc, const CPUState* cpu,
                                   int dispatched);
void bluewake_return_census_print(const BluewakeReturnCensus* census,
                                  FILE* output);

#endif
