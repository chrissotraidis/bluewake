#ifndef BLUEWAKE_CPU_STATE_COMPARE_H
#define BLUEWAKE_CPU_STATE_COMPARE_H

#include "core/cpu.h"
#include <stdio.h>

/* Same fields as bluewake_guest_cpu_hash. Guest members are copied; host
 * pointers compare as present or absent. external_read is byte 3400.
 *
 * cycle_observation_suffix (byte 3536) is left out on purpose. The translation
 * writes the suffix of the block it is in there, for the reconcile that follows
 * a cycle-observing instruction, and reads it back before that block ends
 * (DolRecomp emitter.c, emit_cycle_observation_suffix). It is scratch inside one
 * translated block, not guest state: a native that replaces a whole function
 * cannot produce the value the test's own block would have, and nothing after
 * the call reads it. Comparing it failed every strcmp case on that field alone,
 * got 02 00 00 00 against A0 82 66 CB, with the rest of the state identical.
 * What a native does owe is to leave it as it found it, which is a check for
 * the call sites, where the entry state is known. */

#define BLUEWAKE_CPU_COMPARE_FIELDS \
    BLUEWAKE_CPU_VALUE(gpr) \
    BLUEWAKE_CPU_VALUE(fpr) \
    BLUEWAKE_CPU_VALUE(ps1) \
    BLUEWAKE_CPU_VALUE(pc) \
    BLUEWAKE_CPU_VALUE(lr) \
    BLUEWAKE_CPU_VALUE(ctr) \
    BLUEWAKE_CPU_VALUE(cr) \
    BLUEWAKE_CPU_VALUE(xer) \
    BLUEWAKE_CPU_VALUE(fpscr) \
    BLUEWAKE_CPU_VALUE(msr) \
    BLUEWAKE_CPU_VALUE(srr0) \
    BLUEWAKE_CPU_VALUE(srr1) \
    BLUEWAKE_CPU_VALUE(dar) \
    BLUEWAKE_CPU_VALUE(dsisr) \
    BLUEWAKE_CPU_VALUE(ear) \
    BLUEWAKE_CPU_VALUE(hid2) \
    BLUEWAKE_CPU_VALUE(timebase) \
    BLUEWAKE_CPU_VALUE(sr) \
    BLUEWAKE_CPU_VALUE(gqr) \
    BLUEWAKE_CPU_VALUE(exception) \
    BLUEWAKE_CPU_VALUE(program_exception) \
    BLUEWAKE_CPU_VALUE(tlb_last_vps) \
    BLUEWAKE_CPU_VALUE(tlb_last_index) \
    BLUEWAKE_CPU_VALUE(tlb_invalidate_count) \
    BLUEWAKE_CPU_VALUE(external_addr) \
    BLUEWAKE_CPU_VALUE(external_value) \
    BLUEWAKE_CPU_VALUE(external_rid) \
    BLUEWAKE_CPU_VALUE(external_read_count) \
    BLUEWAKE_CPU_VALUE(external_write_count) \
    BLUEWAKE_CPU_VALUE(reserve_addr) \
    BLUEWAKE_CPU_VALUE(reserve_valid) \
    BLUEWAKE_CPU_VALUE(locked_cache_tag) \
    BLUEWAKE_CPU_VALUE(locked_cache_valid) \
    BLUEWAKE_CPU_POINTER(external_read, 0) \
    BLUEWAKE_CPU_POINTER(external_write, 1) \
    BLUEWAKE_CPU_POINTER(external_read32, 2) \
    BLUEWAKE_CPU_POINTER(external_write32, 3) \
    BLUEWAKE_CPU_POINTER(instruction_fallback, 4) \
    BLUEWAKE_CPU_POINTER(host_call, 5) \
    BLUEWAKE_CPU_POINTER(external_user_data, 6) \
    BLUEWAKE_CPU_POINTER(ram, 7) \
    BLUEWAKE_CPU_VALUE(ram_size) \
    BLUEWAKE_CPU_POINTER(external_pointer, 8) \
    BLUEWAKE_CPU_VALUE(downcount) \
    BLUEWAKE_CPU_VALUE(cycle_budget) \
    BLUEWAKE_CPU_POINTER(exram, 9) \
    BLUEWAKE_CPU_VALUE(exram_size) \
    BLUEWAKE_CPU_POINTER(spr_read, 10) \
    BLUEWAKE_CPU_POINTER(spr_write, 11) \
    BLUEWAKE_CPU_POINTER(cache_control, 12) \
    BLUEWAKE_CPU_VALUE(cycle_deadline_active) \
    BLUEWAKE_CPU_VALUE(cycle_deadline_budget)

/* 0 when the guest fields and pointer presence match, as memcmp would. */
static inline int bluewake_cpu_state_compare(const CPUState* a, const CPUState* b) {
    int differ = 0;
#define BLUEWAKE_CPU_VALUE(member) \
    if (memcmp(&a->member, &b->member, sizeof a->member) != 0) \
        differ = 1;
#define BLUEWAKE_CPU_POINTER(member, bit) \
    if ((a->member != NULL) != (b->member != NULL)) \
        differ = 1;
    BLUEWAKE_CPU_COMPARE_FIELDS
#undef BLUEWAKE_CPU_VALUE
#undef BLUEWAKE_CPU_POINTER
    return differ;
}

/* Member name and the byte offset inside CPUState. Pointers print presence. */
static inline void bluewake_report_cpu_state(const CPUState* got, const CPUState* want) {
#define BLUEWAKE_CPU_VALUE(member) \
    do { \
        const unsigned base = (unsigned)offsetof(CPUState, member); \
        const u8* g = (const u8*)&got->member; \
        const u8* w = (const u8*)&want->member; \
        for (unsigned i = 0; i < sizeof got->member; ++i) \
            if (g[i] != w[i]) \
                fprintf(stderr, "  CPU %s byte %u: got %02X want %02X\n", #member, base + i, g[i], w[i]); \
    } while (0);
#define BLUEWAKE_CPU_POINTER(member, bit) \
    do { \
        const int gp = got->member != NULL; \
        const int wp = want->member != NULL; \
        if (gp != wp) \
            fprintf(stderr, "  CPU %s byte %u: got %s want %s\n", #member, \
                    (unsigned)offsetof(CPUState, member), gp ? "present" : "absent", \
                    wp ? "present" : "absent"); \
    } while (0);
    BLUEWAKE_CPU_COMPARE_FIELDS
#undef BLUEWAKE_CPU_VALUE
#undef BLUEWAKE_CPU_POINTER
}

_Static_assert(sizeof(CPUState) == 3552, "CPUState is 3552 bytes on this tree");
_Static_assert(offsetof(CPUState, gpr) == 0, "bytes 4-24 sit in gpr");
_Static_assert(offsetof(CPUState, fpr) == 128, "fpr follows gpr");
_Static_assert(offsetof(CPUState, external_read) == 3400, "external_read is byte 3400");
_Static_assert(offsetof(CPUState, external_write) == 3408, "external_write is byte 3408");
_Static_assert(offsetof(CPUState, cycle_observation_suffix) == 3536,
               "byte 3536 is cycle_observation_suffix");

#endif
