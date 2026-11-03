#ifndef BLUEWAKE_GUEST_CHECKPOINT_H
#define BLUEWAKE_GUEST_CHECKPOINT_H

#include "core/cpu.h"
#include <xxhash.h>

/* Same-architecture diagnostic hash. Copy fields individually so padding and
 * ASLR addresses cannot create false differences. Keep this list in sync with
 * the pinned CPUState; native callback presence is retained separately. */
static inline XXH128_hash_t bluewake_guest_cpu_hash(const CPUState* cpu) {
    struct {
        CPUState state;
        u32 pointer_presence;
    } snapshot;
    memset(&snapshot, 0, sizeof snapshot);
#define COPY(member) memcpy(&snapshot.state.member, &cpu->member, sizeof cpu->member)
    COPY(gpr); COPY(fpr); COPY(ps1);
    COPY(pc); COPY(lr); COPY(ctr); COPY(cr); COPY(xer); COPY(fpscr);
    COPY(msr); COPY(srr0); COPY(srr1); COPY(dar); COPY(dsisr); COPY(ear);
    COPY(hid2); COPY(timebase); COPY(sr); COPY(gqr);
    COPY(exception); COPY(program_exception);
    COPY(tlb_last_vps); COPY(tlb_last_index); COPY(tlb_invalidate_count);
    COPY(external_addr); COPY(external_value); COPY(external_rid);
    COPY(external_read_count); COPY(external_write_count);
    COPY(reserve_addr); COPY(reserve_valid);
    COPY(locked_cache_tag); COPY(locked_cache_valid);
    COPY(ram_size); COPY(exram_size);
    COPY(downcount); COPY(cycle_budget); COPY(cycle_observation_suffix);
    COPY(cycle_deadline_active); COPY(cycle_deadline_budget);
#undef COPY
#define POINTER(member, bit) snapshot.pointer_presence |= (cpu->member != NULL) << (bit)
    POINTER(external_read, 0); POINTER(external_write, 1);
    POINTER(external_read32, 2); POINTER(external_write32, 3);
    POINTER(instruction_fallback, 4); POINTER(host_call, 5);
    POINTER(external_user_data, 6); POINTER(ram, 7);
    POINTER(external_pointer, 8); POINTER(exram, 9);
    POINTER(spr_read, 10); POINTER(spr_write, 11); POINTER(cache_control, 12);
#undef POINTER
    return XXH3_128bits(&snapshot, sizeof snapshot);
}

/* Missing/empty/zero disables the diagnostic. Invalid input must be reported,
 * rather than producing a successful run with absent checkpoint evidence. */
static inline bool bluewake_guest_checkpoint_interval(const char* value, u32* result) {
    u32 number = 0;
    if (value != NULL) {
        for (; *value != '\0'; ++value) {
            if (*value < '0' || *value > '9')
                return false;
            const u32 digit = (u32)(*value - '0');
            if (number > (UINT32_MAX - digit) / 10u)
                return false;
            number = number * 10u + digit;
        }
    }
    *result = number;
    return true;
}

#endif
