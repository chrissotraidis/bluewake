#include "return_census.h"

#include <string.h>

int main(void) {
    static const StaticRecompRange code[] = {
        {0x80001000u, 0x80001200u}, {0x80002000u, 0x80002100u},
    };
    static const StaticRecompRange chunks[] = {
        {0x80001000u, 0x80001100u}, {0x80001100u, 0x80001200u},
        {0x80002000u, 0x80002100u},
    };
    StaticRecompModuleDesc module;
    CPUState cpu;
    BluewakeReturnCensus census;
    memset(&module, 0, sizeof(module));
    memset(&cpu, 0, sizeof(cpu));
    module.code_ranges = code;
    module.num_code_ranges = 2u;
    module.chunk_ranges = chunks;
    module.num_chunk_ranges = 3u;
    bluewake_return_census_init(&census);

    cpu.pc = 0x80001010u;
    bluewake_return_census_record(&census, &module, 0x80001000u, &cpu, 0);
    cpu.exception = 1u;
    bluewake_return_census_record(&census, &module, 0x80001000u, &cpu, 1);
    cpu.exception = 0u;
    cpu.cycle_budget = 256;
    cpu.downcount = -256;
    bluewake_return_census_record(&census, &module, 0x80001000u, &cpu, 1);
    cpu.cycle_budget = 0;
    cpu.downcount = -12;
    bluewake_return_census_record(&census, &module, 0x80001000u, &cpu, 1);
    cpu.pc = 0x80001100u;
    bluewake_return_census_record(&census, &module, 0x80001000u, &cpu, 1);
    cpu.pc = 0x80002000u;
    bluewake_return_census_record(&census, &module, 0x80001000u, &cpu, 1);
    cpu.pc = 0x90000000u;
    bluewake_return_census_record(&census, &module, 0x80001000u, &cpu, 1);

    return !(census.reasons[BLUEWAKE_RETURN_MISS] == 1u &&
             census.reasons[BLUEWAKE_RETURN_EXCEPTION] == 1u &&
             census.reasons[BLUEWAKE_RETURN_BUDGET] == 1u &&
             census.reasons[BLUEWAKE_RETURN_SAME_CHUNK] == 1u &&
             census.reasons[BLUEWAKE_RETURN_CROSS_CHUNK] == 2u &&
             census.reasons[BLUEWAKE_RETURN_OUTSIDE_CODE] == 1u &&
             census.eligible_chunk_entries == 1u &&
             census.dropped_edges == 0u);
}
