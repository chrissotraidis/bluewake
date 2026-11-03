#include "cold_fallback.h"

static u32 base_address(const CPUState* cpu, u32 reg) {
    return reg == 0u ? 0u : cpu->gpr[reg];
}

static void set_compare(CPUState* cpu, u32 field, bool less, bool greater,
                        bool equal) {
    u32 bits = less ? 8u : greater ? 4u : equal ? 2u : 0u;
    bits |= (cpu->xer >> 31) & 1u;
    const u32 shift = 28u - field * 4u;
    cpu->cr = (cpu->cr & ~(0xFu << shift)) | (bits << shift);
}

static bool branch_condition(CPUState* cpu, u32 bo, u32 bi) {
    bool ctr_ok = true;
    if ((bo & 4u) == 0u) {
        cpu->ctr--;
        ctr_ok = (cpu->ctr != 0u) != ((bo & 2u) != 0u);
    }
    const bool cr_ok =
        (bo & 16u) != 0u ||
        (((cpu->cr >> (31u - bi)) & 1u) == ((bo >> 3u) & 1u));
    return ctr_ok && cr_ok;
}

static void update_fp_record(CPUState* cpu, u32 raw) {
    if ((raw & 1u) != 0u)
        cpu->cr = (cpu->cr & 0xF0FFFFFFu) |
                  ((cpu->fpscr >> 4) & 0x0F000000u);
}

bool bluewake_cold_fallback_execute(CPUState* cpu, u32 raw, u32 cia) {
    if (cpu == NULL)
        return false;
    const u32 op = raw >> 26;
    const u32 rd = (raw >> 21) & 31u;
    const u32 ra = (raw >> 16) & 31u;
    const u32 imm = raw & 0xFFFFu;
    const s32 simm = sign_extend(imm, 16u);
    const u32 base = base_address(cpu, ra);

    switch (op) {
    case 10u: {
        const u32 value = cpu->gpr[ra];
        set_compare(cpu, (raw >> 23) & 7u, value < imm, value > imm,
                    value == imm);
        break;
    }
    case 11u: {
        const s32 value = (s32)cpu->gpr[ra];
        set_compare(cpu, (raw >> 23) & 7u, value < simm, value > simm,
                    value == simm);
        break;
    }
    case 14u:
        cpu->gpr[rd] = base + (u32)simm;
        break;
    case 15u:
        cpu->gpr[rd] = base + ((u32)simm << 16);
        break;
    case 24u:
        cpu->gpr[ra] = cpu->gpr[rd] | imm;
        break;
    case 25u:
        cpu->gpr[ra] = cpu->gpr[rd] | (imm << 16);
        break;
    case 26u:
        cpu->gpr[ra] = cpu->gpr[rd] ^ imm;
        break;
    case 27u:
        cpu->gpr[ra] = cpu->gpr[rd] ^ (imm << 16);
        break;
    case 28u:
    case 29u: {
        cpu->gpr[ra] = cpu->gpr[rd] & (op == 28u ? imm : imm << 16);
        const s32 value = (s32)cpu->gpr[ra];
        set_compare(cpu, 0u, value < 0, value > 0, value == 0);
        break;
    }
    case 16u: {
        const u32 bo = (raw >> 21) & 31u;
        const u32 bi = (raw >> 16) & 31u;
        const u32 next = cia + 4u;
        if ((raw & 1u) != 0u)
            cpu->lr = next;
        cpu->pc = branch_condition(cpu, bo, bi)
                      ? ((raw & 2u) != 0u
                             ? (u32)sign_extend(raw & 0xFFFCu, 16u)
                             : cia + (u32)sign_extend(raw & 0xFFFCu, 16u))
                      : next;
        return true;
    }
    case 18u: {
        const u32 next = cia + 4u;
        if ((raw & 1u) != 0u)
            cpu->lr = next;
        cpu->pc = (raw & 2u) != 0u
                      ? (u32)sign_extend(raw & 0x03FFFFFCu, 26u)
                      : cia + (u32)sign_extend(raw & 0x03FFFFFCu, 26u);
        return true;
    }
    case 19u: {
        const u32 xo = (raw >> 1) & 0x3FFu;
        if (xo == 0u) {
            const u32 dst = (raw >> 23) & 7u;
            const u32 src = (raw >> 18) & 7u;
            const u32 src_shift = 28u - src * 4u;
            const u32 dst_shift = 28u - dst * 4u;
            cpu->cr = (cpu->cr & ~(0xFu << dst_shift)) |
                      (((cpu->cr >> src_shift) & 0xFu) << dst_shift);
            break;
        }
        if (xo == 16u || xo == 528u) {
            const u32 bo = (raw >> 21) & 31u;
            const u32 bi = (raw >> 16) & 31u;
            const u32 next = cia + 4u;
            const u32 target = (xo == 16u ? cpu->lr : cpu->ctr) & ~3u;
            if ((raw & 1u) != 0u)
                cpu->lr = next;
            cpu->pc = branch_condition(cpu, bo, bi) ? target : next;
            return true;
        }
        const u32 bt = (raw >> 21) & 31u;
        const u32 ba = (raw >> 16) & 31u;
        const u32 bb = (raw >> 11) & 31u;
        const bool a = ((cpu->cr >> (31u - ba)) & 1u) != 0u;
        const bool b = ((cpu->cr >> (31u - bb)) & 1u) != 0u;
        bool value;
        switch (xo) {
        case 33u: value = !(a || b); break;
        case 129u: value = a && !b; break;
        case 193u: value = a != b; break;
        case 225u: value = !(a && b); break;
        case 257u: value = a && b; break;
        case 289u: value = a == b; break;
        case 417u: value = a || !b; break;
        case 449u: value = a || b; break;
        default: return false;
        }
        const u32 mask = 0x80000000u >> bt;
        cpu->cr = (cpu->cr & ~mask) | (value ? mask : 0u);
        break;
    }
    case 32u:
    case 33u:
        cpu->gpr[rd] = mem_read32(cpu, base + (u32)simm);
        if (cpu->exception != 0u)
            return true;
        if (op == 33u)
            cpu->gpr[ra] = base + (u32)simm;
        break;
    case 34u:
    case 35u:
        cpu->gpr[rd] = mem_read8(cpu, base + (u32)simm);
        if (cpu->exception != 0u)
            return true;
        if (op == 35u)
            cpu->gpr[ra] = base + (u32)simm;
        break;
    case 36u:
    case 37u:
        mem_write32(cpu, base + (u32)simm, cpu->gpr[rd]);
        if (cpu->exception != 0u)
            return true;
        if (op == 37u)
            cpu->gpr[ra] = base + (u32)simm;
        break;
    case 38u:
    case 39u:
        mem_write8(cpu, base + (u32)simm, (u8)cpu->gpr[rd]);
        if (cpu->exception != 0u)
            return true;
        if (op == 39u)
            cpu->gpr[ra] = base + (u32)simm;
        break;
    case 40u:
    case 41u:
    case 42u:
    case 43u: {
        const u16 value = mem_read16(cpu, base + (u32)simm);
        cpu->gpr[rd] = op >= 42u ? (u32)(s32)(s16)value : value;
        if (cpu->exception != 0u)
            return true;
        if ((op & 1u) != 0u)
            cpu->gpr[ra] = base + (u32)simm;
        break;
    }
    case 44u:
    case 45u:
        mem_write16(cpu, base + (u32)simm, (u16)cpu->gpr[rd]);
        if (cpu->exception != 0u)
            return true;
        if (op == 45u)
            cpu->gpr[ra] = base + (u32)simm;
        break;
    case 48u:
    case 49u:
    case 50u:
    case 51u:
    case 52u:
    case 53u:
    case 54u:
    case 55u: {
        if (!ppc_fp_available(cpu, cia))
            return true;
        const u32 address = base + (u32)simm;
        bool completed;
        if (op <= 49u)
            completed = ppc_lfs_op(cpu, (u8)rd, address, cia);
        else if (op <= 51u)
            completed = ppc_lfd_op(cpu, (u8)rd, address, cia);
        else if (op <= 53u)
            completed = ppc_stfs_op(cpu, (u8)rd, address, cia);
        else
            completed = ppc_stfd_op(cpu, (u8)rd, address, cia);
        if (!completed || cpu->exception != 0u)
            return true;
        if ((op & 1u) != 0u)
            cpu->gpr[ra] = address;
        break;
    }
    case 59u: {
        if (!ppc_fp_available(cpu, cia))
            return true;
        const u8 fd = (u8)rd;
        const u8 fa = (u8)ra;
        const u8 fb = (u8)((raw >> 11) & 31u);
        const u8 fc = (u8)((raw >> 6) & 31u);
        switch ((raw >> 1) & 31u) {
        case 18u:
            ppc_fdivs(cpu, fd, fa, fb);
            break;
        case 20u:
            ppc_fsubs(cpu, fd, fa, fb);
            break;
        case 21u:
            ppc_fadds(cpu, fd, fa, fb);
            break;
        case 25u:
            ppc_fmuls(cpu, fd, fa, fc);
            break;
        case 28u:
        case 29u:
        case 30u:
        case 31u: {
            const u32 xo = (raw >> 1) & 31u;
            ppc_fmadd_op(cpu, fd, fa, fc, fb, true,
                         xo == 28u || xo == 30u, xo >= 30u);
            break;
        }
        default:
            return false;
        }
        update_fp_record(cpu, raw);
        break;
    }
    case 63u: {
        if (!ppc_fp_available(cpu, cia))
            return true;
        const u32 xo = (raw >> 1) & 0x3FFu;
        if (xo != 0u && xo != 32u)
            return false;
        const u8 fa = (u8)ra;
        const u8 fb = (u8)((raw >> 11) & 31u);
        ppc_fcmp(cpu, (u8)((raw >> 23) & 7u), cpu->fpr[fa], cpu->fpr[fb],
                 xo == 32u);
        break;
    }
    default:
        return false;
    }

    cpu->pc = cia + 4u;
    return true;
}
