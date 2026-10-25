#ifdef NDEBUG
#undef NDEBUG
#endif
#include "inline_gpr.h"
#include <assert.h>
#include <stdio.h>

static void journal(u32 offset, u32 size, void* user) {
    (void)offset; (void)size; (void)user;
    assert(!"readiness query must not write memory");
}
/* bw_inline_gpr_frame's direct stores and loads give what the per-register
 * mem_write32/mem_read32 calls give: the same bytes, the same values, and the
 * same reservation state, for every first register, both stack forms and a
 * reservation on every word of RAM in both forms. */
static void direct_matches_helpers(void) {
    static u8 ram_a[256], ram_b[256];
    for (unsigned first = 14; first <= 31; ++first)
        for (unsigned form = 0; form < 2; ++form)
            for (u32 reserve = 0; reserve <= sizeof ram_a; reserve += 4)
                for (unsigned rform = 0; rform < 3; ++rform) {
                    CPUState a = {0}, b = {0};
                    for (unsigned i = 0; i < sizeof ram_a; ++i) ram_a[i] = ram_b[i] = (u8)(i * 7u + 1u);
                    a.ram = ram_a; b.ram = ram_b; a.ram_size = b.ram_size = sizeof ram_a;
                    a.gpr[11] = b.gpr[11] = (form ? 0xC0000000u : 0x80000000u) + 0xC0u;
                    for (unsigned r = 14; r < 32; ++r) a.gpr[r] = b.gpr[r] = 0x01020304u * (r + 1u);
                    a.reserve_valid = b.reserve_valid = rform != 2;
                    a.reserve_addr = b.reserve_addr = (rform == 1 ? 0xC0000000u : 0x80000000u) + reserve;
                    assert(bw_inline_gpr_memory_ready(&b, first));
                    const u32 frame = a.gpr[11];
                    for (unsigned r = first; r < 32; ++r)
                        mem_write32(&a, frame + (u32)(s32)(4 * (int)r - 128), (u32)a.gpr[r]);
                    u8* const slots = bw_inline_gpr_frame(&b, first, true);
                    for (unsigned r = first; r < 32; ++r)
                        write_be32(slots + 4u * (r - first), (u32)b.gpr[r]);
                    assert(a.reserve_valid == b.reserve_valid);
                    for (unsigned i = 0; i < sizeof ram_a; ++i) assert(ram_a[i] == ram_b[i]);
                    for (unsigned r = first; r < 32; ++r) {
                        a.gpr[r] = mem_read32(&a, frame + (u32)(s32)(4 * (int)r - 128));
                        b.gpr[r] = read_be32(bw_inline_gpr_frame(&b, first, false) + 4u * (r - first));
                        assert(a.gpr[r] == b.gpr[r]);
                    }
                    assert(a.reserve_valid == b.reserve_valid);
                }
    puts("Inline GPR: direct frame stores and loads match the memory helpers");
}

int main(void) {
    u8 ram[256] = {0}, alias[4] = {0};
    CPUState cpu = {0};
    cpu.ram = ram; cpu.ram_size = sizeof ram;
    cpu.gpr[11] = 0x80000100u;
    assert(bw_inline_gpr_memory_ready(&cpu, 14));
    assert(bw_inline_gpr_memory_ready(&cpu, 31));
    cpu.reserve_valid = true;
    assert(bw_inline_gpr_memory_ready(&cpu, 14));
    /* An alias elsewhere in MEM1 does not reject this ordinary stack frame. */
    assert(ppc_guest_alias_add_shared(0x80000020u, sizeof alias, alias));
    assert(bw_inline_gpr_memory_ready(&cpu, 14));
    assert(ppc_guest_alias_remove(0x80000020u, sizeof alias));
    /* An alias in the middle must reject, even though resolving the whole
     * frame would miss it. */
    assert(ppc_guest_alias_add_shared(0x800000ECu, sizeof alias, alias));
    assert(!bw_inline_gpr_memory_ready(&cpu, 14));
    assert(bw_inline_gpr_memory_ready(&cpu, 31));
    assert(ppc_guest_alias_remove(0x800000ECu, sizeof alias));
    assert(ppc_guest_alias_add_shared(0x800000ECu, sizeof alias, alias));
    cpu.gpr[11] = 0xC0000100u;
    assert(!bw_inline_gpr_memory_ready(&cpu, 14));
    assert(ppc_guest_alias_remove(0x800000ECu, sizeof alias));
    cpu.gpr[11] = 0x80000048u;
    assert(bw_inline_gpr_memory_ready(&cpu, 14));
    cpu.gpr[11] = 0x80000044u;
    assert(!bw_inline_gpr_memory_ready(&cpu, 14));
    cpu.gpr[11] = 0xC0000100u;
    assert(bw_inline_gpr_memory_ready(&cpu, 14));
    g_mem_write_journal = journal;
    assert(!bw_inline_gpr_memory_ready(&cpu, 31));
    g_mem_write_journal = NULL;
    cpu.gpr[11] = 0xCC008004u;
    assert(!bw_inline_gpr_memory_ready(&cpu, 31));
    cpu.gpr[11] = 0x80000101u;
    assert(!bw_inline_gpr_memory_ready(&cpu, 31));
    cpu.gpr[11] = 0x80000004u;
    cpu.ram_size = 1;
    assert(!bw_inline_gpr_memory_ready(&cpu, 31));
    cpu.ram_size = sizeof ram;
    cpu.ram = NULL;
    assert(!bw_inline_gpr_memory_ready(&cpu, 31));
    cpu.ram = ram;
    assert(!bw_inline_gpr_memory_ready(&cpu, 13));
    assert(!bw_inline_gpr_memory_ready(&cpu, 32));
    for (unsigned i = 0; i < sizeof ram; ++i) assert(ram[i] == 0);
    puts("Inline GPR: plain RAM eligibility excludes aliases, MMIO, journals and bounds");
    direct_matches_helpers();
    return 0;
}
