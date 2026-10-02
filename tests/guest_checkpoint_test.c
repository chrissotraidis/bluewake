#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include "guest_checkpoint.h"

static bool equal(XXH128_hash_t a, XXH128_hash_t b) {
    return a.low64 == b.low64 && a.high64 == b.high64;
}
static u64 read_one(CPUState* cpu, u32 address, u8 size) {
    (void)cpu; (void)address; (void)size; return 0;
}
static u64 read_two(CPUState* cpu, u32 address, u8 size) {
    (void)cpu; (void)address; (void)size; return 1;
}
int main(void) {
    CPUState cpu = {0}, other = {0};
    u8 ram[32] = {0}, alternate[32] = {0};
    cpu.ram = ram; other.ram = alternate;
    cpu.external_read = read_one; other.external_read = read_two;
    const XXH128_hash_t baseline = bluewake_guest_cpu_hash(&cpu);
    assert(equal(baseline, bluewake_guest_cpu_hash(&other)));
    assert(cpu.ram == ram && cpu.external_read == read_one);
    other.external_read = NULL;
    assert(!equal(baseline, bluewake_guest_cpu_hash(&other)));
    other = cpu;
#define CHANGES(member, value) do { \
    other.member = (value); \
    assert(!equal(baseline, bluewake_guest_cpu_hash(&other))); \
    other = cpu; \
} while (0)
    CHANGES(gpr[31], 1); CHANGES(fpr[31], -0.0); CHANGES(ps1[31], 1.0);
    CHANGES(fpscr, 1); CHANGES(msr, 1); CHANGES(timebase, 1);
    CHANGES(reserve_valid, true); CHANGES(reserve_addr, 1);
    CHANGES(locked_cache_valid[511], true); CHANGES(locked_cache_tag[511], 1);
    CHANGES(external_value, 1); CHANGES(external_write_count, 1);
    CHANGES(sr[15], 1); CHANGES(gqr[7], 1); CHANGES(exception, 1);
    CHANGES(ram_size, 1); CHANGES(exram_size, 1);
    CHANGES(downcount, -1); CHANGES(cycle_budget, 1);
    CHANGES(cycle_observation_suffix, 1); CHANGES(cycle_deadline_active, 1);
    CHANGES(cycle_deadline_budget, 1);
#undef CHANGES
    const u64 nan_a = 0x7FF8000000000001ull, nan_b = 0x7FF8000000000002ull;
    memcpy(&cpu.fpr[0], &nan_a, sizeof nan_a);
    memcpy(&other.fpr[0], &nan_b, sizeof nan_b);
    assert(!equal(bluewake_guest_cpu_hash(&cpu), bluewake_guest_cpu_hash(&other)));
    assert(equal(XXH3_128bits(ram, sizeof ram), XXH3_128bits(alternate, sizeof alternate)));
    alternate[31] = 1;
    assert(!equal(XXH3_128bits(ram, sizeof ram), XXH3_128bits(alternate, sizeof alternate)));
    u32 interval = 123;
    assert(bluewake_guest_checkpoint_interval(NULL, &interval) && interval == 0);
    assert(bluewake_guest_checkpoint_interval("", &interval) && interval == 0);
    assert(bluewake_guest_checkpoint_interval("0", &interval) && interval == 0);
    assert(bluewake_guest_checkpoint_interval("1000", &interval) && interval == 1000);
    assert(bluewake_guest_checkpoint_interval("4294967295", &interval) && interval == UINT32_MAX);
    const char* invalid[] = {"-1", "+1", "1x", " 1", "4294967296", "9999999999999999999"};
    for (unsigned i = 0; i < sizeof invalid / sizeof invalid[0]; ++i)
        assert(!bluewake_guest_checkpoint_interval(invalid[i], &interval));
    puts("guest checkpoint state normalization and interval tests passed");
    return 0;
}
