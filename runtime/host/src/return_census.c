#include "return_census.h"

#include <stdbool.h>
#include <string.h>

static int range_index(const StaticRecompRange* ranges, u32 count, u32 address) {
    u32 lo = 0u;
    u32 hi = count;
    while (lo < hi) {
        const u32 mid = lo + (hi - lo) / 2u;
        if (ranges[mid].start <= address)
            lo = mid + 1u;
        else
            hi = mid;
    }
    if (lo == 0u || address >= ranges[lo - 1u].end)
        return -1;
    return (int)(lo - 1u);
}

static bool is_chunk_entry(const StaticRecompModuleDesc* module, u32 address) {
    const int index = range_index(module->chunk_ranges,
                                  module->num_chunk_ranges, address);
    return index >= 0 && module->chunk_ranges[index].start == address;
}

static void record_edge(BluewakeReturnCensus* census, u32 input_pc,
                        u32 output_pc, u64 cycles) {
    u32 slot = ((input_pc * 16777619u) ^ output_pc) &
               (BLUEWAKE_RETURN_CENSUS_EDGE_CAPACITY - 1u);
    for (u32 probe = 0u; probe < BLUEWAKE_RETURN_CENSUS_EDGE_CAPACITY;
         probe++) {
        BluewakeReturnEdge* edge = &census->edges[slot];
        if (edge->count == 0u) {
            edge->input_pc = input_pc;
            edge->output_pc = output_pc;
        }
        if (edge->input_pc == input_pc && edge->output_pc == output_pc) {
            edge->count++;
            edge->cycles += cycles;
            return;
        }
        slot = (slot + 1u) & (BLUEWAKE_RETURN_CENSUS_EDGE_CAPACITY - 1u);
    }
    census->dropped_edges++;
}

void bluewake_return_census_init(BluewakeReturnCensus* census) {
    memset(census, 0, sizeof(*census));
}

void bluewake_return_census_record(BluewakeReturnCensus* census,
                                   const StaticRecompModuleDesc* module,
                                   u32 input_pc, const CPUState* cpu,
                                   int dispatched) {
    const u32 output_pc = cpu->pc;
    const u64 cycles = cpu->downcount < 0 ? (u64)(-cpu->downcount) : 0u;
    BluewakeReturnReason reason;
    if (!dispatched) {
        reason = BLUEWAKE_RETURN_MISS;
    } else if (cpu->exception != 0u) {
        reason = BLUEWAKE_RETURN_EXCEPTION;
    } else if (cpu->cycle_budget > 0 && cpu->downcount <= -cpu->cycle_budget) {
        reason = BLUEWAKE_RETURN_BUDGET;
    } else {
        const int input_code = range_index(module->code_ranges,
                                           module->num_code_ranges, input_pc);
        const int output_code = range_index(module->code_ranges,
                                            module->num_code_ranges, output_pc);
        if (output_code < 0) {
            reason = BLUEWAKE_RETURN_OUTSIDE_CODE;
        } else {
            const int input_chunk = range_index(module->chunk_ranges,
                                                module->num_chunk_ranges,
                                                input_pc);
            const int output_chunk = range_index(module->chunk_ranges,
                                                 module->num_chunk_ranges,
                                                 output_pc);
            reason = input_chunk >= 0 && input_chunk == output_chunk
                         ? BLUEWAKE_RETURN_SAME_CHUNK
                         : BLUEWAKE_RETURN_CROSS_CHUNK;
            if (input_code >= 0 && input_code == output_code &&
                is_chunk_entry(module, output_pc))
                census->eligible_chunk_entries++;
        }
    }
    census->reasons[reason]++;
    if (census->edges_enabled)
        record_edge(census, input_pc, output_pc, cycles);
}

void bluewake_return_census_print(const BluewakeReturnCensus* census,
                                  FILE* output) {
    static const char* names[BLUEWAKE_RETURN_CENSUS_REASON_COUNT] = {
        "miss", "exception", "budget", "same_chunk", "cross_chunk",
        "outside_code",
    };
    u64 total = 0u;
    for (u32 i = 0u; i < BLUEWAKE_RETURN_CENSUS_REASON_COUNT; i++)
        total += census->reasons[i];
    fprintf(output, "[return-census] total=%llu eligible_chunk_entries=%llu "
                    "dropped_edges=%llu",
            (unsigned long long)total,
            (unsigned long long)census->eligible_chunk_entries,
            (unsigned long long)census->dropped_edges);
    for (u32 i = 0u; i < BLUEWAKE_RETURN_CENSUS_REASON_COUNT; i++)
        fprintf(output, " %s=%llu", names[i],
                (unsigned long long)census->reasons[i]);
    fputc('\n', output);

    if (!census->edges_enabled)
        return;
    bool selected[BLUEWAKE_RETURN_CENSUS_EDGE_CAPACITY] = {false};
    for (u32 rank = 0u; rank < 16u; rank++) {
        u32 best = BLUEWAKE_RETURN_CENSUS_EDGE_CAPACITY;
        for (u32 i = 0u; i < BLUEWAKE_RETURN_CENSUS_EDGE_CAPACITY; i++) {
            if (!selected[i] && census->edges[i].count != 0u &&
                (best == BLUEWAKE_RETURN_CENSUS_EDGE_CAPACITY ||
                 census->edges[i].count > census->edges[best].count))
                best = i;
        }
        if (best == BLUEWAKE_RETURN_CENSUS_EDGE_CAPACITY)
            break;
        selected[best] = true;
        fprintf(output,
                "[return-census] edge[%u] input=0x%08X output=0x%08X "
                "count=%llu cycles=%llu\n",
                rank, census->edges[best].input_pc,
                census->edges[best].output_pc,
                (unsigned long long)census->edges[best].count,
                (unsigned long long)census->edges[best].cycles);
    }
}
